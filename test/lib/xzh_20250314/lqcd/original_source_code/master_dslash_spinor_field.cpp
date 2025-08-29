#include <chrono>
#include "master.h"
#include <cstring>
#include <cstdio>
#include "arm_neon.h"
extern master_transfer* mtransfer;
extern Data_send Dsend[2][4];
extern Data_recv Drecv[2][4];
extern int group_id;
extern int mpi_rank;
extern double dslash_send, dslash_main, dslash_recv, dslash_recv_acc, mpi_time,dslash_all;
extern spinor_sliceX* dslash_backward_spinor, *dslash_forward_spinor;
void neon_memcpy(void *dst, void *src, int sz)
{
    if (sz & 63)
        sz = (sz & -64) + 64;
    asm volatile(
        "NEONCopyPLD: \n"
        "LD1 {v0.2d,v1.2d,v2.2d,v3.2d},[%0],#64 \n"
        "ST1 {v0.2d,v1.2d,v2.2d,v3.2d},[%1],#64 \n"
        "SUBS %2,%2,#64 \n"
        "BGT NEONCopyPLD \n"
        : "+r"(src), "+r"(dst), "+r"(sz)
        :
        : "v0", "v1", "v2", "v3", "cc", "memory");
}

void memcpy_neon(void* dst,void* src,size_t size){
    float64x2x4_t r;
    double (*p)[8],(*q)[8];
    p = (double(*)[8])src;
    q = (double(*)[8])dst;
    #pragma omp parallel for num_threads(4) private(r)
    for(int i = 0; i < size/64; ++i){
        r = vld4q_f64((double*)(p+i));
        vst4q_f64((double*)(q+i),r);
        //size -= 8*sizeof(double);
    }
}
void pull_edge_forward(int dim,spinor_field src){
    switch (dim)
    {
    case 0:
        //memcpy(Dsend[e_forward][0].buf,src,Dsend[e_forward][0].num*sizeof(FLOAT));
        memcpy_neon(Dsend[e_forward][0].buf,src,Dsend[e_forward][0].num*sizeof(FLOAT));
        break;
    case 1:
        for(int i = 0;i<NT;++i){
            memcpy_neon(Dsend[e_forward][1].buf+i*NY*NX*24,src[i],sizeof(FLOAT)*NY*NX*24);
        }
        break;
    case 2:
        for(int i = 0; i < NT; ++i)
            for(int j = 0; j < NZ; ++j){
                memcpy_neon(Dsend[e_forward][2].buf+(i*NZ+j)*NX*24,src[i][j],sizeof(FLOAT)*NX*24);
            }
        break;
    case 3:{
    	auto ptr_x = mtransfer->data_send_recv[0][e_forward][3].ptr;
        auto ptr_s = (double*) src;
        for(int i = 0; i < NT*NZ*NY; ++i){
            for(int j = 0; j <3*4*2*16 ; j+=16){
                *(ptr_x+j) = *(ptr_s+j);
            }
            ++ptr_x;
            ptr_s += 3*4*2*32;
            if(i&15==15) ptr_x += 3*4*2*16-4;
        }
        
	}                
    default:
        break;
    }
}
void pull_data_forward(spinor_field src){
    // memcpy_neon(Dsend[e_forward][0].buf,src,Dsend[e_forward][0].num*sizeof(FLOAT));
    // #pragma omp parallel for num_threads(4)
    // for(int i = 0;i<NT;++i){
    //     neon_memcpy(Dsend[e_forward][1].buf+i*NY*NX*24,src[i],sizeof(FLOAT)*NY*NX*24);
    // }
    // #pragma omp parallel for num_threads(4)
    // for(int i = 0; i < NT; ++i)
    //         for(int j = 0; j < NZ; ++j){
    //             neon_memcpy(Dsend[e_forward][2].buf+(i*NZ+j)*NX*24,src[i][j],sizeof(FLOAT)*NX*24);
    //         }

    auto ptr_x = mtransfer->data_send_recv[0][e_forward][3].ptr;
        auto ptr_s = (double*) src;
        #pragma omp parallel for num_threads(4)
        for(int i = 0; i < NT*NZ*NY; ++i){
            asm volatile (
                    "prfm pldl1strm,[%[arr1],#3072]  \n"
                    "prfm pstl1keep,[%[arr2],#3072]  \n"
                    ::[arr1]  "r"(ptr_s),
                      [arr2]    "r"(ptr_x)
                    :
            );
            for(int j = 0; j <3*4*2 ; j++){
                *(ptr_x+j*16) = *(ptr_s+j*16);
            }
            ++ptr_x;
            ptr_s += 3*4*2*16;//跳到下一个y
            if((i&15)==15) ptr_x += 3*4*2*16-16;
        }
    // #pragma omp parallel num_threads(4)
    // {
    //     #pragma omp sections
    //     {
    //         #pragma omp section
    //         {
    //             pull_edge_forward(0,src);
    //         }
    //         #pragma omp section
    //         {
    //             pull_edge_forward(1,src);
    //         }
    //         #pragma omp section
    //         {
    //             pull_edge_forward(2,src);
    //         }
    //         #pragma omp section
    //         {
    //             //pull_edge_forward(3,src);
    //         }
    //     }
    // }
}
// void pull_xdata_backward(spinor_field src){
//     auto ptr = mtransfer->x_data;
//     #pragma omp parallel for num_threads(4)
//     for (int i = 0; i < NT; ++i)
//         for (int j = 0; j < NZ; ++j)
//             for (int k = 0; k < NY; ++k)
//             for (int a = 0; a < 3; ++a)
//                 for (int b = 0; b < 4; ++b)
//                     for (int c = 0; c < 2; ++c)
                        
//                         {
//                             ptr[0][i][j][k/16][a][b][c][k%16] = src[i][j][k][NX/16-1][a][b][c][15];
//                         }
// }
void spinor_field_neighbour_acc(spinor_field dst,t_dir dir){
    {
        int it = dir == e_backward ? 0 : NT - 1;
        spinor_sliceT* spinor_slice_t = (spinor_sliceT*)Drecv[dir][0].buf;
        #pragma omp parallel for num_threads(4)
        for (int iz = 0; iz < NZ; ++iz)
            for (int iy = 0; iy < NY; ++iy)
                for(int ix = 0; ix < 2; ++ix)
                for (int a = 0; a < 3; ++a)
                    for (int b = 0; b < 4; ++b)
                        for (int ir = 0; ir < 2; ++ir)
                            for (int iix = 0; iix < 16; ++iix)
                            { 
                                dst[it][iz][iy][ix][a][b][ir][iix] += spinor_slice_t[0][iz][iy][ix][a][b][ir][iix];
                            }
    }
    {
        int iz = dir == e_backward ? 0 : NZ - 1;
        spinor_sliceZ* spinor_slice_z = (spinor_sliceZ*)Drecv[dir][1].buf;
        #pragma omp parallel for num_threads(4)
        for (int it = 0; it < NT; ++it)
            for (int iy = 0; iy < NY; ++iy)
                for(int ix = 0; ix < 2; ++ix)
                for (int a = 0; a < 3; ++a)
                    for (int b = 0; b < 4; ++b)
                        for (int ir = 0; ir < 2; ++ir)
                            for (int iix = 0; iix < 16; ++iix)
                            {
                                dst[it][iz][iy][ix][a][b][ir][iix] += spinor_slice_z[0][it][iy][ix][a][b][ir][iix];
                            }
    }
    {
        int iy = dir == e_backward ? 0 : NY - 1;
        spinor_sliceY* spinor_slice_y = (spinor_sliceY*)Drecv[dir][2].buf;
        #pragma omp parallel for num_threads(4)
        for (int it = 0; it < NT; ++it)
            for (int iz = 0; iz < NZ; ++iz)
            for(int ix = 0; ix < 2; ++ix)
                for (int a = 0; a < 3; ++a)
                    for (int b = 0; b < 4; ++b)
                        for (int ir = 0; ir < 2; ++ir)
                            for (int iix = 0; iix < 16; ++iix)
                            {
                                dst[it][iz][iy][ix][a][b][ir][iix] += spinor_slice_y[0][it][iz][ix][a][b][ir][iix];
                            }
    }

}
void print_spinor_slice(spinor_sliceX s){
    int i,j,k;
  for(int it=0; it<NT; it++)
    for(int iz=0; iz<NZ; iz++)
        for(int iy=0; iy<NY; iy++){
          for(int a = 0; a < 3; ++a){
            for(int b = 0; b < 4; ++b){
                printf("%lf ",s[it][iz][iy>>4][a][b][0][iy&15]);
            }
            printf("\n");
          }
          printf("\n");
          for(int a = 0; a < 3; ++a){
            for(int b = 0; b < 4; ++b){
                printf("%lf ",s[it][iz][iy>>4][a][b][1][iy&15]);
            }
            printf("\n");
          }
          return;
        }
}
void dslash(spinor_field dst, su3_field link, spinor_field src){
    auto dslash_start = std::chrono::high_resolution_clock::now();
    recv_data(e_forward);
    recv_data(e_backward);
    uint64_t args[5];
    args[0] = (uint64_t) link;
    args[1] = (uint64_t) src;
    args[2] = (uint64_t) mtransfer;
    args[3] = (uint64_t) dslash_backward_spinor;
    args[4] = (uint64_t) dslash_forward_spinor;
    auto start = std::chrono::high_resolution_clock::now();
    hthread_group_exec(group_id,"dslash_backward_neighbour",0,5,args); //(3*4*2*2+2*3*2)*8*16*32*32*3+(3*4*2*2+2*3*2)*8*16*16*32 = 27525120
    hthread_group_wait(group_id);
    auto end = std::chrono::high_resolution_clock::now();
    dslash_send+=std::chrono::duration<double>(end-start).count();
    //print_spinor_slice(mtransfer->data_send_recv[0][e_forward][3].slx[0]);

    auto mpi_start = std::chrono::high_resolution_clock::now();
    send_data(e_forward);
    send_data(e_backward);
    
    args[0] = (uint64_t) dst;
    args[1] = (uint64_t) link;
    args[2] = (uint64_t) src;
    start = std::chrono::high_resolution_clock::now();
    hthread_group_exec(group_id,"dslash_main",0,3,args);//(3*4*2*2+2*3*2*4)*16*16*32*32*8 = 201326592
    hthread_group_wait(group_id);
    end = std::chrono::high_resolution_clock::now();
    dslash_main+=std::chrono::duration<double>(end-start).count();

    mpi_wait(e_forward);
    mpi_wait(e_backward);
    auto mpi_end = std::chrono::high_resolution_clock::now();
    mpi_time+=std::chrono::duration<double>(mpi_end-mpi_start).count();

    args[0] = (uint64_t) dst;
    args[1] = (uint64_t) link;
    args[2] = (uint64_t) src;
    args[3] = (uint64_t) mtransfer;
    start = std::chrono::high_resolution_clock::now();
    hthread_group_exec(group_id,"dslash_forward_neighbour",0,4,args);//(3*4*2*2+2*3*2)*8*16*32*32*2+(3*4*2*2+2*3*2)*8*16*16*32*2 = 27525120
    //spinor_field_neighbour_acc(e_backward,dst);//3*4*2*2*8*16*32*32*3+3*4*2*2*8*16*16*32 = 22020096
    hthread_group_wait(group_id);
    end = std::chrono::high_resolution_clock::now();
    dslash_recv+=std::chrono::duration<double>(end-start).count();

    start = std::chrono::high_resolution_clock::now();
    // spinor_field_neighbour_acc(dst,e_backward);//3*4*2*3*8*16*32*32*2+3*4*2*3*8*16*16*32*2 = 22020096
    // spinor_field_neighbour_acc(dst,e_forward);
    
    end = std::chrono::high_resolution_clock::now();
    dslash_recv_acc+=std::chrono::duration<double>(end-start).count();
    //printf("forward acc time: %lf s\n",std::chrono::duration<double>(end-start).count());
    auto dslash_end = std::chrono::high_resolution_clock::now();
    dslash_all+=std::chrono::duration<double>(dslash_end-dslash_start).count();
    //if(mpi_rank==0)printf("all dslash time: %lf s\n",std::chrono::duration<double>(dslash_end-dslash_start).count());
}

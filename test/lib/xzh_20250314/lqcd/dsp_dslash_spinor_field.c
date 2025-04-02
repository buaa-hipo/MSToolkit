#include "dsp.h"
//#include<hthread_device.h>

#define SLICE_0(i) [i][local_para->thread_id][0]
#define SLICE_1(i) [local_para->thread_id][i][0]
#define SLICE_2(i) [local_para->thread_id][0][i]
#define SLICE_3(i) [local_para->thread_id][0][0][i/16]
void dslash(spinor_field dst, su3_field link, spinor_field src, Para* local_para){
    int i,j,k;
    int dma_handle;
    dma_pull(local_para->local_ferm[0], src SLICE_0(NT-1),sizeof(spinor_local),1,0);
    spinor_local_zero(local_para->local_ferm[1]);
    for(i=NT-1; i>0; --i){
        //dma_pull(local_para->local_ferm[0], dsp_comm[local_para->thread_id][e_forward],sizeof(spinor_local),1,0);

        //z
        dma_pull(local_para->local_link,link[2] SLICE_0(i), sizeof(su3_local),1,0);
        dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[0],1,e_forward,local_para->local_ferm[2]);

        dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[0],1,e_backward,local_para->local_ferm[2]);
        //y
        dma_pull(local_para->local_link, link[1] SLICE_0(i), sizeof(su3_local), 1, 0);
        dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[0],2,e_forward,NULL);
        dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[0],2,e_backward,NULL);
	//if(local_para->thread_id==0&&i==15) print_spinor_vec(local_para->local_ferm[1][1]);
        //x
        dma_pull(local_para->local_link, link[0] SLICE_0(i), sizeof(su3_local), 1, 0);
        dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[0],3,e_forward,local_para->local_ferm[2]);
        dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[0],3,e_backward,local_para->local_ferm[2]);
        


        //t-1 forward
        dma_pull(local_para->local_link, link[3] SLICE_0(i-1), sizeof(su3_local), 1, 0);
        dslash_spinor_local(local_para->local_ferm[2], local_para->local_link[0], local_para->local_ferm[0], 0, e_forward,NULL);
        //t backward
        dma_pull(local_para->local_ferm[0], src SLICE_0(i-1), sizeof(spinor_local),1,0);
        dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[0], 0, e_backward,NULL);

        /*int dma_handle_ =*/ dma_push(local_para->local_ferm[1], dst SLICE_0(i), sizeof(spinor_local),1,0);
       // dma_handle = dma_pull_async(local_para->local_link0, link[2] SLICE_0(i-1),sizeof(su3_local), 1, 0);

        spinor_local_copy(local_para->local_ferm[1], local_para->local_ferm[2]);
        //dma_wait(dma_handle_);
    }
    //if(local_para->thread_id==14) print_spinor_vec(local_para->local_ferm[1][31][1]);
    // z
    
    //dma_wait(dma_handle);
    dma_pull(local_para->local_link, link[2] SLICE_0(0),sizeof(su3_local), 1, 0);
    dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0],local_para->local_ferm[0], 1, e_forward,local_para->local_ferm[2]);
    dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[0], 1, e_backward,local_para->local_ferm[2]);
    // y
    dma_pull(local_para->local_link, link[1] SLICE_0(0), sizeof(su3_local), 1, 0);
    dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[0], 2, e_forward,NULL);
    dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[0], 2, e_backward,NULL);
    // x
    dma_pull(local_para->local_link, link[0] SLICE_0(0), sizeof(su3_local), 1, 0);
    dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[0], 3, e_forward,local_para->local_ferm[2]);
    dslash_spinor_local_acc(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[0], 3, e_backward,local_para->local_ferm[2]);
    //if(local_para->thread_id==0) print_spinor_vec(local_para->local_ferm[1][31][0]);
    dma_push(local_para->local_ferm[1], dst SLICE_0(0), sizeof(spinor_local), 1, 0);
}
void push_neighbour(t_dir dir,Para* local_para, int dim){
    void* dst = NULL;
    switch (dim)
    {
    case 0:
        dst = local_para->mtransfer->data_send_recv[0][dir][dim].slt[0][local_para->thread_id];
        dma_push(local_para->local_ferm[1],dst,sizeof(spinor_local),1,0);
        break;
    case 1:
        dst = local_para->mtransfer->data_send_recv[0][dir][dim].slz[0][local_para->thread_id];
        dma_push(local_para->local_ferm[1],dst,sizeof(spinor_local),1,0);
        break;
    case 2:
        dst = local_para->mtransfer->data_send_recv[0][dir][dim].sly[0][local_para->thread_id];
        dma_push(local_para->local_ferm[1],dst,sizeof(spinor_local)/2,1,0);
        break;
    case 3:
        dst = local_para->mtransfer->data_send_recv[0][dir][dim].slx[0][local_para->thread_id];
        dma_push(local_para->local_ferm[1],dst,sizeof(spinor_local)/2,1,0);
    default:
        break;
    }
    
}
int pull_neighbour_async(spinor_local dst,Para* local_para, int dim,t_dir dir){
    void* src = NULL;
    int dma_handle;
    switch (dim)
    {
    case 0:
        src = local_para->mtransfer->data_send_recv[1][dir][dim].slt[0][local_para->thread_id];
        dma_handle = dma_pull_async(dst,src,sizeof(spinor_local),1,0);
        break;
    case 1:
        src = local_para->mtransfer->data_send_recv[1][dir][dim].slz[0][local_para->thread_id];
        dma_handle = dma_pull_async(dst,src,sizeof(spinor_local),1,0);
        break;
    case 2:
        src = local_para->mtransfer->data_send_recv[1][dir][dim].sly[0][local_para->thread_id];
        dma_handle = dma_pull_async(dst,src,sizeof(spinor_local)/2,1,0);
        break;
    case 3:
        src = local_para->mtransfer->data_send_recv[1][dir][dim].slx[0][local_para->thread_id];
        dma_handle = dma_pull_async(dst,src,sizeof(spinor_local)/2,1,0);
    default:
        break;
    }

    return dma_handle;
}
void push_forward_to_cpu(Para* local_para, int dim){
    void* dst = NULL;
    switch (dim)
    {
    case 0:
        dst = local_para->mtransfer->data_send_recv[1][e_forward][dim].slt[0][local_para->thread_id];
        dma_push(local_para->local_ferm[1],dst,sizeof(spinor_local),1,0);
        break;
    case 1:
        dst = local_para->mtransfer->data_send_recv[1][e_forward][dim].slz[0][local_para->thread_id];
        dma_push(local_para->local_ferm[1],dst,sizeof(spinor_local),1,0);
        break;
    case 2:
        dst = local_para->mtransfer->data_send_recv[1][e_forward][dim].sly[0][local_para->thread_id];
        dma_push(local_para->local_ferm[1],dst,sizeof(spinor_local)/2,1,0);
        break;
    case 3:
        dst = local_para->mtransfer->data_send_recv[1][e_forward][dim].slx[0][local_para->thread_id];
        dma_push(local_para->local_ferm[1],dst,sizeof(spinor_local)/2,1,0);
    default:
        break;
    }
    
}

/*void pull_data_x(spinor_field src,Para* local_para){
    typedef double spinor_field_[NT][NZ][NY][NX/16][3][4][2][16];
    spinor_field_* sf = (spinor_field_*) src;
    double local_ferm[2][3][4][2][16];
    int i,j,k,a,b,c;
    double* d = (double*)(local_para->mtransfer->x_data[0][local_para->thread_id]);
    for(i=0;i<16;++i){
        for(j=0;j<32;++j){
            for(a=0;a<3;++a)
                for(b=0;b<4;++b)
                    for(c=0;c<2;++c){
                        local_ferm[j/16][a][b][c][j%16] = sf[0][local_para->thread_id][i][j][0][a][b][c][0];
                    }
        }
        dma_push(&local_ferm,d+32*3*4*2*i,sizeof(local_ferm),1,0);
    }
    dma_pull(local_para->local_ferm[0],local_para->mtransfer->x_data[0][local_para->thread_id],sizeof(spinor_local)/2,1,0);
}*/

void dma32(void* dst, void* src,t_dir dir){
    double* ddst = (double*) dst;
    double* dsrc = (double*) src + (dir == e_forward? 0 : 15);
    for(int iz = 0; iz < NZ; ++iz){
        for(int i = 0; i < 24; ++i)
            dma_pull(ddst+24*32*iz+16*i,dsrc+24*32*32*iz+16*i,sizeof(double),16,NX/16*sizeof(spinor_vec)-sizeof(double));
        
        for(int i = 0; i < 24; ++i)
            dma_pull(ddst+24*32*iz+16*24+16*i,dsrc+24*32*32*iz+16*24*32+16*i,sizeof(double),16,NX/16*sizeof(spinor_vec)-sizeof(double));
    }



}

void dslash_spinor_backward_neighbour_field(su3_field link, spinor_field src,Para* local_para,spinor_sliceX backward_spinor,spinor_sliceX forward_spinor){//处理需要发送的数据
    dma_pull(local_para->local_ferm[1],src SLICE_0(0), sizeof(spinor_local),1,0);
    push_neighbour(e_forward,local_para,0);

    dma_pull(local_para->local_ferm[1],src SLICE_1(0), sizeof(spinor_local),1,0);
    push_neighbour(e_forward,local_para,1);

    dma_pull(local_para->local_ferm[1],src SLICE_2(0),sizeof(spinor_vec)*NX/16,NZ,(NY-1)*NX/16*sizeof(spinor_vec));
    push_neighbour(e_forward,local_para,2);

    double* gsm_mem = dsp_comm;
    //dma32(local_para->local_ferm[1], src SLICE_3(0),e_forward);
    //dma_pull_sg(local_para->local_ferm[1], src /*SLICE_3(0)*/,local_para->mtransfer->x_index_forward+local_para->thread_id*NZ*NY*3*4*2,sizeof(spinor_local)/2,1,0);
    dma_pull(local_para->local_ferm[1],gsm_mem+(NT+(local_para->thread_id))*NZ*NY*24,sizeof(spinor_local)/2,1,0);
    push_neighbour(e_forward,local_para,3);


    dma_pull(local_para->local_link, local_para->mtransfer->link_x[0] SLICE_3(0),sizeof(su3_local)/2,1,0);
    //dma32(local_para->local_ferm[0],src SLICE_3(NX-1),e_backward);
    //dma_pull_sg(local_para->local_ferm[0],src /*SLICE_3(0)*/,local_para->mtransfer->x_index_backward+local_para->thread_id*NZ*NY*3*4*2,sizeof(spinor_local)/2,1,0);
    //dma_pull(local_para->local_ferm[0],backward_spinor[local_para->thread_id],sizeof(spinor_local)/2,1,0);
    dma_pull(local_para->local_ferm[0],gsm_mem+(local_para->thread_id)*NZ*NY*24,sizeof(spinor_local)/2,1,0);
    int dma_handle1 = dma_pull_async(local_para->local_ferm[2],src SLICE_2(NY-1),sizeof(spinor_vec)*NX/16,NZ,(NY-1)*NX/16*sizeof(spinor_vec));
    dslash_spinor_local_noshift(local_para->local_ferm[1],local_para->local_link[0],local_para->local_ferm[0],3,e_backward);
    push_neighbour(e_backward,local_para,3);

    dma_pull(local_para->local_link,link[1] SLICE_2(NY-1),sizeof(su3_vec)*NX/16,NZ,(NY-1)*NX/16*sizeof(su3_vec));
    dma_wait(dma_handle1);    
    dma_handle1 = dma_pull_async(local_para->local_ferm[0], src SLICE_1(NZ-1),sizeof(spinor_local),1,0);
    dslash_spinor_local_noshift(local_para->local_ferm[1],local_para->local_link[0],local_para->local_ferm[2],2,e_backward);
    push_neighbour(e_backward,local_para,2);

    dma_pull(local_para->local_link,link[2] SLICE_1(NZ-1),sizeof(su3_local),1,0);
    dma_wait(dma_handle1);
    dma_handle1 = dma_pull_async(local_para->local_ferm[2], src SLICE_0(NT-1), sizeof(spinor_local), 1, 0);
    dslash_spinor_local_noshift(local_para->local_ferm[1],local_para->local_link[0],local_para->local_ferm[0],1,e_backward);
    push_neighbour(e_backward,local_para,1);



    dma_pull(local_para->local_link, link[3] SLICE_0(NT-1), sizeof(su3_local), 1, 0);
    dma_wait(dma_handle1);
    dslash_spinor_local_noshift(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[2], 0, e_backward);
    push_neighbour(e_backward,local_para,0);


}
void push_to_data_x(spinor_field dst,Para* local_para){
    typedef double sf_[NZ][NY][NX/16][3][4][2][16];
    sf_* sf = (sf_*) (dst[local_para->thread_id]);
    int i,j,k,a,b,c;
    for(i=0;i<16;++i)
        for(j=0;j<2;++j)
            for(a=0;a<3;++a)
                for(b=0;b<4;++b)
                    for(c=0;c<2;++c){
                        mov_to_svr_v16df(local_para->local_ferm[1][i][j][a][b][c]);
                        sf[0][i][j*16+0][0][a][b][c][0] += mov_from_svr0_df();
                        sf[0][i][j*16+1][0][a][b][c][0] += mov_from_svr1_df();
                        sf[0][i][j*16+2][0][a][b][c][0] += mov_from_svr2_df();
                        sf[0][i][j*16+3][0][a][b][c][0] += mov_from_svr3_df();
                        sf[0][i][j*16+4][0][a][b][c][0] += mov_from_svr4_df();
                        sf[0][i][j*16+5][0][a][b][c][0] += mov_from_svr5_df();
                        sf[0][i][j*16+6][0][a][b][c][0] += mov_from_svr6_df();
                        sf[0][i][j*16+7][0][a][b][c][0] += mov_from_svr7_df();
                        sf[0][i][j*16+8][0][a][b][c][0] += mov_from_svr8_df();
                        sf[0][i][j*16+9][0][a][b][c][0] += mov_from_svr9_df();
                        sf[0][i][j*16+10][0][a][b][c][0] += mov_from_svr10_df();
                        sf[0][i][j*16+11][0][a][b][c][0] += mov_from_svr11_df();
                        sf[0][i][j*16+12][0][a][b][c][0] += mov_from_svr12_df();
                        sf[0][i][j*16+13][0][a][b][c][0] += mov_from_svr13_df();
                        sf[0][i][j*16+14][0][a][b][c][0] += mov_from_svr14_df();
                        sf[0][i][j*16+15][0][a][b][c][0] += mov_from_svr15_df();
                    }
}
void dslash_spinor_forward_neighbour_field(spinor_field dst, su3_field link, spinor_field src,Para* local_para){
    dma_pull(local_para->local_link, local_para->mtransfer->link_x[0] SLICE_3(0),sizeof(su3_local)/2,1,0);
    int dma_handle2 = pull_neighbour_async(local_para->local_ferm[0],local_para, 3,e_forward);
    int dma_handle1 = pull_neighbour_async(local_para->local_ferm[2],local_para, 2,e_forward);
    dma_wait(dma_handle2);
    dslash_spinor_local_noshift(local_para->local_ferm[1],local_para->local_link[0],local_para->local_ferm[0],3,e_forward);
    //push_forward_to_cpu(local_para,3);
    double* gsm_mem = (double*)dsp_comm;
    dma_push(local_para->local_ferm[1],gsm_mem+(NT+(local_para->thread_id))*NZ*NY*24,sizeof(spinor_local)/2,1,0);
//group_barrier(barrier);
    dma_pull(local_para->local_link,link[1] SLICE_2(NY-1),sizeof(su3_vec)*NX/16,NZ,(NY-1)*NX/16*sizeof(su3_vec));
    dma_pull(local_para->local_ferm[1],dst SLICE_2(NY-1),NX/16*sizeof(spinor_vec),NZ,(NY-1)*NX/16*sizeof(spinor_vec));
    dma_wait(dma_handle1);
    dma_handle1 = pull_neighbour_async(local_para->local_ferm[0],local_para, 1,e_forward);
    //if(local_para->thread_id==15) print_spinor_vec(local_para->local_ferm[2][0][0]);
    dslash_spinor_local_acc_noshift(local_para->local_ferm[1],local_para->local_link[0],local_para->local_ferm[2],2,e_forward);
    //if(local_para->thread_id==15) print_spinor_vec(local_para->local_ferm[1][0][0]);
    dma_push(local_para->local_ferm[1],dst SLICE_2(NY-1),NX/16*sizeof(spinor_vec),NZ,(NY-1)*NX/16*sizeof(spinor_vec));
//group_barrier(barrier);

    dma_pull(local_para->local_link,link[2] SLICE_1(NZ-1),sizeof(su3_local),1,0);
    dma_pull(local_para->local_ferm[1],dst SLICE_1(NZ-1),sizeof(spinor_local),1,0);
    dma_wait(dma_handle1);
    dma_handle1 = pull_neighbour_async(local_para->local_ferm[2],local_para, 0,e_forward);
    dslash_spinor_local_acc_noshift(local_para->local_ferm[1],local_para->local_link[0],local_para->local_ferm[0],1,e_forward);
    dma_push(local_para->local_ferm[1],dst SLICE_1(NZ-1),sizeof(spinor_local),1,0);
//group_barrier(barrier);

    dma_pull(local_para->local_link, link[3] SLICE_0(NT-1), sizeof(su3_local), 1, 0);
    dma_pull(local_para->local_ferm[1],dst SLICE_0(NT-1),sizeof(spinor_local),1,0);
    dma_wait(dma_handle1);
    dma_handle1 = pull_neighbour_async(local_para->local_ferm[0],local_para,0,e_backward);
    dslash_spinor_local_acc_noshift(local_para->local_ferm[1], local_para->local_link[0], local_para->local_ferm[2], 0, e_forward);
    dma_push(local_para->local_ferm[1],dst SLICE_0(NT-1),sizeof(spinor_local),1,0);
//group_barrier(barrier);

    dma_pull(local_para->local_ferm[1],dst SLICE_0(0),sizeof(spinor_local),1,0);
    dma_wait(dma_handle1);
    dma_handle1 = pull_neighbour_async(local_para->local_ferm[2],local_para,1,e_backward);
    spinor_local_acc(local_para->local_ferm[1],local_para->local_ferm[0]);
    dma_push(local_para->local_ferm[1],dst SLICE_0(0),sizeof(spinor_local),1,0);
//group_barrier(barrier);

    dma_pull(local_para->local_ferm[1], dst SLICE_1(0),sizeof(spinor_local),1,0);
    dma_wait(dma_handle1);
    dma_handle1 = pull_neighbour_async(local_para->local_ferm[0],local_para,2,e_backward);
    spinor_local_acc(local_para->local_ferm[1],local_para->local_ferm[2]);
    dma_push(local_para->local_ferm[1],dst SLICE_1(0),sizeof(spinor_local),1,0);
//group_barrier(barrier);

    dma_pull(local_para->local_ferm[1],dst SLICE_2(0),NX/16*sizeof(spinor_vec),NZ,(NY-1)*NX/16*sizeof(spinor_vec));
    dma_wait(dma_handle1);
    dma_handle1 = pull_neighbour_async(local_para->local_ferm[2],local_para,3,e_backward);
    spinor_local_acc(local_para->local_ferm[1],local_para->local_ferm[0]);
    dma_push(local_para->local_ferm[1],dst SLICE_2(0),NX/16*sizeof(spinor_vec),NZ,(NY-1)*NX/16*sizeof(spinor_vec));
//group_barrier(barrier);

    dma_wait(dma_handle1);
    dma_push(local_para->local_ferm[2],gsm_mem+(local_para->thread_id)*NZ*NY*24,sizeof(spinor_local)/2,1,0);
}

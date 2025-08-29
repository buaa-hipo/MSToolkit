#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>

#include "master.h"

#include "instrument/instrumented_func_host.h"
#include "record/mt_callback_defs.h"


#define PRINT(__STR__) \
    if (mpi_rank == 0) printf("\n%s\n", __STR__);
#define N_spinor_buf 26
#define N_su3_buf    1
double dslash_send = 0.0, dslash_main = 0.0, dslash_recv = 0.0, dslash_recv_acc = 0.0, re1 = 0.0, re2 = 0.0,
       mpi_time = 0.0, allre1 = 0.0, allre2 = 0.0, dslash_all = 0.0;
int                     cluster_id;
int                     group_id;
void*                   global_buf;
spinor_field*           global_spinor_buf;
su3_field*              global_link_buf;
extern master_transfer* mtransfer;
extern double*          reduce_buffer;
extern int              mpi_rank;
extern MPI_Comm         cart_comm;

char* align_f4(char* p) {
    while ((size_t)p % N_align) ++p;
    return p;
}

void set_spinor_source(spinor_field* s, int color, int spin) {
    (*s)[0][0][0][0][color][spin][0][0] = 1.0;
    (*s)[0][0][0][0][color][spin][1][0] = 0.0;
}

void mtransfer_conf_init() {
    mtransfer->link_x =
        (su3_sliceX*)instrumented_hthread_malloc(cluster_id, sizeof(FLOAT) * NY * NT * NZ * 2 * 3 * 2 + N_align, HT_MEM_RW);
    mtransfer->link_x = (su3_sliceX*)align_f4((char*)(mtransfer->link_x));
    // mtransfer->x_data = (spinor_sliceX *)hthread_malloc(cluster_id,sizeof(FLOAT)*NT*NZ*NY*3*4*2+N_align,HT_MEM_RW);
    // mtransfer->x_data = (spinor_sliceX*)align_f4((char*)(mtransfer->x_data));
    // mtransfer->x_index_backward = (int*)hthread_malloc(cluster_id,sizeof(int)*NT*NZ*NY*3*4*2+N_align,HT_MEM_RW);
    // mtransfer->x_index_forward = (int*)hthread_malloc(cluster_id,sizeof(int)*NT*NZ*NY*3*4*2+N_align,HT_MEM_RW);
    auto ptr = mtransfer->link_x;
    for (int i = 0; i < NT; ++i)
        for (int j = 0; j < NZ; ++j)
            for (int a = 0; a < 2; ++a)
                for (int b = 0; b < 3; ++b)
                    for (int c = 0; c < 2; ++c)
                        for (int k = 0; k < NY; ++k) {
                            ptr[0][i][j][k / 16][a][b][c][k % 16] =
                                global_link_buf[0][0][i][j][k][NX / 16 - 1][a][b][c][15];
                        }
    // auto p1 = mtransfer->x_index_forward, p2 = mtransfer->x_index_backward;
    // for(int it = 0; it < NT; ++it){
    //     for(int iz = 0; iz < NZ; ++iz)
    // 	    for(int iy = 0; iy < NY/16; ++iy)
    // 		    for(int a = 0; a < 3; ++a)
    // 			    for(int b = 0; b < 4; ++b)
    // 				    for(int c = 0; c < 2; ++c)
    //                         for(int iiy = 0; iiy < 16; ++iiy){
    // 					        *p1 = (it*NZ*NY*NX*3*4*2 + iz*NY*NX*3*4*2 +
    // (iy*16+iiy)*3*4*2*NX+a*4*2*16+b*2*16+c*16)*8;
    //                             //printf("%d ",*p1);
    //                             ++p1;
    //                             *p2 = (it*NZ*NY*NX*3*4*2 + iz*NY*NX*3*4*2 +
    //                             (iy*16+iiy)*3*4*2*NX+a*4*2*16+b*2*16+c*16+3*4*2*16+15)*8;
    //                             ++p2;
    // 				        }
    // return;
    //}
}

void init() {
    // PRINT("mpi init.....");
    MPI_Init(NULL, NULL);
    mpi_init();
    mpiio_init();
    PRINT("reduce init.....")
    reduce_init();
    PRINT("data init.....");
    global_buf = instrumented_hthread_malloc(
        cluster_id, N_spinor_buf * sizeof(spinor_field) + N_su3_buf * sizeof(su3_field) + N_align, HT_MEM_RW);
    if (global_buf == NULL) {
        PRINT("Error to alloc memory!");
        return;
    }
    memset(global_buf, 0, N_spinor_buf * sizeof(spinor_field) + N_su3_buf * sizeof(su3_field) + N_align);
    global_spinor_buf = (spinor_field*)align_f4((char*)global_buf);
    global_link_buf   = (su3_field*)((char*)global_spinor_buf + sizeof(spinor_field) * N_spinor_buf);
    PRINT("su3 init.....");
    // mpi_read_conf("test16x16x32x32.cfg",global_link_buf[0]);
    read_conf("test16x16x32x32.cfg", global_link_buf);
    PRINT("mtransfer init.....");
    mtransfer_conf_init();
    if (instrumented_hthread_dat_load(cluster_id, "dsp_kernel.dat") != HT_SUCCESS) {
        PRINT("Failed to load dat file!");
        return;
    }
    group_id = instrumented_hthread_group_create2(cluster_id, NThreads);
    if (group_id < 0) {
        PRINT("Error to create group!");
        return;
    }
    PRINT("init finished.");
}

int main(int argc, char** argv) {
    try {
        init();
        int p;
        MPI_Comm_size(comm, &p);
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 3; ++j) {
                set_spinor_source(&global_spinor_buf[2 + 3 * i + j], j, i);
                set_spinor_source(&global_spinor_buf[14 + 3 * i + j], j, i);
                auto start = std::chrono::high_resolution_clock::now();
                mr_solver(global_spinor_buf[i], global_link_buf[0], global_spinor_buf[i + 2], global_spinor_buf[i + 1],
                          global_spinor_buf[i + 14]);
                auto   end       = std::chrono::high_resolution_clock::now();
                double time_used = std::chrono::duration<double>(end - start).count();
                double ave_time;
                MPI_Allreduce(&time_used, &ave_time, 1, MPI_DOUBLE, MPI_SUM, cart_comm);
                ave_time = ave_time / p;
                if (mpi_rank == 0) printf("%lf\n", ave_time);
                // write_ferm_field(global_spinor_buf[i],"result16x16x32x32");
                break;
            }
            break;
        }
        double old_times[7];
        double ave_times[7];
        old_times[0] = dslash_send;
        old_times[1] = allre1;
        old_times[2] = allre2;
        old_times[3] = dslash_main;
        old_times[4] = dslash_recv;
        old_times[5] = mpi_time;
        old_times[6] = dslash_all;
        MPI_Allreduce(old_times, ave_times, 7, MPI_DOUBLE, MPI_SUM, cart_comm);
        for (int i = 0; i < 7; ++i) ave_times[i] /= p;
        if (mpi_rank == 0)
            printf("dslash_send:%lf\nallreduce1:%lf\nallreduce2:%lf\ndslash_main:%lf\ndslash_recv:%lf\nmpi:%lf\ndslash_"
                   "all:%lf\n",
                   ave_times[0], ave_times[1], ave_times[2], ave_times[3], ave_times[4], ave_times[5], ave_times[6]);
    } catch (MPI::Exception e) {
        char name[256];
        int  len = 256;
        MPI_Get_processor_name(name, &len);
        printf("node %s error!\n", name);
        MPI_Abort(comm, -2);
        // return 0;
    }

    instrumented_hthread_free(global_buf);
    instrumented_hthread_free(mtransfer);
    instrumented_hthread_free(reduce_buffer);

    instrumented_hthread_dev_close(cluster_id);
    MPI_Finalize();

    return 0;
}

#include<chrono>
#include "master.h"

#include "instrument/instrumented_func_host.h"
#include "record/mt_callback_defs.h"


extern MPI_Comm cart_comm;
double* reduce_buffer;
spinor_sliceX* dslash_backward_spinor, *dslash_forward_spinor;
extern double residual_square, coef_g[2];
extern int group_id,cluster_id;
extern double re1,re2,allre1,allre2;
void allreduce1(){
    double src[3] = {0,0,0};
    double dst[3];
    for(int i = 0; i < NThreads; ++i){
        for(int j = 0; j < 3; ++j){
            src[j] += reduce_buffer[i*3+j];
        }
    }
    MPI_Allreduce(src,dst,3,MPI_DOUBLE,MPI_SUM,cart_comm);
    residual_square = dst[0];
    coef_g[0] = dst[1];
    coef_g[1] = dst[2];
}
void allreduce2(){
    double src = 0.0;
    double dst;
    for(int i = 0; i < NThreads; ++i){
        src+=reduce_buffer[i];
    }
    MPI_Allreduce(&src,&dst,1,MPI_DOUBLE,MPI_SUM,cart_comm);
    residual_square = dst;
}
void reduce1(spinor_field dst,spinor_field src){
    uint64_t args[3];
    args[0] = (uint64_t) dst;
    args[1] = (uint64_t) src;
    args[2] = (uint64_t) reduce_buffer;
    
    auto start = std::chrono::high_resolution_clock::now();
    instrumented_hthread_group_exec(group_id,"dsp_reduce1",0,3,args);
    instrumented_hthread_group_wait(group_id);
    auto end = std::chrono::high_resolution_clock::now();
    re1+=std::chrono::duration<double>(end-start).count();
    //printf("reduce1 time: %lf s\n",std::chrono::duration<double>(end-start).count());
    start = std::chrono::high_resolution_clock::now();
    allreduce1();
    end = std::chrono::high_resolution_clock::now();
    allre1+=std::chrono::duration<double>(end-start).count();

}

void reduce2(double* coef, spinor_field dst, spinor_field src, spinor_field aux){
    uint64_t args[7];
    args[0] = (uint64_t)dst;
    args[1] = (uint64_t)src;
    args[2] = (uint64_t)aux;
    args[3] = (uint64_t)coef;
    args[4] = (uint64_t)reduce_buffer;

    args[5] = (uint64_t) dslash_backward_spinor;
    args[6] = (uint64_t) dslash_forward_spinor;

    auto start = std::chrono::high_resolution_clock::now();
    instrumented_hthread_group_exec(group_id,"dsp_reduce2",0,7,args);
    instrumented_hthread_group_wait(group_id);
    auto end = std::chrono::high_resolution_clock::now();
    re2+=std::chrono::duration<double>(end-start).count();
    //printf("reduce2 time: %lf s\n",std::chrono::duration<double>(end-start).count());
    start = std::chrono::high_resolution_clock::now();
    allreduce2();
    end = std::chrono::high_resolution_clock::now();
    allre2+=std::chrono::duration<double>(end-start).count();
}

void reduce_init(){
    reduce_buffer = (double*) instrumented_hthread_malloc(cluster_id,NThreads*3*sizeof(double),HT_MEM_RW);
    //
    dslash_backward_spinor = (spinor_sliceX*) instrumented_hthread_malloc(cluster_id,sizeof(spinor_sliceX),HT_MEM_RW);
    dslash_forward_spinor = (spinor_sliceX*) instrumented_hthread_malloc(cluster_id,sizeof(spinor_sliceX),HT_MEM_RW);
}

#include "dsp.h"

__gsm__ DSP_COMM dsp_comm[NThreads][2];
volatile int dsp_comm_flag[NThreads][2];




void dsp_para_init(Para* p,master_transfer* mtransfer){
    p->thread_id = get_thread_id();
    p->local_link = vector_malloc(sizeof(su3_local));
    p->local_ferm = vector_malloc(sizeof(spinor_local)*3);
    p->zero_vec = vector_malloc(sizeof(lvector double));
    p->zero_vec[0] = 0.0;
    p->mtransfer = mtransfer;
}
void dsp_para_free(Para* p){
    vector_free(p->local_link);
    vector_free(p->local_ferm);
    vector_free(p->zero_vec);
}

__global__ void init_gsm(spinor_sliceX backward_spinor,spinor_sliceX forward_spinor){
    Para local_para;
    dsp_para_init(&local_para,NULL);
    double* d = (double*) dsp_comm;
    dma_pull(local_para.local_ferm[0],backward_spinor[local_para.thread_id],sizeof(spinor_sliceX)/NT,1,0);
    dma_push(local_para.local_ferm[0],d+(local_para.thread_id)*NZ*NY*24,sizeof(spinor_sliceX)/NT,1,0);

    dma_pull(local_para.local_ferm[0],forward_spinor[local_para.thread_id],sizeof(spinor_sliceX)/NT,1,0);
    dma_push(local_para.local_ferm[0],d+(NT+local_para.thread_id)*NZ*NY*24,sizeof(spinor_sliceX)/NT,1,0);
    
    dsp_para_free(&local_para);
}

__global__ void dslash_backward_neighbour(su3_field link, spinor_field src, master_transfer* mtransfer,spinor_sliceX backward_spinor,spinor_sliceX forward_spinor){
    Para local_para;
    dsp_para_init(&local_para,mtransfer);
    dslash_spinor_backward_neighbour_field(link,src,&local_para,backward_spinor,forward_spinor);
    dsp_para_free(&local_para);
}

__global__ void dslash_main(spinor_field dst, su3_field link, spinor_field src){
    Para local_para;
    dsp_para_init(&local_para,NULL);
    dsp_comm_flag[local_para.thread_id][0] = 1;
    dsp_comm_flag[local_para.thread_id][1] = 1;
    dslash(dst,link,src,&local_para);
    dsp_para_free(&local_para);
}

__global__ void dslash_forward_neighbour(spinor_field dst, su3_field link, spinor_field src, master_transfer* mtransfer){
    Para local_para;
    dsp_para_init(&local_para,mtransfer);
    dslash_spinor_forward_neighbour_field(dst,link,src,&local_para);
    dsp_para_free(&local_para);
}

__global__ void dsp_reduce1(spinor_field dst, spinor_field src, double* reduce_buffer){
    Para local_para;
    dsp_para_init(&local_para,NULL);
    local_para.flag_coef = 1;
    local_para.flag_residual = 1;
    spinor_field_acc_linear_combine_real(dst,src,-0.5,1.0/(2*kappa),&local_para);
    reduce_buffer[local_para.thread_id*3] = local_para.residual_square;
    reduce_buffer[local_para.thread_id*3+1] = local_para.coef_g[0];
    reduce_buffer[local_para.thread_id*3+2] = local_para.coef_g[1];
    dsp_para_free(&local_para);
}

__global__ void dsp_reduce2(spinor_field dst, spinor_field src, spinor_field aux,double *coef_, double* reduce_buffer,double* backward_spinor, double* forward_spinor){
    Para local_para;
    dsp_para_init(&local_para,NULL);
    
    double coef[2];
    coef[0] = coef_[0];
    coef[1] = coef_[1];
    local_para.flag_coef = 0;
    local_para.flag_residual = 0;
    spinor_field_acc_complex_mul(dst,coef,src,&local_para,NULL,NULL);
    coef[0] = -coef[0];
    coef[1] = -coef[1];
    local_para.flag_residual = 1;
    
    spinor_field_acc_complex_mul(src,coef,aux,&local_para,backward_spinor,forward_spinor);
    reduce_buffer[local_para.thread_id] = local_para.residual_square;
    dsp_para_free(&local_para);
}

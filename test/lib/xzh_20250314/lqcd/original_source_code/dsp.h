#ifndef LQCD_DSP_H
#define LQCD_DSP_H
#include "global.h"

typedef struct DSP_COMM{
    //volatile int empty ;
    FLOAT data[NY*NX*3*4*2];
}DSP_COMM;
typedef struct Parameter{
    int thread_id;
    su3_local* local_link;
    spinor_local* local_ferm;
    lvector double* zero_vec;
    master_transfer* mtransfer;
    double residual_square;
    double coef_g[2];
    int flag_residual;
    int flag_coef;
} Para;

extern DSP_COMM dsp_comm[16][2];
//dsp_dslash_spinor_field.c
void dslash(spinor_field dst, su3_field link, spinor_field src, Para* local_para);
void dslash_spinor_backward_neighbour_field(su3_field link, spinor_field src,Para* local_para,spinor_sliceX backward_spinor,spinor_sliceX forward_spinor);
void dslash_spinor_forward_neighbour_field(spinor_field dst, su3_field link, spinor_field src,Para* local_para);

//dsp_dslash_spinor_local.c
void dslash_spinor_local(spinor_local dst, su3_local link, spinor_local src, int dim, t_dir dir,spinor_local tmp);
void dslash_spinor_local_acc(spinor_local dst, su3_local link, spinor_local src, int dim, t_dir dir, spinor_local tmp);
void dslash_spinor_local_noshift(spinor_local dst, su3_local link, spinor_local src, int dim, t_dir dir);
void dslash_spinor_local_acc_noshift(spinor_local dst, su3_local link, spinor_local src, int dim, t_dir dir);

//dsp_spinor_field.c
void spinor_field_zero(spinor_field dst,Para* local_para);
void spinor_field_acc_linear_combine_real(spinor_field dst, spinor_field src, double coef_dst, double coef_s,Para* local_para);
void spinor_field_acc_complex_mul(spinor_field dst, const double coef[2], spinor_field src,Para* local_para,spinor_sliceX backward_spinor, spinor_sliceX forward_spinor);
void spinor_field_acc_sub(spinor_field dst,spinor_field src,Para* local_para);

//dsp_dma.c
void dma_pull(void* local_dst, void* host_src, int unit_size, int step_num, int step_length);
int dma_pull_async(void* local_dst, void* host_src, int unit_size, int step_num, int step_length);
void dma_push(void* local_src, void* host_dst, int unit_size, int step_num, int step_length);
int dma_push_async(void* local_src, void* host_dst, int unit_size, int step_num, int step_length);
void dma_pull_sg(void* local_dst, void* host_src, int* idx, int unit_size, int step_num, int step_length);

//dsp_spinor_local.c
void spinor_local_zero(spinor_local d);
void spinor_local_copy(spinor_local d, spinor_local s);
void spinor_local_acc(spinor_local d, spinor_local s);
void spinor_local_shift(spinor_local d, spinor_local s, int dim, t_dir dir);

//dsp_dslash_spinor.c
void spinor_acc_gamma0_dag_mul(spinor_vec dst, su3_vec link, spinor_vec src);
void spinor_acc_gamma0_mul(spinor_vec dst, su3_vec link, spinor_vec src);
void spinor_acc_gamma1_dag_mul(spinor_vec dst, su3_vec link, spinor_vec src);
void spinor_acc_gamma1_mul(spinor_vec dst, su3_vec link, spinor_vec src);
void spinor_acc_gamma2_dag_mul(spinor_vec dst, su3_vec link, spinor_vec src);
void spinor_acc_gamma2_mul(spinor_vec dst, su3_vec link, spinor_vec src);
void spinor_acc_gamma3_dag_mul(spinor_vec dst, su3_vec link, spinor_vec src);
void spinor_acc_gamma3_mul(spinor_vec dst, su3_vec link, spinor_vec src);

void print_spinor(spinor_field s, Para* local_para);
void print_spinor_vec(spinor_vec sv);
void print_su3_vec(su3_vec sv);
void print_lvector_double(lvector double lv);

#endif

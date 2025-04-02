#ifndef _LQCD_GLOBAL_H
#define _LQCD_GLOBAL_H

// Change scale here
// total scale
#define N0 32
#define N1 32
#define N2 32
#define N3 32
// per dsp cluster scale
#define NT 16
#define NZ 16
#define NY 32
#define NX 32
#define VOLUME (NT*NZ*NY*NX)
#define NThreads 16
#define	kappa		0.135
#define	max_iter	1000
#define	tolerance	1.0e-12
#define	omega		1.1
typedef double FLOAT;
#ifdef LQCD_DSP_H
#include<compiler/m3000.h>
#include<hthread_device.h>
typedef lvector FLOAT su3_vec[2][3][2];
typedef lvector FLOAT spinor_vec[3][4][2];
#else 
typedef FLOAT su3_vec[2][3][2][16];
typedef FLOAT spinor_vec[3][4][2][16];
#endif
typedef su3_vec su3_field[4][NT][NZ][NY][NX/16];
typedef spinor_vec spinor_field[NT][NZ][NY][NX/16];
typedef su3_vec su3_local[NY][NX/16];
typedef spinor_vec spinor_local[NY][NX/16];
typedef spinor_vec spinor_sliceX[NT][NZ][NY/16]; 
typedef spinor_vec spinor_sliceY[NT][NZ][NX/16];
typedef spinor_vec spinor_sliceZ[NT][NY][NX/16];
typedef spinor_vec spinor_sliceT[NZ][NY][NX/16];
typedef su3_vec su3_sliceX[NT][NZ][NY/16];

typedef enum{
    e_backward = 0,
    e_forward = 1
} t_dir;
typedef struct master_transfer
{
    union{
        spinor_sliceT* slt;
        spinor_sliceZ* slz;
        spinor_sliceY* sly;
        spinor_sliceX* slx;
        FLOAT* ptr;
    }data_send_recv[2][2][4];
    su3_sliceX *link_x;
    //spinor_sliceX* x_data;//由CPU组织的发送的backward方向x维度的数据（还需要经过DSP计算）
    //int* x_index_backward;
    //int* x_index_forward;
}master_transfer;
#endif

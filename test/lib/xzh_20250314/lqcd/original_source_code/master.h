#ifndef LQCD_MASTER_H
#define LQCD_MASTER_H
#include "global.h"
#include <hthread_host.h>
#include "mpi.h"
#include "omp.h"

#include <cstdio>

#define N_align 128
#define comm MPI_COMM_WORLD
typedef struct{
    int cart_id;        //mpi id
	int* coords;	    //坐标
	int dst_cart_id;	//发送数据的目标mpi id
	int dst_coords[4];  //发送数据的目标坐标
	FLOAT* buf;         //数据地址
	int num;            //数据量
	MPI_Datatype dtype;
	int tag;
	MPI_Request *req;
}Data_send;

typedef struct{
    int cart_id;
	int* coords;
	int src_cart_id;	
	int src_coords[4];
	FLOAT* buf;
	int num;
	MPI_Datatype dtype;
	int tag;
	MPI_Request *req;
    MPI_Status *sta;
}Data_recv;
//master_read.cpp
void write_ferm_field(spinor_field s,char* filename);
void read_conf(char filename[],su3_field* u);
//master_solver.cpp
void mr_solver(spinor_field dst, su3_field link, spinor_field src, spinor_field aux, spinor_field src0);
void print_spinor(spinor_field s);

//master_transfer.cpp
void send_data(t_dir dir);
void recv_data(t_dir dir);
void mpi_wait(t_dir dir);
void mpi_init();

//master_dslash_spinor_field.cpp
void dslash(spinor_field dst, su3_field link, spinor_field src);

//master_reduce.cpp
void reduce1(spinor_field dst,spinor_field src);
void reduce2(double* coef, spinor_field dst, spinor_field src, spinor_field aux);
void reduce_init();

//master_io.cpp
void mpiio_init();
void mpi_read_conf(char* filename,su3_field data);

char *align_f4(char *p);

#define CHECK_MPI_ERROR(mpi_routine)                                                                                  \
    do {                                                                                                              \
        int _mpiret = mpi_routine;                                                                                    \
        if (_mpiret != MPI_SUCCESS) {                                                                                 \
            fprintf(stderr, "%s:%d macro: MPI Error: function " #mpi_routine " failed with ret=%d.\n", __FILE__, \
                    __LINE__, _mpiret);                                                                      \
            char name[256];                                                                                           \
            int  len = 256;                                                                                           \
            MPI_Get_processor_name(name, &len);                                                                       \
            printf("node %s error!\n", name);                                                                         \
            MPI_Abort(comm, -2);                                                                                      \
        }                                                                                                             \
    } while (0);


#endif

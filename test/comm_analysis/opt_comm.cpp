#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define LARGE_N 50000000
#define TOO_FREQ 100000
int rank=0, size=1;

void frequent_comm_opt() {
    int xs = 0;
    int xr;
    MPI_Status status;
    if(rank==0) {
        printf("Frequent Communication: \n");
    }
    MPI_Barrier(MPI_COMM_WORLD);
    double ts = MPI_Wtime();
    int* xs_array = new int[TOO_FREQ];
    int* xr_array = new int[TOO_FREQ];
    for(int i=0; i<TOO_FREQ; ++i) {
        xs_array[i] = 0;
    }
    MPI_Request* req = new MPI_Request[size];
    for(int j=0; j<size; ++j) {
        if(rank!=j) {
            MPI_Isend(xs_array, TOO_FREQ, MPI_INT, j, 200, MPI_COMM_WORLD, &req[j]);
        }
    }
    for(int j=0; j<size; ++j) {
        if(rank!=j) {
            MPI_Recv(xr_array, TOO_FREQ, MPI_INT, j, 200, MPI_COMM_WORLD, &status);
        }
    }
    for(int j=0; j<size; ++j) {
        if(rank!=j) {
            MPI_Wait(&req[j], &status);
        }
    }
    delete[] xs_array;
    delete[] xr_array;
    MPI_Barrier(MPI_COMM_WORLD);
    double te = MPI_Wtime();
    if(rank==0) {
        printf("++ time %lf sec\n", te-ts);
    }
}

void long_wait_opt() {
    if(rank==0) {
        printf("Long Wait: \n");
    }
    MPI_Barrier(MPI_COMM_WORLD);
    double ts = MPI_Wtime();
    unsigned long long int a = 0;
    /******* Inefficient code ***********
    if(rank==0) {
        for(int i=0; i<LARGE_N; ++i) {
            a+=sin(i)*cos(i)*sqrt(i);
        }
    } 
    *************************************/
   /******* Optimized code **************/
    unsigned long long int* b;
    MPI_Status status;
    MPI_Request* req = new MPI_Request[size];
    if(rank==0) {
        b = (unsigned long long int*)malloc(size*sizeof(unsigned long long int));
        for(int i=1; i<size; ++i) {
            MPI_Irecv(&b[i], 1, MPI_DOUBLE, i, 200, MPI_COMM_WORLD, &req[i]);
        }
    }
    int per_rank = LARGE_N/size;
    for(int i=rank*per_rank; i<(rank+1)*per_rank; ++i) {
        a+=sin(i)*cos(i)*sqrt(i);
    }
    if(rank==0) {
        for(int i=1; i<size; ++i) {
            MPI_Wait(&req[i], &status);
            a += b[i];
        }
    } else {
        MPI_Send(&a, 1, MPI_DOUBLE, 0, 200, MPI_COMM_WORLD);
    }
    /*************************************/
    MPI_Barrier(MPI_COMM_WORLD);
    double te = MPI_Wtime();
    if(rank==0) {
        printf("++ time %lf sec, result: %llu\n", te-ts, a);
    }
}

void large_comm_opt() {
    if(rank==0) {
        printf("Large Communication: \n");
    }
    MPI_Barrier(MPI_COMM_WORLD);
    double ts = MPI_Wtime();
    double* data_loc = new double[LARGE_N];
    double* data_up  = new double[LARGE_N];
    double* data_down= new double[LARGE_N];
    double data = 0;
    for(int i=0; i<LARGE_N; ++i) {
        data_loc[i] = i;
    }
    MPI_Status status;
    MPI_Request recv_req[2], send_req[2];
    int up = rank+1;
    if(up==size) up = 0;
    int down = rank-1;
    if(down<0) down=size-1;

    double ts_0 = MPI_Wtime();
    MPI_Isend(data_loc, LARGE_N, MPI_DOUBLE, up, 200, MPI_COMM_WORLD, &send_req[0]);
    MPI_Isend(data_loc, LARGE_N, MPI_DOUBLE, down, 200, MPI_COMM_WORLD, &send_req[1]);
    MPI_Irecv(data_up, LARGE_N, MPI_DOUBLE, up, 200, MPI_COMM_WORLD, &recv_req[0]);
    MPI_Irecv(data_down, LARGE_N, MPI_DOUBLE, down, 200, MPI_COMM_WORLD, &recv_req[1]);
    /* Large comm and unrelated calc can be overlapped */
    // do something unrelated to the communication
    double ts_c = MPI_Wtime();
    for(int i=0; i<LARGE_N; ++i) {
        data += sin(i)*cos(i)*sqrt(i);
    }
    double te_c = MPI_Wtime();

    MPI_Wait(&recv_req[0], &status);
    MPI_Wait(&recv_req[1], &status);
    MPI_Wait(&send_req[0], &status);
    MPI_Wait(&send_req[1], &status);

    double te_0 = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    double te = MPI_Wtime();
    if(rank==0) {
        printf("++ time %lf sec, calc time %lf, mpi prev %lf, wait %lf; %lf, %lf, %lf\n", te-ts, te_c-ts_c, ts_c-ts_0, te_0-te_c, data_up[1], data_down[1], data);
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    long_wait_opt();
    large_comm_opt();
    frequent_comm_opt();
    MPI_Finalize();
}
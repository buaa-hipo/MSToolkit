#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define LARGE_N 50000000
#define TOO_FREQ 100000
int rank=0, size=1;

void frequent_comm() {
    int xs = 0;
    int xr;
    MPI_Status status;
    if(rank==0) {
        printf("Frequent Communication: \n");
    }
    MPI_Barrier(MPI_COMM_WORLD);
    double ts = MPI_Wtime();
    MPI_Request* req = new MPI_Request[size];
    for(int i=0; i<TOO_FREQ; ++i) {
        for(int j=0; j<size; ++j) {
            if(rank!=j) {
                MPI_Isend(&xs, 1, MPI_INT, j, 200, MPI_COMM_WORLD, &req[j]);
            }
        }
        for(int j=0; j<size; ++j) {
            if(rank!=j) {
                MPI_Recv(&xr, 1, MPI_INT, j, 200, MPI_COMM_WORLD, &status);
            }
        }
        for(int j=0; j<size; ++j) {
            if(rank!=j) {
                MPI_Wait(&req[j], &status);
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    double te = MPI_Wtime();
    if(rank==0) {
        printf("++ time %lf sec\n", te-ts);
    }
}

void long_wait() {
    if(rank==0) {
        printf("Long Wait: \n");
    }
    MPI_Barrier(MPI_COMM_WORLD);
    double ts = MPI_Wtime();
    unsigned long long int a = 0;
    if(rank==0) {
        for(int i=0; i<LARGE_N; ++i) {
            a+=sin(i)*cos(i)*sqrt(i);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    double te = MPI_Wtime();
    if(rank==0) {
        printf("++ time %lf sec, result: %llu\n", te-ts, a);
    }
}

void large_comm() {
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
    MPI_Request up_req, down_req;
    int up = rank+1;
    if(up==size) up = 0;
    int down = rank-1;
    if(down<0) down=size-1;

    double ts_0 = MPI_Wtime();
    MPI_Irecv(data_up, LARGE_N, MPI_DOUBLE, up, 200, MPI_COMM_WORLD, &up_req);
    MPI_Irecv(data_down, LARGE_N, MPI_DOUBLE, down, 200, MPI_COMM_WORLD, &down_req);
    MPI_Send(data_loc, LARGE_N, MPI_DOUBLE, up, 200, MPI_COMM_WORLD);
    MPI_Send(data_loc, LARGE_N, MPI_DOUBLE, down, 200, MPI_COMM_WORLD);

    MPI_Wait(&up_req, &status);
    MPI_Wait(&down_req, &status);

    // do something unrelated to the communication
    double ts_c = MPI_Wtime();
    for(int i=0; i<LARGE_N; ++i) {
        data += sin(i)*cos(i)*sqrt(i);
    }
    double te_c = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    double te = MPI_Wtime();
    if(rank==0) {
        printf("++ time %lf sec, calc time %lf, mpi %lf;%lf, %lf, %lf\n", te-ts, te_c-ts_c, ts_c-ts_0, data_up[1], data_down[1], data);
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    long_wait();
    large_comm();
    frequent_comm();
    MPI_Finalize();
}
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_ITER 1000
#define WORK_SIZE 100000

int rank;
int size;

void calc_workload(int N) {
    int* buffer = (int*)malloc(sizeof(int)*N);
    for(int i=0; i<N; ++i) {
        buffer[i] = (int)(sqrt(i)+i);
    }
    free(buffer);
}

void comm_workload(int N) {
    int* sbuff = (int*)malloc(sizeof(int)*N);
    int* rbuff = (int*)malloc(sizeof(int)*N);
    MPI_Request req;
    MPI_Status stat;
    int src, dst;
    if(rank==size-1) {
        dst = 0;
    } else {
        dst = rank + 1;
    }
    if(rank==0) {
        src = size-1;
    } else {
        src = rank-1;
    }
    MPI_Isend(sbuff, N, MPI_INT, dst, 100, MPI_COMM_WORLD, &req);
    MPI_Recv(rbuff, N, MPI_INT, src, 100, MPI_COMM_WORLD, &stat);
    MPI_Wait(&req, &stat);
    free(sbuff);
    free(rbuff);
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    // printf("Rank %d in Size %d\n", rank, size);
    for (int i=0; i<MAX_ITER; ++i) {
        calc_workload(WORK_SIZE);
        comm_workload(WORK_SIZE);
    } 
    MPI_Finalize();
    return 0;
}
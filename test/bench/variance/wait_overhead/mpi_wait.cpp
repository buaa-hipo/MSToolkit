#include <stdio.h>
#include <chrono>
#include <mpi.h>
#include <unistd.h>

#include <assert.h>

#define SMALL_COMM_SIZE 1
#define LARGE_COMM_SIZE (1*1024*1024)
#define NITER 1000

int* data_small;
int* data_large;

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    assert(size % 2 == 0);

    data_small = new int[SMALL_COMM_SIZE];
    data_large = new int[LARGE_COMM_SIZE];

    int src = (rank - 1);
    int dst = (rank + 1);
    if (src<0) { src = size-1; }
    if (dst>=size) { dst = 0; }

    double* wait_time_list = new double[NITER];
    double max_wt = 0;

    // overhead profiling with small communications with usleep
    std::chrono::duration<double, std::milli> wait_time;
    if (rank%2==0) {
        for(int i=0; i<NITER; ++i) {
            MPI_Request req;
            MPI_Status stat;
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Isend(data_small, SMALL_COMM_SIZE, MPI_INT, dst, 1, MPI_COMM_WORLD, &req);
            // sleep for ~10 ms to ensure the small communication is finished
            MPI_Barrier(MPI_COMM_WORLD);
            usleep(10000);
            auto w1 = std::chrono::high_resolution_clock::now();
            MPI_Wait(&req, &stat);
            auto w2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> wt = w2-w1;
            wait_time += wt;
            wait_time_list[i] = wt.count();
            max_wt = max_wt > wt.count() ? max_wt : wt.count();
        }
        std::cout << "## overhead profiling with usleep: Rank " << rank << "\n";
        std::cout << "MPI_Wait for Isend: " << wait_time.count()/NITER << " ms, maximum=" << max_wt << "ms\n";
    } else {
        for(int i=0; i<NITER; ++i) {
            MPI_Request req;
            MPI_Status stat;
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Irecv(data_small, SMALL_COMM_SIZE, MPI_INT, src, 1, MPI_COMM_WORLD, &req);
            // sleep for ~10 ms to ensure the small communication is finished
            MPI_Barrier(MPI_COMM_WORLD);
            usleep(10000);
            auto w1 = std::chrono::high_resolution_clock::now();
            MPI_Wait(&req, &stat);
            auto w2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> wt = w2-w1;
            wait_time += wt;
            wait_time_list[i] = wt.count();
            max_wt = max_wt > wt.count() ? max_wt : wt.count();
        }
        std::cout << "## overhead profiling with usleep: Rank " << rank << "\n";
        std::cout << "MPI_Wait for Irecv: " << wait_time.count()/NITER << " ms, maximum=" << max_wt << "ms\n";
    }
    

    // wait profiling with small communications (shoud be close to overhead)
    max_wt = 0;
    wait_time = std::chrono::duration<double, std::milli>(0);
    if (rank%2==0) {
        for(int i=0; i<NITER; ++i) {
            MPI_Request req;
            MPI_Status stat;
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Isend(data_small, SMALL_COMM_SIZE, MPI_INT, dst, 1, MPI_COMM_WORLD, &req);
            // Do nothing to measure the actual waiting time
            auto w1 = std::chrono::high_resolution_clock::now();
            MPI_Wait(&req, &stat);
            auto w2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> wt = w2-w1;
            wait_time += wt;
            wait_time_list[i] = wt.count();
            max_wt = max_wt > wt.count() ? max_wt : wt.count();
        }
        std::cout << "## wait profiling with small data: Rank " << rank << "\n";
        std::cout << "MPI_Wait for Isend: " << wait_time.count()/NITER << " ms, maximum=" << max_wt << "ms\n";
    } else {
        for(int i=0; i<NITER; ++i) {
            MPI_Request req;
            MPI_Status stat;
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Irecv(data_small, SMALL_COMM_SIZE, MPI_INT, src, 1, MPI_COMM_WORLD, &req);
            // Do nothing to measure the actual waiting time
            auto w1 = std::chrono::high_resolution_clock::now();
            MPI_Wait(&req, &stat);
            auto w2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> wt = w2-w1;
            wait_time += wt;
            wait_time_list[i] = wt.count();
            max_wt = max_wt > wt.count() ? max_wt : wt.count();
        }
        std::cout << "## wait profiling with small data: Rank " << rank << "\n";
        std::cout << "MPI_Wait for Irecv: " << wait_time.count()/NITER << " ms, maximum=" << max_wt << "ms\n";
    }

    // wait profiling with large communications
    max_wt = 0;
    wait_time = std::chrono::duration<double, std::milli>(0);
    if (rank%2==0) {
        for(int i=0; i<NITER; ++i) {
            MPI_Request req;
            MPI_Status stat;
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Isend(data_large, LARGE_COMM_SIZE, MPI_INT, dst, 1, MPI_COMM_WORLD, &req);
            // Do nothing to measure the actual waiting time
            auto w1 = std::chrono::high_resolution_clock::now();
            MPI_Wait(&req, &stat);
            auto w2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> wt = w2-w1;
            wait_time += wt;
            wait_time_list[i] = wt.count();
            max_wt = max_wt > wt.count() ? max_wt : wt.count();
        }
        std::cout << "## wait profiling with large data: Rank " << rank << "\n";
        std::cout << "MPI_Wait for Isend: " << wait_time.count()/NITER << " ms, maximum=" << max_wt << "ms\n";
    } else {
        for(int i=0; i<NITER; ++i) {
            MPI_Request req;
            MPI_Status stat;
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Irecv(data_large, LARGE_COMM_SIZE, MPI_INT, src, 1, MPI_COMM_WORLD, &req);
            // Do nothing to measure the actual waiting time
            auto w1 = std::chrono::high_resolution_clock::now();
            MPI_Wait(&req, &stat);
            auto w2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> wt = w2-w1;
            wait_time += wt;
            wait_time_list[i] = wt.count();
            max_wt = max_wt > wt.count() ? max_wt : wt.count();
        }
        std::cout << "## wait profiling with large data: Rank " << rank << "\n";
        std::cout << "MPI_Wait for Irecv: " << wait_time.count()/NITER << " ms, maximum=" << max_wt << "ms\n";
    }

    delete[] data_small;
    delete[] data_large;

    MPI_Finalize();
}
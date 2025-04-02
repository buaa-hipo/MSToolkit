#include "hthread_host.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mpi.h>
using namespace std;

#pragma pack(1)

typedef struct {
    int a, b, c;
    unsigned long d;
} type_s __attribute__((aligned(1)));

int main(int argc, char *argv[]) {
    printf("sizeof host type_s: %lu\n", sizeof(type_s));
  int rank, size;

  MPI_Init(&argc, &argv);               /* starts MPI */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); /* get current process id */
  MPI_Comm_size(MPI_COMM_WORLD, &size); /* get number of processes */
  printf("Hello world from process %d of %d\n", rank, size);

  hthread_dev_open(rank);

  hthread_dat_load(rank, "simple_mpi.dev.dat");

  int64_t cpu_result = 0;

  int64_t *data = (int64_t *)hthread_malloc(rank, sizeof(int64_t) * 20, HT_MEM_RO);
  int64_t *result = (int64_t *)hthread_malloc(rank, sizeof(int64_t) * 2, HT_MEM_WO);
  for (int i = 0; i < 20; ++i) {
    data[i] = i;
    cpu_result += i;
  }

  int group_id = hthread_group_create(rank, 1);
  hthread_group_wait(group_id);
  unsigned long args[3] = {
      (unsigned long)(20 / size),
      (unsigned long)(data + rank * (20 / size)),
      (unsigned long)(result + rank)
  };
  hthread_group_exec(group_id, "reduce_kernel", 1, 2, args);
  hthread_group_wait(group_id);
  if (rank == 0) {
    MPI_Recv(result + (1 - rank), 1, MPI_LONG, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (result[0] + result[1] == cpu_result) {
        printf("MPI-DSP reduce succeed\n");
    } else {
        printf("MPI-DSP reduce failed, expecting %ld, got %ld\n", cpu_result, result[0] + result[1]);
    }
  } else {
    MPI_Send(result + rank, 1, MPI_LONG, rank - 1, 0, MPI_COMM_WORLD);
  }
  hthread_free(data);
  hthread_group_destroy(group_id);
  hthread_dat_unload(rank);
  hthread_dev_close(rank);
  MPI_Finalize();
  return 0;
}
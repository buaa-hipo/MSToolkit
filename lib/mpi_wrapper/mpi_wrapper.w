#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/mman.h>
#include <sys/stat.h>        
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include "mpi_instrument.h"
#include "record/record_type_traits.h"

// MPI_Init does all the communicator setup
//
{{fn func MPI_Init}}{
    uint64_t start = get_tsc_raw();
    {{callfn}}
    auto ctxt = (record_t*) ALLOCATE(sizeof(record_t)+2*sizeof(uint64_t)*jsi_pmu_num);
    jsi_inst_init(ctxt, {{fn_id}}, start);
}{{endfn}}

{{fn func MPI_Init_thread}}{
    uint64_t start = get_tsc_raw();
    {{callfn}}
    auto ctxt = (record_t*) ALLOCATE(sizeof(record_t)+2*sizeof(uint64_t)*jsi_pmu_num);
    jsi_inst_init(ctxt, {{fn_id}}, start);
}{{endfn}}

{{fn func MPI_Finalize}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter(ctxt, {{fn_id}});
    {{callfn}}
    jsi_inst_exit(ctxt);
    jsi_inst_finalize();
}{{endfn}}

{{fn func MPI_Comm_rank}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_comm_rank(ctxt, {{fn_id}}, {{0}});
    {{callfn}}
    jsi_inst_exit_comm_rank(ctxt, {{1}});
}{{endfn}}

{{fn func MPI_Comm_dup}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_comm_dup(ctxt, {{fn_id}}, {{0}});
    {{callfn}}
    jsi_inst_exit_comm_dup(ctxt, {{1}});
}{{endfn}}

{{fn func MPI_Comm_split}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_comm_split(ctxt, {{fn_id}}, {{0}}, {{1}});
    {{callfn}}
    jsi_inst_exit_comm_split(ctxt, {{3}});
}{{endfn}}

{{fn func MPI_Send}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_comm(ctxt, {{fn_id}}, {{1}}, {{2}}, {{3}}, {{4}}, {{5}});
    {{callfn}}
    jsi_inst_exit(ctxt);
}{{endfn}}

{{fn func MPI_Isend}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_comm_async(ctxt, {{fn_id}}, {{1}}, {{2}}, {{3}}, {{4}}, {{5}});
    {{callfn}}
    jsi_inst_exit_comm_async(ctxt, {{6}});
}{{endfn}}

{{fn func MPI_Recv}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_comm(ctxt, {{fn_id}}, {{1}}, {{2}}, {{3}}, {{4}}, {{5}});
    {{callfn}}
    jsi_inst_exit(ctxt);
}{{endfn}}

{{fn func MPI_Irecv}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_comm_async(ctxt, {{fn_id}}, {{1}}, {{2}}, {{3}}, {{4}}, {{5}});
    {{callfn}}
    jsi_inst_exit_comm_async(ctxt, {{6}});
}{{endfn}}

{{fn func MPI_Wait}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_wait(ctxt, {{fn_id}}, {{0}});
    {{callfn}}
    jsi_inst_exit(reinterpret_cast<record_comm_wait_t*>(ctxt));
}{{endfn}}

{{fn func MPI_Alltoall}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_alltoall(ctxt, {{fn_id}}, {{1}}, {{2}}, {{4}}, {{6}});
    {{callfn}}
    jsi_inst_exit(reinterpret_cast<record_all2all_t*>(ctxt));
}{{endfn}}

{{fn func MPI_Alltoallv}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_alltoallv(ctxt, {{fn_id}}, {{1}}, {{3}}, {{5}}, {{8}});
    {{callfn}}
    jsi_inst_exit(reinterpret_cast<record_all2all_t*>(ctxt));
}{{endfn}}

{{fn func MPI_Allreduce}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_allreduce(ctxt, {{fn_id}}, {{2}}, {{3}}, {{4}}, {{5}});
    {{callfn}}
    jsi_inst_exit(reinterpret_cast<record_allreduce_t*>(ctxt));
}{{endfn}}

{{fn func MPI_Reduce}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_reduce(ctxt, {{fn_id}}, {{2}}, {{3}}, {{4}}, {{5}}, {{6}});
    {{callfn}}
    jsi_inst_exit(reinterpret_cast<record_reduce_t*>(ctxt));
}{{endfn}}

{{fn func MPI_Bcast}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_bcast(ctxt, {{fn_id}}, {{1}}, {{2}}, {{3}}, {{4}});
    {{callfn}}
    jsi_inst_exit(reinterpret_cast<record_bcast_t*>(ctxt));
}{{endfn}}

{{fn func MPI_Barrier}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter_barrier(ctxt, {{fn_id}}, {{0}});
    {{callfn}}
    jsi_inst_exit(reinterpret_cast<record_barrier_t*>(ctxt));
}{{endfn}}

// Ignore frequently called constant workload functions.
// TODO: just profile (time & call counts) with little overhead
{{fn func MPI_Cart_rank}}{
    {{callfn}}
}{{endfn}}

// This generates interceptors that will catch every MPI routine
// *except* MPI_Init.  The interceptors just make sure that if
// they are called with an argument of type MPI_Comm that has a
// value of MPI_COMM_WORLD, they switch it with world48.
{{fnall func MPI_Init MPI_Init_thread MPI_Finalize MPI_Comm_rank MPI_Comm_dup MPI_Comm_split MPI_Send MPI_Isend MPI_Recv MPI_Irecv MPI_Wait MPI_Alltoall MPI_Alltoallv MPI_Allreduce MPI_Reduce MPI_Bcast MPI_Barrier MPI_Cart_rank}}{
    auto ctxt = (MacroToType<{{fn_id}}>*)ALLOCATE(sizeof(MacroToType<{{fn_id}}>)+sizeof(uint64_t)*2*jsi_pmu_num);
    ctxt = jsi_inst_enter(ctxt, {{fn_id}});
    {{callfn}}
    jsi_inst_exit(ctxt);
}{{endfnall}}

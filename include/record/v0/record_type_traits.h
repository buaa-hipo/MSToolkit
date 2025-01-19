#pragma once
#include "record/record_reader.h"

template <int MsgType>
struct MacroToTypeHelper
{
    using type = decltype([] {
        if constexpr (MsgType == event_MPI_Send || MsgType == event_MPI_Recv)
        {
            return record_comm_t{};
        }
        else if constexpr (MsgType == event_MPI_Isend || (MsgType == event_MPI_Irecv))
        {
            return record_comm_async_t{};
        }
        else if constexpr (MsgType == event_MPI_Wait)
        {
            return record_comm_wait_t{};
        }

        else if constexpr ((MsgType == event_MPI_Alltoall) || (MsgType == event_MPI_Alltoallv))
        {
            return record_all2all_t{};
        }

        else if constexpr (MsgType == event_MPI_Allreduce)
        {
            return record_allreduce_t{};
        }

        else if constexpr (MsgType == event_MPI_Reduce)
        {
            return record_reduce_t{};
        }

        else if constexpr (MsgType == event_MPI_Bcast)
        {
            return record_bcast_t{};
        }

        else if constexpr (MsgType == event_MPI_Comm_rank)
        {
            return record_comm_rank_t{};
        }

        else if constexpr (MsgType == event_MPI_Comm_dup)
        {
            return record_comm_dup_t{};
        }

        else if constexpr (MsgType == event_MPI_Comm_split)
        {
            return record_comm_split_t{};
        }

        else if constexpr (MsgType == event_MPI_Barrier)
        {
            return record_barrier_t{};
        }

	    else if constexpr (MsgType == event_Memory_Malloc)
		{
			return record_memory_malloc{};
		}

	    else if constexpr (MsgType == event_Memory_Calloc)
		{
			return record_memory_calloc{};
		}

	    else if constexpr (MsgType == event_Memory_Realloc)
		{
			return record_memory_realloc{};
		}

	    else if constexpr (MsgType == event_Memory_Free)
		{
			return record_memory_free{};
		}
        
        else
        {
            return record_t{};
        }
    }());
};

template <int MsgType>
using MacroToType = typename MacroToTypeHelper<MsgType>::type;
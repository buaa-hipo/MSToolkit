#include <iostream>
#include "utils/cxxopts.hpp"
#include <filesystem>
#include <stdio.h>
#include "../graph/dependancy_graph.h"
#include <unordered_map>
#include <list>
#include <queue>
#include <string>

namespace fs = std::filesystem;

void comm_statistic(int* comm_mark, RecordTraceCollection& traces){

    int rank_all = traces.size();
    printf("%6d, ",rank_all);
    for(auto it=traces.begin(), ie=traces.end(); it!=ie; ++it)  {
        int rank = it->first;
        RecordTrace& rt = *(it->second);
        RecordTraceIterator rti = rt.find(JSI_PROCESS_START, true/*ignore zoom*/);
        if(RecordTraceIterator::is_invalid(rti)) {
            JSI_ERROR("Error: Could not find PROCESS_START event in Rank %d\n", rank);
        }

        for(auto ri=rt.begin(), re=rt.end(); ri!=re; ri=ri.next()) {
            record_t* r = ri.val();
            //printf("point 1 -----: 1\n");
            //RecordNode* node;
            if (RecordHelper::is_event(r, event_MPI_Send)){
                record_comm_t* rec = reinterpret_cast<record_comm_t*>(r);
                int dst = rec->dest;
                comm_mark[ rank * rank_all + dst]++;
                comm_mark[ dst * rank_all + rank]++;
            }
            if (RecordHelper::is_event(r, event_MPI_Isend)){
                record_comm_async_t* rec = reinterpret_cast<record_comm_async_t*>(r);
                int dst = rec->dest;
                comm_mark[ rank * rank_all + dst]++;
                comm_mark[ dst * rank_all + rank]++;
            }
            if (RecordHelper::is_event(r, event_MPI_Recv)){
                record_comm_t* rec = reinterpret_cast<record_comm_t*>(r);
                int dst = rec->dest;
                comm_mark[ rank * rank_all + dst]++;
                comm_mark[ dst * rank_all + rank]++;
                
            }
            if (RecordHelper::is_event(r, event_MPI_Irecv)){
                record_comm_async_t* rec = reinterpret_cast<record_comm_async_t*>(r);
                int dst = rec->dest;
                comm_mark[ rank * rank_all + dst]++;
                comm_mark[ dst * rank_all + rank]++;
            }
            if (RecordHelper::is_event(r, event_MPI_Alltoall)){
                for ( int i = 0; i< rank_all*rank_all; i++){
                    comm_mark[ i ]++;    
                }
            }
            if (RecordHelper::is_event(r, event_MPI_Alltoallv)){
                for ( int i = 0; i< rank_all*rank_all; i++){
                    comm_mark[ i ]++;    
                }
            }
            if (RecordHelper::is_event(r, event_MPI_Allreduce)){
                for ( int i = 0; i< rank_all*rank_all; i++){
                    comm_mark[ i ]++;    
                }
            }
            if (RecordHelper::is_event(r, event_MPI_Reduce)){
                record_reduce_t* rec = reinterpret_cast<record_reduce_t*>(r);
                int dst = rec->root;
                for ( int i = dst , j = 0 ; j < rank_all ; j++){
                    comm_mark[ i * rank_all + j ]++;
                    comm_mark[ j * rank_all + i ]++;
                }
            }
            if (RecordHelper::is_event(r, event_MPI_Bcast)){
                record_bcast_t* rec = reinterpret_cast<record_bcast_t*>(r);
                int dst = rec->root;
                for ( int i = dst , j = 0 ; j < rank_all ; j++){
                    comm_mark[ i * rank_all + j ]++;
                    comm_mark[ j * rank_all + i ]++;
                }
            }
        }
    }

}
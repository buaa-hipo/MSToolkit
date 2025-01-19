import sys
import argparse
# copied from wrap.py
MPI_FUNC_INDEX=["MPI_Init","MPI_Finalize","MPI_Send","MPI_Recv","MPI_Irecv","MPI_Abort","MPI_Accumulate","MPI_Add_error_class","MPI_Add_error_code","MPI_Add_error_string","MPI_Address","MPI_Aint_add","MPI_Aint_diff","MPI_Allgather","MPI_Allgatherv","MPI_Alloc_mem","MPI_Allreduce","MPI_Alltoall","MPI_Alltoallv","MPI_Alltoallw","MPI_Attr_delete","MPI_Attr_get","MPI_Attr_put","MPI_Barrier","MPI_Bcast","MPI_Bsend","MPI_Bsend_init","MPI_Buffer_attach","MPI_Buffer_detach","MPI_Cancel","MPI_Cart_coords","MPI_Cart_create","MPI_Cart_get","MPI_Cart_map","MPI_Cart_rank","MPI_Cart_shift","MPI_Cart_sub","MPI_Cartdim_get","MPI_Close_port","MPI_Comm_accept","MPI_Comm_call_errhandler","MPI_Comm_compare","MPI_Comm_connect","MPI_Comm_create","MPI_Comm_create_errhandler","MPI_Comm_create_group","MPI_Comm_create_keyval","MPI_Comm_delete_attr","MPI_Comm_disconnect","MPI_Comm_dup","MPI_Comm_dup_with_info","MPI_Comm_free","MPI_Comm_free_keyval","MPI_Comm_get_attr","MPI_Comm_get_errhandler","MPI_Comm_get_info","MPI_Comm_get_name","MPI_Comm_get_parent","MPI_Comm_group","MPI_Comm_idup","MPI_Comm_join","MPI_Comm_rank","MPI_Comm_remote_group","MPI_Comm_remote_size","MPI_Comm_set_attr","MPI_Comm_set_errhandler","MPI_Comm_set_info","MPI_Comm_set_name","MPI_Comm_size","MPI_Comm_split","MPI_Comm_split_type","MPI_Comm_test_inter","MPI_Compare_and_swap","MPI_Dims_create","MPI_Dist_graph_create","MPI_Dist_graph_create_adjacent","MPI_Dist_graph_neighbors","MPI_Dist_graph_neighbors_count","MPI_Errhandler_create","MPI_Errhandler_free","MPI_Errhandler_get","MPI_Errhandler_set","MPI_Error_class","MPI_Error_string","MPI_Exscan","MPI_Fetch_and_op","MPI_File_call_errhandler","MPI_File_close","MPI_File_create_errhandler","MPI_File_delete","MPI_File_get_amode","MPI_File_get_atomicity","MPI_File_get_byte_offset","MPI_File_get_errhandler","MPI_File_get_group","MPI_File_get_info","MPI_File_get_position","MPI_File_get_position_shared","MPI_File_get_size","MPI_File_get_type_extent","MPI_File_get_view","MPI_File_iread","MPI_File_iread_all","MPI_File_iread_at","MPI_File_iread_at_all","MPI_File_iread_shared","MPI_File_iwrite","MPI_File_iwrite_all","MPI_File_iwrite_at","MPI_File_iwrite_at_all","MPI_File_iwrite_shared","MPI_File_open","MPI_File_preallocate","MPI_File_read","MPI_File_read_all","MPI_File_read_all_begin","MPI_File_read_all_end","MPI_File_read_at","MPI_File_read_at_all","MPI_File_read_at_all_begin","MPI_File_read_at_all_end","MPI_File_read_ordered","MPI_File_read_ordered_begin","MPI_File_read_ordered_end","MPI_File_read_shared","MPI_File_seek","MPI_File_seek_shared","MPI_File_set_atomicity","MPI_File_set_errhandler","MPI_File_set_info","MPI_File_set_size","MPI_File_set_view","MPI_File_sync","MPI_File_write","MPI_File_write_all","MPI_File_write_all_begin","MPI_File_write_all_end","MPI_File_write_at","MPI_File_write_at_all","MPI_File_write_at_all_begin","MPI_File_write_at_all_end","MPI_File_write_ordered","MPI_File_write_ordered_begin","MPI_File_write_ordered_end","MPI_File_write_shared","MPI_Finalized","MPI_Free_mem","MPI_Gather","MPI_Gatherv","MPI_Get","MPI_Get_accumulate","MPI_Get_address","MPI_Get_count","MPI_Get_elements","MPI_Get_elements_x","MPI_Get_library_version","MPI_Get_processor_name","MPI_Get_version","MPI_Graph_create","MPI_Graph_get","MPI_Graph_map","MPI_Graph_neighbors","MPI_Graph_neighbors_count","MPI_Graphdims_get","MPI_Grequest_complete","MPI_Grequest_start","MPI_Group_compare","MPI_Group_difference","MPI_Group_excl","MPI_Group_free","MPI_Group_incl","MPI_Group_intersection","MPI_Group_range_excl","MPI_Group_range_incl","MPI_Group_rank","MPI_Group_size","MPI_Group_translate_ranks","MPI_Group_union","MPI_Iallgather","MPI_Iallgatherv","MPI_Iallreduce","MPI_Ialltoall","MPI_Ialltoallv","MPI_Ialltoallw","MPI_Ibarrier","MPI_Ibcast","MPI_Ibsend","MPI_Iexscan","MPI_Igather","MPI_Igatherv","MPI_Improbe","MPI_Imrecv","MPI_Ineighbor_allgather","MPI_Ineighbor_allgatherv","MPI_Ineighbor_alltoall","MPI_Ineighbor_alltoallv","MPI_Ineighbor_alltoallw","MPI_Info_create","MPI_Info_delete","MPI_Info_dup","MPI_Info_free","MPI_Info_get","MPI_Info_get_nkeys","MPI_Info_get_nthkey","MPI_Info_get_valuelen","MPI_Info_set","MPI_Intercomm_create","MPI_Intercomm_merge","MPI_Iprobe","MPI_Ireduce","MPI_Ireduce_scatter","MPI_Ireduce_scatter_block","MPI_Irsend","MPI_Is_thread_main","MPI_Iscan","MPI_Iscatter","MPI_Iscatterv","MPI_Isend","MPI_Issend","MPI_Keyval_create","MPI_Keyval_free","MPI_Lookup_name","MPI_Mprobe","MPI_Mrecv","MPI_Neighbor_allgather","MPI_Neighbor_allgatherv","MPI_Neighbor_alltoall","MPI_Neighbor_alltoallv","MPI_Neighbor_alltoallw","MPI_Op_commutative","MPI_Op_create","MPI_Op_free","MPI_Open_port","MPI_Pack","MPI_Pack_external","MPI_Pack_external_size","MPI_Pack_size","MPI_Pcontrol","MPI_Probe","MPI_Publish_name","MPI_Put","MPI_Query_thread","MPI_Raccumulate","MPI_Recv_init","MPI_Reduce","MPI_Reduce_local","MPI_Reduce_scatter","MPI_Reduce_scatter_block","MPI_Register_datarep","MPI_Request_free","MPI_Request_get_status","MPI_Rget","MPI_Rget_accumulate","MPI_Rput","MPI_Rsend","MPI_Rsend_init","MPI_Scan","MPI_Scatter","MPI_Scatterv","MPI_Send_init","MPI_Sendrecv","MPI_Sendrecv_replace","MPI_Ssend","MPI_Ssend_init","MPI_Start","MPI_Startall","MPI_Status_set_cancelled","MPI_Status_set_elements","MPI_Status_set_elements_x","MPI_Test","MPI_Test_cancelled","MPI_Testall","MPI_Testany","MPI_Testsome","MPI_Topo_test","MPI_Type_commit","MPI_Type_contiguous","MPI_Type_create_darray","MPI_Type_create_f90_complex","MPI_Type_create_f90_integer","MPI_Type_create_f90_real","MPI_Type_create_hindexed","MPI_Type_create_hindexed_block","MPI_Type_create_hvector","MPI_Type_create_indexed_block","MPI_Type_create_keyval","MPI_Type_create_resized","MPI_Type_create_struct","MPI_Type_create_subarray","MPI_Type_delete_attr","MPI_Type_dup","MPI_Type_extent","MPI_Type_free","MPI_Type_free_keyval","MPI_Type_get_attr","MPI_Type_get_contents","MPI_Type_get_envelope","MPI_Type_get_extent","MPI_Type_get_extent_x","MPI_Type_get_name","MPI_Type_get_true_extent","MPI_Type_get_true_extent_x","MPI_Type_hindexed","MPI_Type_hvector","MPI_Type_indexed","MPI_Type_lb","MPI_Type_match_size","MPI_Type_set_attr","MPI_Type_set_name","MPI_Type_size","MPI_Type_size_x","MPI_Type_struct","MPI_Type_ub","MPI_Type_vector","MPI_Unpack","MPI_Unpack_external","MPI_Unpublish_name","MPI_Wait","MPI_Waitall","MPI_Waitany","MPI_Waitsome","MPI_Win_allocate","MPI_Win_allocate_shared","MPI_Win_attach","MPI_Win_call_errhandler","MPI_Win_complete","MPI_Win_create","MPI_Win_create_dynamic","MPI_Win_create_errhandler","MPI_Win_create_keyval","MPI_Win_delete_attr","MPI_Win_detach","MPI_Win_fence","MPI_Win_flush","MPI_Win_flush_all","MPI_Win_flush_local","MPI_Win_flush_local_all","MPI_Win_free","MPI_Win_free_keyval","MPI_Win_get_attr","MPI_Win_get_errhandler","MPI_Win_get_group","MPI_Win_get_info","MPI_Win_get_name","MPI_Win_lock","MPI_Win_lock_all","MPI_Win_post","MPI_Win_set_attr","MPI_Win_set_errhandler","MPI_Win_set_info","MPI_Win_set_name","MPI_Win_shared_query","MPI_Win_start","MPI_Win_sync","MPI_Win_test","MPI_Win_unlock","MPI_Win_unlock_all","MPI_Win_wait","MPI_Wtick","MPI_Wtime","MPI_Init_thread",
                "Memory_Malloc","Memory_Calloc","Memory_Realloc","Memory_Free"]

parser = argparse.ArgumentParser(description='generate wrapper headers')
parser.add_argument('--workdir', help='workdir', default='./')
args = parser.parse_args()

WORKDIR=args.workdir

output_include = WORKDIR+"/include/record/wrap_defines.h"

include_header = '''
/* Generated by generate_wrap_defines.py */
#ifndef __PY_WRAP_MPI_WRAPPER_H__
#define __PY_WRAP_MPI_WRAPPER_H__
'''
include_end = "#endif"

if output_include:
    try:
        output = open(output_include, "w")
        output.write(include_header)
        for i,name in enumerate(MPI_FUNC_INDEX):
            output.write("#define {fn} {fn_id}\n".format(fn='event_'+name, fn_id=i))
        output.write("#define __RECORD_MAP_IMPL ")
        for i,name in enumerate(MPI_FUNC_INDEX):
            output.write("\\\n  case {fn_id}: return std::string(\"{fn}\");".format(fn=name, fn_id=i))
        output.write("\n")
        
        #output.write("#define  event_Memory_Malloc 5200\n")
        #output.write("#define  event_Memory_Calloc 5201\n")
        #output.write("#define  event_Memory_Realloc 5202\n")
        #output.write("#define  event_Memory_Free 5203\n")
        
        output.write("enum class event\n"
                    "{\n")
        for i,name in enumerate(MPI_FUNC_INDEX):
            output.write("{fn} = {fn_id},\n".format(fn=name, fn_id=i))
        output.write('};\n')
        output.write("\n")


        output.write(include_end)
        output.close()
    except IOError:
        sys.stderr.write("Error: couldn't open file " + output_include + " for writing.\n")
        sys.exit(1)


record_type_info_file = WORKDIR+'/include/record/record_type_info.h'

try:
    with open(record_type_info_file, "w") as output:
        output.write('#pragma once\n')
        output.write('#include "record/wrap_defines.h"\n')
        output.write('#include "record/record_type_traits.h"\n')
        output.write('#include "utils/compile_time.h"\n')
        output.write('#include "ral/time_detector.h"\n')
        output.write('#include <string_view>\n')
        output.write("inline pse::utils::EncodedStruct record_info[] = {\n")
        for i,name in enumerate(MPI_FUNC_INDEX):
            output.write("pse::utils::StructEncoder<MacroToType<event_{fn}>>::get_encoded_struct(),\n".format(fn=name))
        output.write("};\n\n")

        output.write("inline long record_time_offset[] = {\n")
        for i,name in enumerate(MPI_FUNC_INDEX):
            output.write(f"[](){{using T = MacroToType<event_{name}>; T t; return (char*)&pse::ral::TimeAccessor<T>::get_field(t) - (char*)&t;}}(),\n")
        output.write("};\n\n")


except IOError:
    sys.stderr.write("Error: couldn't open file " + record_type_info_file + " for writing.\n")
    sys.exit(1)
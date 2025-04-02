
#pragma once
#include "record/wrap_defines.h"
#include "ral/time_detector.h"
#include "record/record_type.h"

template <int MsgType>
struct MacroToTypeHelper
{
    using type = decltype([] {

	if constexpr (MsgType == event_MPI_Send)
		{
			return record_comm_t{};
		}

	else if constexpr (MsgType == event_MPI_Recv)
		{
			return record_comm_t{};
		}

	else if constexpr (MsgType == event_MPI_Irecv)
		{
			return record_comm_async_t{};
		}

	else if constexpr (MsgType == event_MPI_Allreduce)
		{
			return record_allreduce_t{};
		}

	else if constexpr (MsgType == event_MPI_Alltoall)
		{
			return record_all2all_t{};
		}

	else if constexpr (MsgType == event_MPI_Alltoallv)
		{
			return record_all2all_t{};
		}

	else if constexpr (MsgType == event_MPI_Barrier)
		{
			return record_barrier_t{};
		}

	else if constexpr (MsgType == event_MPI_Bcast)
		{
			return record_bcast_t{};
		}

	else if constexpr (MsgType == event_MPI_Comm_dup)
		{
			return record_comm_dup_t{};
		}

	else if constexpr (MsgType == event_MPI_Comm_rank)
		{
			return record_comm_rank_t{};
		}

	else if constexpr (MsgType == event_MPI_Comm_split)
		{
			return record_comm_split_t{};
		}

	else if constexpr (MsgType == event_MPI_Isend)
		{
			return record_comm_async_t{};
		}

	else if constexpr (MsgType == event_MPI_Reduce)
		{
			return record_reduce_t{};
		}

	else if constexpr (MsgType == event_MPI_Wait)
		{
			return record_comm_wait_t{};
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

	else if constexpr (MsgType == event_Memory_Memalign)
		{
			return record_memory_memalign{};
		}

	else if constexpr (MsgType == event_Memory_Aligned_Alloc)
		{
			return record_memory_aligned_alloc{};
		}

	else if constexpr (MsgType == event_Memory_Posix_Memalign)
		{
			return record_memory_posix_memalign{};
		}

	else if constexpr (MsgType == event_hipDeviceEnablePeerAccess)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipImportExternalMemory)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipFuncSetSharedMemConfig)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDestroyExternalMemory)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipProfilerStop)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMallocPitch)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipMalloc)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipMemsetD16)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDrvMemcpy2DUnaligned)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipExtStreamGetCUMask)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipEventRecord)
		{
			return record_activity_event_t{};
		}

	else if constexpr (MsgType == event_hipCtxSynchronize)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipSetDevice)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipCtxGetApiVersion)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyFromSymbolAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipExtGetLinkTypeAndHopCount)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event___hipPopCallConfiguration)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipModuleOccupancyMaxActiveBlocksPerMultiprocessor)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemset3D)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDestroySurfaceObject)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamCreateWithPriority)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpy2DToArray)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipMemsetD8Async)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipCtxGetCacheConfig)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamWaitEvent)
		{
			return record_activity_wait_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetStreamPriorityRange)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipModuleLoad)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyToSymbolAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipDrvPointerGetAttributes)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDevicePrimaryCtxSetFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipArrayDestroy)
		{
			return record_activity_free_t{};
		}

	else if constexpr (MsgType == event_hipLaunchCooperativeKernel)
		{
			return record_activity_launch_t{};
		}

	else if constexpr (MsgType == event_hipLaunchCooperativeKernelMultiDevice)
		{
			return record_activity_launch_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipMalloc3DArray)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipCtxGetCurrent)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipExternalMemoryGetMappedBuffer)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDevicePrimaryCtxGetState)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipEventQuery)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamWaitValue64)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipEventCreate)
		{
			return record_activity_event_t{};
		}

	else if constexpr (MsgType == event_hipMemGetAddressRange)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamWriteValue32)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyFromSymbol)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipArrayCreate)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipStreamAttachMemAsync)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamGetFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMallocArray)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipCtxGetSharedMemConfig)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceDisablePeerAccess)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipModuleOccupancyMaxPotentialBlockSize)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPtrGetInfo)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipFuncGetAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipCtxGetFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamDestroy)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event___hipPushCallConfiguration)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemset3DAsync)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetPCIBusId)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_RESERVED_59)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipInit)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyAtoH)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipStreamGetPriority)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemset2D)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemset2DAsync)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceCanAccessPeer)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipLaunchByPtr)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipLaunchKernel)
		{
			return record_activity_launch_t{};
		}

	else if constexpr (MsgType == event_hipCtxDestroy)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemsetD16Async)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipModuleUnload)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipHostUnregister)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipImportExternalSemaphore)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipExtStreamCreateWithCUMask)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipExtGetNearstCPU)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamSynchronize)
		{
			return record_activity_wait_t{};
		}

	else if constexpr (MsgType == event_hipDeviceSetCacheConfig)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyHtoD)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipModuleGetGlobal)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyHtoA)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipCtxCreate)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpy2D)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipIpcCloseMemHandle)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDestroyExternalSemaphore)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipChooseDevice)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceSetSharedMemConfig)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMallocMipmappedArray)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipSetupArgument)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipIpcGetEventHandle)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipFreeArray)
		{
			return record_activity_free_t{};
		}

	else if constexpr (MsgType == event_hipCtxSetCacheConfig)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipFuncSetCacheConfig)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipModuleOccupancyMaxActiveBlocksPerMultiprocessorWithFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipModuleGetTexRef)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipFuncSetAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipEventElapsedTime)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipConfigureCall)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGetMipmappedArrayLevel)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpy3DAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipSignalExternalSemaphoresAsync)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipEventDestroy)
		{
			return record_activity_event_t{};
		}

	else if constexpr (MsgType == event_hipCtxPopCurrent)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipPointerGetAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPrefetchAsync)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGetSymbolAddress)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipHostGetFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipHostMalloc)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipCtxSetSharedMemConfig)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipFreeMipmappedArray)
		{
			return record_activity_free_t{};
		}

	else if constexpr (MsgType == event_hipMemGetInfo)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceReset)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemset)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemsetD8)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyParam2DAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipHostRegister)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDriverGetVersion)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipArray3DCreate)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipIpcOpenMemHandle)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamWaitValue32)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGetLastError)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGetDeviceFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetSharedMemConfig)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDrvMemcpy3D)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipMemcpy2DFromArray)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipOccupancyMaxActiveBlocksPerMultiprocessorWithFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipSetDeviceFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipHccModuleLaunchKernel)
		{
			return record_activity_launch_t{};
		}

	else if constexpr (MsgType == event_hipFree)
		{
			return record_activity_free_t{};
		}

	else if constexpr (MsgType == event_hipOccupancyMaxPotentialBlockSize)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceComputeCapability)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipWaitExternalSemaphoresAsync)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipCtxDisablePeerAccess)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMallocManaged)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetByPCIBusId)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipIpcGetMemHandle)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyHtoDAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipCtxGetDevice)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyDtoD)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipModuleLoadData)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDevicePrimaryCtxRelease)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipOccupancyMaxActiveBlocksPerMultiprocessor)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipCtxSetCurrent)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamCreate)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDevicePrimaryCtxRetain)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGet)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamCreateWithFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyFromArray)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipMemcpy2DAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipFuncGetAttributes)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGetSymbolSize)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipCreateSurfaceObject)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemAdvise)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipEventCreateWithFlags)
		{
			return record_activity_event_t{};
		}

	else if constexpr (MsgType == event_hipStreamQuery)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpy3D)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyToSymbol)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipMemcpy)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipStreamWriteValue64)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipPeekAtLastError)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipExtLaunchMultiKernelMultiDevice)
		{
			return record_activity_launch_t{};
		}

	else if constexpr (MsgType == event_hipStreamAddCallback)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyToArray)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipMemsetD32)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipExtModuleLaunchKernel)
		{
			return record_activity_launch_t{};
		}

	else if constexpr (MsgType == event_hipDeviceSynchronize)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetCacheConfig)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemRangeGetAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMalloc3D)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipPointerGetAttributes)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpy2DToArrayAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipMemsetAsync)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetName)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipModuleOccupancyMaxPotentialBlockSizeWithFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipCtxPushCurrent)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyPeer)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipEventSynchronize)
		{
			return record_activity_wait_t{};
		}

	else if constexpr (MsgType == event_hipExtMallocManaged)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyDtoDAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipProfilerStart)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipExtMallocWithFlags)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipCtxEnablePeerAccess)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyDtoHAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipModuleLaunchKernel)
		{
			return record_activity_launch_t{};
		}

	else if constexpr (MsgType == event_hipMemAllocPitch)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipExtLaunchKernel)
		{
			return record_activity_launch_t{};
		}

	else if constexpr (MsgType == event_hipMemcpy2DFromArrayAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetLimit)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipModuleLoadDataEx)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipRuntimeGetVersion)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipHostFree)
		{
			return record_activity_free_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetP2PAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyPeerAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipGetDeviceProperties)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyDtoH)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyWithStream)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_hipDeviceTotalMem)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipHostGetDevicePointer)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemRangeGetAttributes)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipExtHostMalloc)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipMemcpyParam2D)
		{
			return record_activity_memcpy_t{};
		}

	else if constexpr (MsgType == event_hipDevicePrimaryCtxReset)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipModuleGetFunction)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemsetD32Async)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGetDevice)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGetDeviceCount)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipIpcOpenEventHandle)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDrvMemcpy3DAsync)
		{
			return record_activity_memcpy_async_t{};
		}

	else if constexpr (MsgType == event___hipPopCallConfiguration_internal)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event___hipPushCallConfiguration_internal)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetDefaultMemPool)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetMemPool)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetUuid)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceSetMemPool)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipFreeAsync)
		{
			return record_activity_free_t{};
		}

	else if constexpr (MsgType == event_hipFreeHost)
		{
			return record_activity_free_t{};
		}

	else if constexpr (MsgType == event_hipGLGetDevices)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGetChannelDesc)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGetErrorString)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddChildGraphNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddDependencies)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddEmptyNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddEventRecordNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddEventWaitNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddHostNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddKernelNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddMemcpyNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddMemcpyNode1D)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddMemcpyNodeFromSymbol)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddMemcpyNodeToSymbol)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddMemsetNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphChildGraphNodeGetGraph)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphClone)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphCreate)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphDestroy)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphDestroyNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphEventRecordNodeGetEvent)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphEventRecordNodeSetEvent)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphEventWaitNodeGetEvent)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphEventWaitNodeSetEvent)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecChildGraphNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecDestroy)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecEventRecordNodeSetEvent)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecEventWaitNodeSetEvent)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecHostNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecKernelNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecMemcpyNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecMemcpyNodeSetParams1D)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecMemcpyNodeSetParamsFromSymbol)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecMemcpyNodeSetParamsToSymbol)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecMemsetNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecUpdate)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphGetEdges)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphGetNodes)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphGetRootNodes)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphHostNodeGetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphHostNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphInstantiate)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphInstantiateWithFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphKernelNodeGetAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphKernelNodeGetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphKernelNodeSetAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphKernelNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphLaunch)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphMemcpyNodeGetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphMemcpyNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphMemcpyNodeSetParams1D)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphMemcpyNodeSetParamsFromSymbol)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphMemcpyNodeSetParamsToSymbol)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphMemsetNodeGetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphMemsetNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphNodeFindInClone)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphNodeGetDependencies)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphNodeGetDependentNodes)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphNodeGetType)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphRemoveDependencies)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphicsGLRegisterBuffer)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphicsMapResources)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphicsResourceGetMappedPointer)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphicsUnmapResources)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphicsUnregisterResource)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipHostAlloc)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipLaunchKernel_internal)
		{
			return record_activity_launch_t{};
		}

	else if constexpr (MsgType == event_hipMallocAsync)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipMallocFromPoolAsync)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipMallocHost)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipMemAddressFree)
		{
			return record_activity_free_t{};
		}

	else if constexpr (MsgType == event_hipMemAddressReserve)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemAllocHost)
		{
			return record_activity_mem_alloc_t{};
		}

	else if constexpr (MsgType == event_hipMemCreate)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemExportToShareableHandle)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemGetAccess)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemGetAllocationGranularity)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemGetAllocationPropertiesFromHandle)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemImportFromShareableHandle)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemMap)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemMapArrayAsync)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPoolCreate)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPoolDestroy)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPoolExportPointer)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPoolExportToShareableHandle)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPoolGetAccess)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPoolGetAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPoolImportFromShareableHandle)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPoolImportPointer)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPoolSetAccess)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPoolSetAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemPoolTrimTo)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemRelease)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemRetainAllocationHandle)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemSetAccess)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMemUnmap)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMipmappedArrayCreate)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipMipmappedArrayDestroy)
		{
			return record_activity_free_t{};
		}

	else if constexpr (MsgType == event_hipMipmappedArrayGetLevel)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipRegisterActivityCallback)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipRegisterApiCallback)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipRemoveActivityCallback)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipRemoveApiCallback)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamBeginCapture)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamEndCapture)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamGetCaptureInfo)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamGetCaptureInfo_v2)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamIsCapturing)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamUpdateCaptureDependencies)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefGetAddress)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefGetFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefGetFormat)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefGetMaxAnisotropy)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefGetMipMappedArray)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefGetMipmapLevelBias)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefGetMipmapLevelClamp)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefSetAddress)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefSetAddress2D)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefSetArray)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefSetBorderColor)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefSetFlags)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefSetFormat)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefSetMaxAnisotropy)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefSetMipmapLevelBias)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefSetMipmapLevelClamp)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefSetMipmappedArray)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphDebugDotPrint)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphKernelNodeCopyAttributes)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphNodeGetEnabled)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphNodeSetEnabled)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipPointerSetAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddMemAllocNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddMemFreeNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphMemAllocNodeGetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphMemFreeNodeGetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipArray3DGetDescriptor)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipArrayGetDescriptor)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipArrayGetInfo)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipStreamGetDevice)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGetGraphMemAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceGraphMemTrim)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceSetGraphMemAttribute)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipDeviceSetLimit)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddExternalSemaphoresSignalNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphAddExternalSemaphoresWaitNode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecExternalSemaphoresSignalNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExecExternalSemaphoresWaitNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExternalSemaphoresSignalNodeGetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExternalSemaphoresSignalNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExternalSemaphoresWaitNodeGetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphExternalSemaphoresWaitNodeSetParams)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphReleaseUserObject)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphRetainUserObject)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphUpload)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphicsGLRegisterImage)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipGraphicsSubResourceGetMappedArray)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipLaunchHostFunc)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefGetArray)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefGetBorderColor)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipTexRefGetMipmappedArray)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipThreadExchangeStreamCaptureMode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipUserObjectCreate)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipUserObjectRelease)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipUserObjectRetain)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcAddNameExpression)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcCompileProgram)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcCreateProgram)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcCreateProgram_internal)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcDestroyProgram)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcGetBitcode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcGetBitcodeSize)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcGetCode)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcGetCodeSize)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcGetLoweredName)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcGetProgramLog)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcGetProgramLogSize)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcLinkAddData)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcLinkAddFile)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcLinkComplete)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcLinkCreate)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcLinkDestroy)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hiprtcVersion)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_hipFuncGetModule)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_group_create)
		{
			return mt_record_kernel_launch_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_group_create_masked_launch)
		{
			return mt_record_kernel_launch_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_group_create_launch)
		{
			return mt_record_kernel_launch_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_group_exec)
		{
			return mt_record_kernel_launch_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_group_wait)
		{
			return mt_record_group_wait_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_group_destroy)
		{
			return mt_record_kernel_launch_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_malloc)
		{
			return mt_record_malloc_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_free)
		{
			return mt_record_free_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_vector_malloc)
		{
			return mt_record_malloc_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_vector_free)
		{
			return mt_record_free_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_scalar_malloc)
		{
			return mt_record_malloc_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_scalar_free)
		{
			return mt_record_free_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_hbm_malloc)
		{
			return mt_record_malloc_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_hbm_free)
		{
			return mt_record_free_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_vector_load)
		{
			return mt_record_memcpy_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_vector_store)
		{
			return mt_record_memcpy_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_scalar_load)
		{
			return mt_record_memcpy_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_scalar_store)
		{
			return mt_record_memcpy_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_vector_load_async)
		{
			return mt_record_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_vector_store_async)
		{
			return mt_record_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_scalar_load_async)
		{
			return mt_record_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_scalar_store_async)
		{
			return mt_record_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_dma_p2p)
		{
			return mt_record_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_dma_broadcast)
		{
			return mt_record_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_dma_segment)
		{
			return mt_record_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_dma_sg)
		{
			return mt_record_memcpy_async_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_dma_wait)
		{
			return mt_record_dma_wait_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_dma_wait_p2p)
		{
			return mt_record_dma_wait_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_dma_wait_sg)
		{
			return mt_record_dma_wait_t{};
		}

	else if constexpr (MsgType == event_ACCL_USER_FUNC)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_dat_load)
		{
			return mt_record_driver_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_dat_unload)
		{
			return mt_record_driver_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_dev_close)
		{
			return mt_record_driver_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_dev_open)
		{
			return mt_record_driver_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_dev_owner)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_group_get_status)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_barrier_create)
		{
			return mt_record_barrier_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_barrier_destroy)
		{
			return mt_record_barrier_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_rwlock_create)
		{
			return mt_record_rwlock_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_rwlock_destroy)
		{
			return mt_record_rwlock_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_intr_send)
		{
			return mt_record_intr_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_intr_reg)
		{
			return mt_record_intr_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_group_barrier)
		{
			return mt_record_dev_barrier_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_core_barrier)
		{
			return mt_record_dev_barrier_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_core_barrier_wait)
		{
			return mt_record_dev_barrier_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_rwlock_try_rdlock)
		{
			return mt_record_dev_rwlock_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_rwlock_try_wrlock)
		{
			return mt_record_dev_rwlock_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_rwlock_rdlock)
		{
			return mt_record_dev_rwlock_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_rwlock_wrlock)
		{
			return mt_record_dev_rwlock_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_rwlock_unlock)
		{
			return mt_record_dev_rwlock_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_intr_handler_register)
		{
			return mt_record_dev_intr_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_cpu_interrupt)
		{
			return mt_record_dev_intr_t{};
		}

	else if constexpr (MsgType == event_ACCL_ACTIVITY_kernel)
		{
			return mt_record_kernel_t{};
		}

	else if constexpr (MsgType == event_ACCL_API_UNKNOWN)
		{
			return record_activity_t{};
		}

	else if constexpr (MsgType == event_OMPT_Thread_Begin)
		{
			return ext_record_ompt_thread_begin_t{};
		}

	else if constexpr (MsgType == event_OMPT_Thread_End)
		{
			return ext_record_ompt_thread_end_t{};
		}

	else if constexpr (MsgType == event_OMPT_Parallel_Begin)
		{
			return ext_record_ompt_parallel_begin_t{};
		}

	else if constexpr (MsgType == event_OMPT_Parallel_End)
		{
			return ext_record_ompt_parallel_end_t{};
		}

	else if constexpr (MsgType == event_OMPT_Task_Create)
		{
			return ext_record_ompt_task_create_t{};
		}

	else if constexpr (MsgType == event_OMPT_Task_Schedule)
		{
			return ext_record_ompt_task_schedule_t{};
		}

	else if constexpr (MsgType == event_OMPT_Implicit_Task)
		{
			return ext_record_ompt_implicit_task_t{};
		}

	else if constexpr (MsgType == event_OMPT_Sync_Region_Wait)
		{
			return ext_record_ompt_sync_region_t{};
		}

	else if constexpr (MsgType == event_OMPT_Mutex_Released)
		{
			return ext_record_ompt_mutex_t{};
		}

	else if constexpr (MsgType == event_OMPT_Dependences)
		{
			return ext_record_ompt_dependences_t{};
		}

	else if constexpr (MsgType == event_OMPT_Task_Dependence)
		{
			return ext_record_ompt_task_dependence_t{};
		}

	else if constexpr (MsgType == event_OMPT_Work)
		{
			return ext_record_ompt_work_t{};
		}

	else if constexpr (MsgType == event_OMPT_Master)
		{
			return ext_record_ompt_master_t{};
		}

	else if constexpr (MsgType == event_OMPT_Sync_Region)
		{
			return ext_record_ompt_sync_region_t{};
		}

	else if constexpr (MsgType == event_OMPT_Lock_Init)
		{
			return ext_record_ompt_mutex_acquire_t{};
		}

	else if constexpr (MsgType == event_OMPT_Lock_Destroy)
		{
			return ext_record_ompt_mutex_t{};
		}

	else if constexpr (MsgType == event_OMPT_Mutex_Acquire)
		{
			return ext_record_ompt_mutex_acquire_t{};
		}

	else if constexpr (MsgType == event_OMPT_Mutex_Acquired)
		{
			return ext_record_ompt_mutex_t{};
		}

	else if constexpr (MsgType == event_OMPT_Nest_Lock)
		{
			return ext_record_ompt_nest_lock_t{};
		}

	else if constexpr (MsgType == event_OMPT_Flush)
		{
			return ext_record_ompt_flush_t{};
		}

	else if constexpr (MsgType == event_OMPT_Cancel)
		{
			return ext_record_ompt_cancel_t{};
		}

	else if constexpr (MsgType == event_Pthread_Create)
		{
			return record_pthread_create_t{};
		}

	else if constexpr (MsgType == event_Pthread_Join)
		{
			return record_pthread_join_t{};
		}

	else if constexpr (MsgType == event_Pthread_Detach)
		{
			return record_pthread_detach_t{};
		}

	else if constexpr (MsgType == event_Pthread_Exit)
		{
			return record_pthread_exit_t{};
		}

	else if constexpr (MsgType == event_Pthread_Mutex_Init)
		{
			return record_pthread_mutex_init_t{};
		}

	else if constexpr (MsgType == event_Pthread_Mutex_Destroy)
		{
			return record_pthread_mutex_destroy_t{};
		}

	else if constexpr (MsgType == event_Pthread_Mutex_Lock)
		{
			return record_pthread_mutex_lock_t{};
		}

	else if constexpr (MsgType == event_Pthread_Mutex_Trylock)
		{
			return record_pthread_mutex_trylock_t{};
		}

	else if constexpr (MsgType == event_Pthread_Mutex_Unlock)
		{
			return record_pthread_mutex_unlock_t{};
		}

	else if constexpr (MsgType == event_Pthread_Cond_Init)
		{
			return record_pthread_cond_init_t{};
		}

	else if constexpr (MsgType == event_Pthread_Cond_Destroy)
		{
			return record_pthread_cond_destroy_t{};
		}

	else if constexpr (MsgType == event_Pthread_Cond_Wait)
		{
			return record_pthread_cond_wait_t{};
		}

	else if constexpr (MsgType == event_Pthread_Cond_Timedwait)
		{
			return record_pthread_cond_timedwait_t{};
		}

	else if constexpr (MsgType == event_Pthread_Cond_Signal)
		{
			return record_pthread_cond_signal_t{};
		}

	else if constexpr (MsgType == event_Pthread_Cond_Broadcast)
		{
			return record_pthread_cond_broadcast_t{};
		}

        else
        {
            return record_t{};
        }
    }());
};

template <int MsgType>
using MacroToType = typename MacroToTypeHelper<MsgType>::type;

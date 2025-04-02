#include "dsp.h"

void dma_pull(void* local_dst, void* host_src, int unit_size, int step_num, int step_length){
    int dma_handle = dma_p2p(host_src,step_num,unit_size,step_length,local_dst,1,unit_size*step_num,0,0,0);
    dma_wait(dma_handle);
}
int dma_pull_async(void* local_dst, void* host_src, int unit_size, int step_num, int step_length){
    return dma_p2p(host_src,step_num,unit_size,step_length,local_dst,1,unit_size*step_num,0,0,0);
}
void dma_push(void* local_src, void* host_dst, int unit_size, int step_num, int step_length){
    int dma_handle = dma_p2p(local_src,1,step_num*unit_size,0,host_dst,step_num,unit_size,step_length,0,0);
    dma_wait(dma_handle);
}
int dma_push_async(void* local_src, void* host_dst, int unit_size, int step_num, int step_length){
    return dma_p2p(local_src,1,step_num*unit_size,0,host_dst,step_num,unit_size,step_length,0,0);
}

void dma_pull_sg(void* local_dst, void* host_src, int* idx, int unit_size, int step_num, int step_length){
    int dma_handle = dma_sg(host_src,idx,step_num,unit_size,step_length,local_dst,1,unit_size*step_num,0);
    dma_wait(dma_handle);
}
#include <alloc.h>
#include <barrier.h>
#include <defs.h>
#include <mram.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>

#include "Operator/dpu/LOOKUP.h"

__mram_noinit page_t buffer[NR_SINGLE_DPU_PAGE]; 
__host lookup_args DPU_INPUT_ARGUMENTS;

static void LOOKUP(T *src, T target, T *dst) {
  unsigned int itemNum = DPU_DMA_BFFR_BYTE / sizeof(T);
  dst[0] = 0;
  for (unsigned int i = 0; i < itemNum; i++) {
    if (src[i] == target)
      dst[0]++; // sentinel item, counting result number.
  }
}


int main(void) {
  unsigned int tasklet_id = me();
  uint32_t src1PageIdx = DPU_INPUT_ARGUMENTS.dpuTCB.src1PageIdx;
  uint32_t dstPageIdx =  DPU_INPUT_ARGUMENTS.dpuTCB.dstPageIdx;
  uint32_t pageCnt =     DPU_INPUT_ARGUMENTS.dpuTCB.pageCnt;
  uint32_t maxOffset =   pageCnt * PAGE_SIZE_BYTE;

  T target = DPU_INPUT_ARGUMENTS.target;

  T *cache_A = (T *)mem_alloc(DPU_DMA_BFFR_BYTE);
  T *cache_B = (T *)mem_alloc(DPU_DMA_BFFR_BYTE);

  __mram_ptr void const * src1PageBaseAddr = (__mram_ptr void const *)(&buffer[src1PageIdx]);
  __mram_ptr void const * dstPageBaseAddr = (__mram_ptr void const *)(&buffer[dstPageIdx]);

  for (unsigned int byte_index = DPU_DMA_BFFR_BYTE * tasklet_id;
       byte_index < maxOffset;
       byte_index += DPU_DMA_BFFR_BYTE * NR_TASKLETS) {

    __mram_ptr void const * mySrc1= src1PageBaseAddr + byte_index;
    __mram_ptr void const * myDst = dstPageBaseAddr + byte_index;
    
    mram_read(mySrc1, cache_A, DPU_DMA_BFFR_BYTE);
    LOOKUP(cache_A, target, cache_B);
    mram_write(cache_B, myDst, DPU_DMA_BFFR_BYTE);
  }

  return 0;
}

#include <alloc.h>
#include <barrier.h>
#include <defs.h>
#include <mram.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>

#include "Operator/dpu/common.h"

__mram_noinit page_t buffer[NR_SINGLE_DPU_PAGE];
__host DPU_TCB dpuTCB;

static void ELEW_PROD(T *src1, T *src2, T *dst) {
  unsigned int itemNum = DPU_DMA_BFFR_BYTE / sizeof(T);
  for (unsigned int i = 0; i < itemNum; i++) {
    dst[i] = src1[i] * src2[i];
  }
}

int main(void) {

  unsigned int tasklet_id = me();
  uint32_t src1PageIdx = dpuTCB.src1PageIdx;
  uint32_t src2PageIdx = dpuTCB.src2PageIdx;
  uint32_t dstPageIdx = dpuTCB.dstPageIdx;
  uint32_t pageCnt = dpuTCB.pageCnt;
  uint32_t maxOffset = pageCnt * PAGE_SIZE_BYTE;

  T *cache_A = (T *)mem_alloc(DPU_DMA_BFFR_BYTE);
  T *cache_B = (T *)mem_alloc(DPU_DMA_BFFR_BYTE);
  T *cache_C = (T *)mem_alloc(DPU_DMA_BFFR_BYTE);

  __mram_ptr void const *src1PageBaseAddr =
      (__mram_ptr void const *)(&buffer[src1PageIdx]);
  __mram_ptr void const *src2PageBaseAddr =
      (__mram_ptr void const *)(&buffer[src2PageIdx]);
  __mram_ptr void const *dstPageBaseAddr =
      (__mram_ptr void const *)(&buffer[dstPageIdx]);

  for (unsigned int byte_index = DPU_DMA_BFFR_BYTE * tasklet_id;
       byte_index < maxOffset; byte_index += DPU_DMA_BFFR_BYTE * NR_TASKLETS) {
    __mram_ptr void const *mySrc1 = src1PageBaseAddr + byte_index;
    __mram_ptr void const *mySrc2 = src2PageBaseAddr + byte_index;
    __mram_ptr void const *myDst = dstPageBaseAddr + byte_index;

    mram_read(mySrc1, cache_A, DPU_DMA_BFFR_BYTE);
    mram_read(mySrc2, cache_B, DPU_DMA_BFFR_BYTE);

    ELEW_PROD(cache_A, cache_B, cache_C);

    mram_write(cache_C, myDst, DPU_DMA_BFFR_BYTE);
  }

  return 0;
}

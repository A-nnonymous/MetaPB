#include <alloc.h>
#include <barrier.h>
#include <defs.h>
#include <mram.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>

#include "Operator/dpu/CONV_1D.h"

__mram_noinit page_t buffer[NR_SINGLE_DPU_PAGE];
__host conv_args DPU_INPUT_ARGUMENTS;

static void CONV_1D(T *dst, T *src, T *kernel, int kernelSize, int stride) {
  unsigned int itemNum = DPU_DMA_BFFR_BYTE / sizeof(T);
  size_t padding = (kernelSize - 1) / 2;
  // Perform convolution
  for (size_t i = 0; i < itemNum; ++i) {
    T sum = 0;
    for (size_t j = 0; j < kernelSize; ++j) {
      // Compute the input index, considering padding
      int inputIndex = i - padding + j;
      // Check if the input index is within bounds
      if (inputIndex >= 0 && inputIndex < itemNum) {
        sum += src[inputIndex] * kernel[j];
      }
    }
    dst[i] = sum;
  }
}

int main(void) {
  unsigned int tasklet_id = me();
  uint32_t src1PageIdx = DPU_INPUT_ARGUMENTS.dpuTCB.src1PageIdx;
  uint32_t dstPageIdx = DPU_INPUT_ARGUMENTS.dpuTCB.dstPageIdx;
  uint32_t pageCnt = DPU_INPUT_ARGUMENTS.dpuTCB.pageCnt;
  uint32_t maxOffset = pageCnt * PAGE_SIZE_BYTE;

  T kernel[8];
  int kernelSize = DPU_INPUT_ARGUMENTS.kernelSize;
  for (int i = 0; i < 8; i++) {
    kernel[i] = DPU_INPUT_ARGUMENTS.gaussianKernel[i];
  }
  int stride = DPU_INPUT_ARGUMENTS.stride;

  T *cache_A = (T *)mem_alloc(DPU_DMA_BFFR_BYTE);
  T *cache_B = (T *)mem_alloc(DPU_DMA_BFFR_BYTE);

  __mram_ptr void const *src1PageBaseAddr =
      (__mram_ptr void const *)(&buffer[src1PageIdx]);
  __mram_ptr void const *dstPageBaseAddr =
      (__mram_ptr void const *)(&buffer[dstPageIdx]);

  for (unsigned int byte_index = DPU_DMA_BFFR_BYTE * tasklet_id;
       byte_index < maxOffset; byte_index += DPU_DMA_BFFR_BYTE * NR_TASKLETS) {

    __mram_ptr void const *mySrc1 = src1PageBaseAddr + byte_index;
    __mram_ptr void const *myDst = dstPageBaseAddr + byte_index;

    mram_read(mySrc1, cache_A, DPU_DMA_BFFR_BYTE);
    CONV_1D(cache_B, cache_A, kernel, kernelSize, stride);
    mram_write(cache_B, myDst, DPU_DMA_BFFR_BYTE);
  }
  return 0;
}

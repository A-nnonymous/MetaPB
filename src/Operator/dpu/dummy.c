#include <mram.h>
#include "Operator/dpu/common.h"

__mram_noinit page_t buffer[NR_SINGLE_DPU_PAGE]; 

int main() {
  // avoid optimizing
  T *cache_A = (T *)mem_alloc(DPU_DMA_BFFR_BYTE);
  mram_read((__mram_ptr void const*)(&buffer[0]), cache_A, DPU_DMA_BFFR_BYTE);
  return 0;
}

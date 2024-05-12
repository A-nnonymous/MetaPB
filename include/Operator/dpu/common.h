#ifndef COMMON_H
#define COMMON_H

#ifndef NR_TASKLETS
#define NR_TASKLETS 12
#endif

#define PAGE_SIZE_BYTE 4096
#define DPU_DMA_BFFR_BYTE 1024
#define NR_SINGLE_DPU_PAGE 15360

#ifndef __cplusplus
#define T int
#endif

typedef char page_t[PAGE_SIZE_BYTE];

// Task Control Block
typedef struct DPU_TCB {
  unsigned int src1PageIdx;
  unsigned int src2PageIdx;
  unsigned int dstPageIdx;
  unsigned int pageCnt;
} DPU_TCB;

#endif

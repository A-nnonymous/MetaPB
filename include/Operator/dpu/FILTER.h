#include "Operator/dpu/common.h"
typedef struct {
  DPU_TCB_c dpuTCB;
  float gaussianKernel[8];
  int stride;
  int kernelSize;
} filter_args;

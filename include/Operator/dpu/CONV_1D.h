#include "Operator/dpu/common.h"
typedef struct {
  DPU_TCB dpuTCB;
  float gaussianKernel[8];
  int stride;
  int kernelSize;
} conv_args;

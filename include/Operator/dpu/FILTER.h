#include "Operator/dpu/common.h"
typedef struct {
  common_args co;
  float gaussianKernel[8];
  int stride;
  int kernelSize;
} filter_args;
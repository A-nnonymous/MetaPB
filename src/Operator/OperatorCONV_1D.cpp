#include "Operator/OperatorCONV_1D.hpp"

namespace MetaPB {
namespace Operator {
inline void OperatorCONV_1D::execCPU(const size_t batchSize_MiB,
                                     void **memPoolBffrPtrs) const noexcept {
  size_t inputSize = batchSize_MiB * 1024 * 1024 / sizeof(float);
  size_t padding = (kernelSize - 1) / 2;

  // Assuming memPoolBffrPtrs is an array of pointers to float
  float *inputBuffer = static_cast<float *>(memPoolBffrPtrs[0]);
  float *outputBuffer = static_cast<float *>(memPoolBffrPtrs[1]);

  // Perform convolution
  for (size_t i = 0; i < inputSize; ++i) {
    float sum = 0.0f;
    for (size_t j = 0; j < kernelSize; ++j) {
      // Compute the input index, considering padding
      int inputIndex = i - padding + j;
      // Check if the input index is within bounds
      if (inputIndex >= 0 && inputIndex < inputSize) {
        sum += inputBuffer[inputIndex] * gaussianKernel[j];
      }
    }
    outputBuffer[i] = sum;
  }
}

inline void
OperatorCONV_1D::execDPU(const size_t batchSize_MiB) const noexcept {
  auto DPU_BINARY = getDPUBinaryPath();
  DPU_ASSERT(dpu_load(allDPUs, DPU_BINARY.c_str(), NULL));
  uint32_t nr_of_dpus;
  DPU_ASSERT(dpu_get_nr_dpus(allDPUs, &nr_of_dpus));
  size_t bytes = batchSize_MiB * (1 << 20);
  if(bytes == 0)return; //essential for not being overflowed
  const size_t input_size_dpu =
      divceil(bytes, nr_of_dpus); // Input size per DPU (max.)
  const size_t input_size_dpu_8bytes =
      (input_size_dpu % 8) != 0
          ? roundup(input_size_dpu, 8)
          : input_size_dpu; // Input size per DPU (max.), 8-byte aligned

  // Copy input arrays
  conv_args args;
  for (int i = 0; i < 8; i++) {
    args.gaussianKernel[i] = this->gaussianKernel[i];
  }
  args.stride = this->stride;
  args.co.inputSize = input_size_dpu * getInputTensorNum();
  args.co.operandNum = 1;
  args.kernelSize = this->kernelSize;

  DPU_ASSERT(dpu_broadcast_to(allDPUs, "DPU_INPUT_ARGUMENTS", 0, &args,
                              sizeof(args), DPU_XFER_DEFAULT));
  DPU_ASSERT(dpu_launch(allDPUs, DPU_SYNCHRONOUS));
  return;
}

} // namespace Operator
} // namespace MetaPB

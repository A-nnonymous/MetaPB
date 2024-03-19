#include "Operator/OperatorEUDIST.hpp"

namespace MetaPB {
namespace Operator {

inline void OperatorEUDIST::execCPU(const size_t batchSize_MiB,
                                    void **memPoolBffrPtrs) const noexcept {
  size_t inputSize = batchSize_MiB * 1024 * 1024 / sizeof(float);
  float *src1 = static_cast<float *>(memPoolBffrPtrs[0]);
  float *src2 = static_cast<float *>(memPoolBffrPtrs[1]);
  float *dst = static_cast<float *>(memPoolBffrPtrs[2]);
#pragma omp parallel for
  for (int i = 0; i < inputSize; i++) {
    // ignoring sqrt because DPU doesn't have hardware sqrt
    dst[i] += (src1[i] - src2[i]) * (src1[i] - src2[i]);
  }
}
inline void OperatorEUDIST::execDPU(const size_t batchSize_MiB) const noexcept {
  auto DPU_BINARY = getDPUBinaryPath();
  DPU_ASSERT(dpu_load(allDPUs, DPU_BINARY.c_str(), NULL));
  uint32_t nr_of_dpus;
  DPU_ASSERT(dpu_get_nr_dpus(allDPUs, &nr_of_dpus));
  size_t bytes = batchSize_MiB * (1 << 20);
  if (bytes == 0)
    return; // essential for not being overflowed
  const size_t input_size_dpu =
      divceil(bytes, nr_of_dpus); // Input size per DPU (max.)
  const size_t input_size_dpu_8bytes =
      (input_size_dpu % 8) != 0
          ? roundup(input_size_dpu, 8)
          : input_size_dpu; // Input size per DPU (max.), 8-byte aligned

  // Copy input arrays
  common_args args;
  args.inputSize = getInputTensorNum() * input_size_dpu_8bytes;
  args.operandNum = getInputTensorNum();

  DPU_ASSERT(dpu_broadcast_to(allDPUs, "DPU_INPUT_ARGUMENTS", 0, &args,
                              sizeof(args), DPU_XFER_DEFAULT));
  DPU_ASSERT(dpu_launch(allDPUs, DPU_SYNCHRONOUS));
  return;
}

} // namespace Operator
} // namespace MetaPB

#include "Operator/OperatorMAP.hpp"

namespace MetaPB {
namespace Operator {

inline void OperatorMAP::execCPU(const size_t batchSize_MiB,
                                 void **memPoolBffrPtrs) const noexcept {
  // load a dummy dpu program that do nothing.
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

  int i = 0;
  dpu_set_t dpu;
  DPU_FOREACH(allDPUs, dpu, i) {
    DPU_ASSERT(dpu_prepare_xfer(dpu, memPoolBffrPtrs[0] + i * input_size_dpu));
  }
  DPU_ASSERT(dpu_push_xfer(allDPUs, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME,
                           0, input_size_dpu_8bytes, DPU_XFER_DEFAULT));
}

} // namespace Operator
} // namespace MetaPB

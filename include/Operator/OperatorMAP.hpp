#ifndef OP_MAP_HPP
#define OP_MAP_HPP

#include "Operator/OperatorBase.hpp"

namespace MetaPB {
namespace Operator {

class OperatorMAP : public OperatorBase {
public:
  OperatorMAP() {}

  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }

  inline virtual void execCPU(const size_t batchSize_MiB,
                              void **memPoolBffrPtrs) const noexcept override {
      uint32_t nr_of_dpus;
      DPU_ASSERT(dpu_get_nr_dpus(allDPUs, &nr_of_dpus));
      size_t bytes = batchSize_MiB * (1<<20);

      const size_t input_size_dpu =
          divceil(bytes, nr_of_dpus); // Input size per DPU (max.)
      const size_t input_size_dpu_8bytes =
          (input_size_dpu% 8) != 0
              ? roundup(input_size_dpu, 8)
              : input_size_dpu; // Input size per DPU (max.), 8-byte aligned

      int i = 0;
      dpu_set_t dpu;
      DPU_FOREACH(allDPUs, dpu, i) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, memPoolBffrPtrs[0] + i * input_size_dpu));
      }
      DPU_ASSERT(
          dpu_push_xfer(allDPUs, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0,
                  input_size_dpu, DPU_XFER_DEFAULT));          
    }
  inline virtual void
  execDPU(const size_t batchSize_MiB) const noexcept override {}

  virtual inline constexpr bool checkIfIsTrainable() const noexcept override {
    return true;
  }
  virtual inline constexpr bool checkIfIsCPUOnly() const noexcept override {
    return true;
  }

private:
  inline static const std::string OpName = "MAP";
};
} // namespace Operator
} // namespace MetaPB
#endif

#ifndef OP_LOOKUP_HPP
#define OP_LOOKUP_HPP

#include "Operator/OperatorBase.hpp"

namespace MetaPB {
namespace Operator {

class OperatorLOOKUP : public OperatorBase {
public:
  OperatorLOOKUP() {}

  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }

  inline virtual void execCPU(const size_t batchSize_MiB,
                              void **memPoolBffrPtrs) const noexcept override {
    size_t inputSize = batchSize_MiB * 1024 * 1024 / sizeof(float);
    float* src1 = static_cast<float*>(memPoolBffrPtrs[0]);
    float* src2 = static_cast<float*>(memPoolBffrPtrs[1]);
    float* dst = static_cast<float*>(memPoolBffrPtrs[2]);
    size_t dstIdx = 0;
    #pragma omp parallel for
    for(int i = 0; i < inputSize; i++){
      if(src1[i] == src2[i])dst[dstIdx++] = src1[i];
    }
  }
  inline virtual void
  execDPU(const size_t batchSize_MiB) const noexcept override {}

  virtual inline constexpr bool checkIfIsTrainable() const noexcept override {
    return true;
  }
  virtual inline constexpr bool checkIfIsCPUOnly() const noexcept override {
    return false;
  }

private:
  inline static const std::string OpName = "LOOKUP";
};
} // namespace Operator
} // namespace MetaPB
#endif

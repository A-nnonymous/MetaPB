#ifndef OP_MAC_HPP
#define OP_MAC_HPP

#include "Operator/OperatorBase.hpp"

namespace MetaPB {
namespace Operator {

class OperatorMAC : public OperatorBase {
public:
  OperatorMAC() {}

  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }

  inline virtual void execCPU(const size_t batchSize_MiB,
                              void **memPoolBffrPtrs) const noexcept override {
    size_t inputSize = batchSize_MiB * 1024 * 1024 / sizeof(float);
    float* src1 = static_cast<float*>(memPoolBffrPtrs[0]);
    float* src2 = static_cast<float*>(memPoolBffrPtrs[1]);
    size_t dstIdx = 0;
    #pragma omp parallel for
    for(int i = 0; i < inputSize; i++){
      src1[i] = src1[i] * weight + src2[i];
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
  const float weight = 2.5f;
  inline static const std::string OpName = "MAC";
};
} // namespace Operator
} // namespace MetaPB
#endif

#ifndef OP_DOT_PROD_HPP
#define OP_DOT_PROD_HPP

#include "Operator/OperatorBase.hpp"

namespace MetaPB {
namespace Operator {

class OperatorDOT_PROD : public OperatorBase {
public:
  OperatorDOT_PROD() {}

  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }

  inline virtual void execCPU(const size_t batchSize_MiB,
                              void **memPoolBffrPtrs) const noexcept override {
    size_t inputSize = batchSize_MiB * 1024 * 1024 / sizeof(float);
    float* src1 = static_cast<float*>(memPoolBffrPtrs[0]);
    float* src2 = static_cast<float*>(memPoolBffrPtrs[1]);
    float* dst = static_cast<float*>(memPoolBffrPtrs[2]);
    #pragma omp parallel for
    for(int i = 0; i < inputSize; i++){
      dst[i] = src1[i] * src2[i];
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
  inline static const std::string OpName = "DOT_PROD";
};
} // namespace Operator
} // namespace MetaPB
#endif

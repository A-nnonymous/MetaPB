#ifndef OP_AFFINE_HPP
#define OP_AFFINE_HPP

extern "C" {
#include "Operator/dpu/AFFINE.h"
}
#include "Operator/OperatorBase.hpp"

namespace MetaPB {
namespace Operator {

class OperatorAFFINE : public OperatorBase {
public:
  OperatorAFFINE(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR)
      : OperatorBase(g_DPU_MGR) {}

  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }
  inline virtual constexpr int getInputTensorNum() const noexcept override {
    return 2;
  }
  virtual void execCPU(const size_t batchSize_MiB,
                       void **memPoolBffrPtrs) const noexcept override;
  virtual void execDPU(const size_t batchSize_MiB) const noexcept override;

  virtual inline constexpr bool checkIfIsTrainable() const noexcept override {
    return true;
  }
  virtual inline constexpr bool checkIfIsCPUOnly() const noexcept override {
    return false;
  }

private:
  const float weight = 2.5f;
  inline static const std::string OpName = "AFFINE";
};
} // namespace Operator
} // namespace MetaPB
#endif

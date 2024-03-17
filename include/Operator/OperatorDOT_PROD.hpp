#ifndef OP_DOT_PROD_HPP
#define OP_DOT_PROD_HPP

#include "Operator/OperatorBase.hpp"
#include "Operator/dpu/common.h"

namespace MetaPB {
namespace Operator {

class OperatorDOT_PROD : public OperatorBase {
public:
  OperatorDOT_PROD(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR)
      : OperatorBase(g_DPU_MGR) {}
  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }
  inline virtual constexpr int getInputTensorNum() const noexcept override {
    return 2;
  }
  virtual void execCPU(const size_t batchSize_MiB,
                              void **memPoolBffrPtrs) const noexcept override;
  virtual void
  execDPU(const size_t batchSize_MiB) const noexcept override;

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

#ifndef OP_FILTER_HPP
#define OP_FILTER_HPP

extern "C" {
#include "Operator/dpu/FILTER.h"
}

#include "Operator/OperatorBase.hpp"

using MetaPB::Operator::OperatorBase;
namespace MetaPB {
namespace Operator {

class OperatorFILTER: public OperatorBase {
public:
  OperatorFILTER(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR)
      : OperatorBase(g_DPU_MGR) {}
  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }
  inline virtual constexpr int getInputTensorNum() const noexcept override {
    return 1;
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
  const int kernelSize = 8;
  const int stride = 1;
  const float gaussianKernel[8] = {
      0.000872710786525902, 0.0175288647260302,   0.129521764811203,
      0.352076659676240,    0.352076659676240,    0.129521764811203,
      0.0175288647260302,   0.000872710786525902,
  };

  inline static const std::string OpName = "FILTER";
};
} // namespace Operator
} // namespace MetaPB
#endif

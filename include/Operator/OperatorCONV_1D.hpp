#ifndef OP_CONV_HPP
#define OP_CONV_HPP

extern "C" {
#include "Operator/dpu/CONV_1D.h"
}

#include "Operator/OperatorBase.hpp"

using MetaPB::Operator::OperatorBase;
namespace MetaPB {
namespace Operator {

class OperatorCONV_1D : public OperatorBase {
public:
  OperatorCONV_1D(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR)
      : OperatorBase(g_DPU_MGR) {}
  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }
  inline virtual constexpr int getInputTensorNum() const noexcept override {
    return 1;
  }
  virtual void execCPU(const CPU_TCB& cpuTCB) const noexcept override;

  virtual void execDPU(const DPU_TCB& dpuTCB) const noexcept override;

  virtual inline constexpr bool checkIfIsTrainable() const noexcept override {
    return true;
  }
  virtual inline constexpr bool checkIfIsCPUOnly() const noexcept override {
    return false;
  }

private:
  const int kernelSize = 8;
  const int stride = 1;
  /*
  const float gaussianKernel[8] = {
      0.000872710786525902, 0.0175288647260302,   0.129521764811203,
      0.352076659676240,    0.352076659676240,    0.129521764811203,
      0.0175288647260302,   0.000872710786525902,
  };
  */
  const float gaussianKernel[8] = {
      8.72710786525902, 1.75288647260302,   1.29521764811203,
      3.52076659676240,  3.52076659676240,    1.29521764811203,
      1.75288647260302,   8.72710786525902,
  };

  inline static const std::string OpName = "CONV_1D";
};
} // namespace Operator
} // namespace MetaPB
#endif

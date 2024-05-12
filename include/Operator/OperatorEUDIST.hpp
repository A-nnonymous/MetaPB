#ifndef OP_EUDIST_HPP
#define OP_EUDIST_HPP

#include "Operator/OperatorBase.hpp"
#include "Operator/dpu/common.h"

namespace MetaPB {
namespace Operator {

class OperatorEUDIST : public OperatorBase {
public:
  OperatorEUDIST(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR)
      : OperatorBase(g_DPU_MGR) {}
  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }
  inline virtual constexpr int getInputTensorNum() const noexcept override {
    return 2;
  }
  virtual void execCPU(const CPU_TCB &cpuTCB) const noexcept override;
  virtual void execDPU(const DPU_TCB &dpuTCB) const noexcept override;

  virtual inline constexpr bool checkIfIsTrainable() const noexcept override {
    return true;
  }
  virtual inline constexpr bool checkIfIsCPUOnly() const noexcept override {
    return false;
  }

private:
  inline static const std::string OpName = "EUDIST";
};
} // namespace Operator
} // namespace MetaPB
#endif

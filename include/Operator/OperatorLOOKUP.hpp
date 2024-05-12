#ifndef OP_LOOKUP_HPP
#define OP_LOOKUP_HPP

extern "C" {
#include "Operator/dpu/LOOKUP.h"
}
#include "Operator/OperatorBase.hpp"

namespace MetaPB {
namespace Operator {

class OperatorLOOKUP : public OperatorBase {
public:
  OperatorLOOKUP(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR)
      : OperatorBase(g_DPU_MGR) {}
  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }
  inline virtual constexpr int getInputTensorNum() const noexcept override {
    return 1;
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
  inline static const float target = 2.5f;
  inline static const std::string OpName = "LOOKUP";
};
} // namespace Operator
} // namespace MetaPB
#endif

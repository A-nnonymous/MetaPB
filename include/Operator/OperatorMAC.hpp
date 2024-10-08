#ifndef OP_MAC_HPP
#define OP_MAC_HPP

extern "C" {
#include "Operator/dpu/MAC.h"
}
#include "Operator/OperatorBase.hpp"

namespace MetaPB {
namespace Operator {

class OperatorMAC : public OperatorBase {
public:
  OperatorMAC(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR)
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
  const float weight = 2.5f;
  inline static const std::string OpName = "MAC";
};
} // namespace Operator
} // namespace MetaPB
#endif

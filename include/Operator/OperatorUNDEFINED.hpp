#ifndef OP_UNDEFINED_HPP
#define OP_UNDEFINED_HPP

#include "Operator/OperatorBase.hpp"

namespace MetaPB {
namespace Operator {

class OperatorUNDEFINED : public OperatorBase {
public:
  OperatorUNDEFINED(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR)
      : OperatorBase(g_DPU_MGR) {}
  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }
  inline virtual constexpr int getInputTensorNum() const noexcept override {
    return 0;
  }
  inline virtual void execCPU(const CPU_TCB&) const noexcept override {}
  inline virtual void
  execDPU(const DPU_TCB&) const noexcept override {}

  virtual inline constexpr bool checkIfIsTrainable() const noexcept override {
    return false;
  }
  virtual inline constexpr bool checkIfIsCPUOnly() const noexcept override {
    return true;
  }

private:
  inline static const std::string OpName = "UNDEFINED";
};
} // namespace Operator
} // namespace MetaPB
#endif

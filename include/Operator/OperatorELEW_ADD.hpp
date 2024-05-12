#ifndef OP_ELEW_ADD_HPP
#define OP_ELEW_ADD_HPP

#include "Operator/OperatorBase.hpp"
extern "C" {
#include "Operator/dpu/common.h"
}
#include <omp.h>

using MetaPB::Operator::OperatorBase;
namespace MetaPB {
namespace Operator {

class OperatorELEW_ADD : public OperatorBase {
public:
  OperatorELEW_ADD(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR)
      : OperatorBase(g_DPU_MGR) {}
  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }
  inline virtual constexpr int getInputTensorNum() const noexcept override {
    return 2;
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
  inline static const std::string OpName = "ELEW_ADD";
};
} // namespace Operator
} // namespace MetaPB
#endif

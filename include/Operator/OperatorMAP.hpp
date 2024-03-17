#ifndef OP_MAP_HPP
#define OP_MAP_HPP

#include "Operator/OperatorBase.hpp"

namespace MetaPB {
namespace Operator {

class OperatorMAP : public OperatorBase {
public:
  OperatorMAP(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR)
      : OperatorBase(g_DPU_MGR) {}
  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }
  inline virtual constexpr int getInputTensorNum() const noexcept override {
    return 1;
  }
  virtual void execCPU(const size_t batchSize_MiB,
                       void **memPoolBffrPtrs) const noexcept override;
  inline virtual void
  execDPU(const size_t batchSize_MiB) const noexcept override {}

  virtual inline constexpr bool checkIfIsTrainable() const noexcept override {
    return true;
  }
  virtual inline constexpr bool checkIfIsCPUOnly() const noexcept override {
    return true;
  }

private:
  inline static const std::string OpName = "MAP";
};
} // namespace Operator
} // namespace MetaPB
#endif

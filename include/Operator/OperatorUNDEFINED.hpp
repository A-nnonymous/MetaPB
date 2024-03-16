#ifndef OP_UNDEFINED_HPP
#define OP_UNDEFINED_HPP

#include "Operator/OperatorBase.hpp"

namespace MetaPB {
namespace Operator {

class OperatorUNDEFINED : public OperatorBase {
public:
  OperatorUNDEFINED() {}

  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }

  inline virtual void execCPU(const size_t batchSize_MiB,
                              void **memPoolBffrPtrs) const noexcept override {}
  inline virtual void
  execDPU(const size_t batchSize_MiB) const noexcept override {}

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

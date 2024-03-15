/*
#ifndef OP_LOGIC_START_HPP
#define OP_LOGIC_START_HPP

#include "Operator/OperatorBase.hpp"

namespace MetaPB {
namespace Operator {

class Operator : public  OperatorBase {
public:
  Operator() = default;

  virtual void execCPU(const size_t batchSize_MiB)override;
  virtual void execDPU(const size_t batchSize_MiB)override;

private:
  inline static const std::string OpName = "";
};

} // namespace Operator
} // namespace MetaPB
#endif

*/
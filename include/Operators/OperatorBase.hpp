#ifndef OP_BASE_HPP
#define OP_BASE_HPP
#include "Operator/Datablock.hpp"
#include <unordered_map>
#include <vector>

using std::unordered_map;
using std::vector;
namespace MetaPB {
namespace Operator {

class OperatorBase {
  enum execPlace { CPU, DPU };

public:
  OperatorBase() = delete;

  /// @brief Execute the operator
  /// @param t Type of executor decided by upper level scheduler.
  virtual void exec() = 0;

private:
  unordered_map<> vector<Datablock *> inputBlocks;
  OperatorBase *nextOperator
};

} // namespace Operator
} // namespace MetaPB

#endif

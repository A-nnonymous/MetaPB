#pragma once
#include <map>
#include <vector>
#include "utils/paramDef.hpp"

namespace scheduler{

class OperatorBase{
public:
  OperatorBase()=delete;

  /// @brief Execute the operator
  /// @param t Type of executor decided by upper level scheduler.
  virtual void exec() = 0;

  /// @brief 
  /// @param part 
  /// @return 
  virtual std::vector<OperatorBase*> split(size_t part) = 0;

private:

};

} // namespace scheduler
// TODO:
//  Operator:
//  1. enum class OperatorTag
//  2. operator factory with tagged construction.
//  3. universal virtual function that transfer id to subOperator.
//  4. Operator::modelize(opTag, rangeMax)
//  5. struct Operator::Scaletag with function reduce(set<Operator::ScaleTag>)
//    invokes overrided operator+ of each operator.

#ifndef TASK_HPP
#define TASK_HPP
#include "Operator/OperatorRegistry.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <map>
#include <string>
#include <vector>

using std::list;
using std::map;
using std::set;
using std::string;
using std::vector;

namespace MetaPB {
using Operator::OperatorTag;
using Operator::OperatorType;
namespace Executor {

typedef struct TaskProperties {
  OperatorTag op = OperatorTag::UNDEFINED;
  OperatorType opType = OperatorType::Undefined;
  size_t inputSize_MiB = 0;
  std::string color = "blue";
  std::string name = "N/A";
  // ----- Schedule adjust zone ------
  bool isCPUOnly = false;
  bool isDPUOnly = false;
  double offloadRatio = 0.0f;
  // ----- Schedule adjust zone ------
} TaskProperties;

} // namespace Executor
} // namespace MetaPB

#endif

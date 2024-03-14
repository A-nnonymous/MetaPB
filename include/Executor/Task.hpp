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
#include "Scheduler/SchedulerManager.hpp"
#include "Operator/OperatorManager.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <utils/ChronoTrigger.hpp>
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
using utils::ChronoTrigger;
using Regressor::perfStats;

namespace Executor {

typedef struct TaskProperties {
  OperatorTag op = OperatorTag::UNDEFINED;
  OperatorType opType = OperatorType::Undefined;
  size_t inputSize = 0;
  std::string color = "blue";
  std::string name = "N/A";
  // ----- Schedule adjust zone ------
  bool isCPUOnly = false;
  bool isDPUOnly = false;
  double offloadRatio = 0.0f;
  // ----- Schedule adjust zone ------
} TaskProperties;

class Task {
public:
  Task(TaskProperties tpIn): tp(tpIn){}
  // --------------- MetaScheduler interaction -------------
  // deduce task time&energy consume in pure compute sight.
  perfStats deduceMetrics(const double offloadRatio) const noexcept;
  // ---------------------- Execution ----------------------
  void repeatExec(const string &, const Schedule &, int, int) const noexcept;
  void exec(const Schedule &) const noexcept;
  void execAsync(const Schedule &) const noexcept;
  // ------------------ Getter & Setter --------------------
  // reduced operator scaleArg maximum range.
  map<OperatorTag, ScaleTag> getTrainRequirement() const noexcept;
  void dumpAllReports(const string) const noexcept;

private:
  TaskProperties tp;
  OperatorRegistry reg;
  ChronoTrigger ct;
};
} // namespace Executor
} // namespace MetaPB

#endif

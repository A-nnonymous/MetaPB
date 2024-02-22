//TODO: 
//  Operator:
  //  1. enum class OperatorTag
  //  2. operator factory with tagged construction.
  //  3. universal virtual function that transfer id to subOperator.
  //  4. Operator::modelize(opTag, rangeMax)
  //  5. struct Operator::Scaletag with function reduce(set<Operator::ScaleTag>) 
  //    invokes overrided operator+ of each operator.
 
#ifndef TASK_HPP
#define TASK_HPP
#include <set>
#include <string>
#include <vector>
#include <queue>
#include <utils/ChronoTrigger.hpp>

using std::set;
using std::string;
using std::vector;
using std::list;

namespace MetaPB {
namespace Executor {

class Task {
  using MetaScheduler::Schedule;
  using MetaPB::Operator::OperatorTag;
  using MetaPB::Operator::ScaleTag;
  ChronoTrigger ct;
public:
  typedef map<OperatorTag, set<scaleTags>> OperatorRegistry;

  static vector<Task> instantiateFromPath(const string&) noexcept;
  // --------------- MetaScheduler interaction -------------
  vector<Schedule> randSchedule(const size_t batchSize) const noexcept;
  double deduceTime_ns(const Schedule &) const noexcept;
  double deduceEnergy_joule(const Schedule &) const noexcept;

  // ---------------------- Execution ----------------------
  void test(const string&, const Schedule&, int, int)const noexcept;

  void exec(const Schedule &) const noexcept;
  void execAsync(const Schedule &) const noexcept;

  // ------------------ Getter & Setter --------------------
  // reduced operator scaleArg maximum range.
  map<OperatorTag, scaleArgs> getTrainRequirement()const noexcept;
  set<OperatorSign> getOpSignSet()const noexcept; 
  void dumpAllReports(const string)const noexcept;

private:
  // ------------------ Registry area ---------------------
  OperatorRegistry reg;
  queue<Operator> workQueue;
  map<Operator*, double> sched2Operator;
};
} // namespace Executor
} // namespace MetaPB

#endif

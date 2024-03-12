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
#include <MetaScheduler/MetaScheduler.hpp>
#include <Operators/OperatorManager.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <utils/ChronoTrigger.hpp>
#include <vector>

using std::list;
using std::map;
using std::set;
using std::string;
using std::vector;

namespace MetaPB {
namespace Executor {

class Task {
  using Schedule = MetaPB::MetaScheduler::Schedule;
  using OperatorTag = MetaPB::Operator::OperatorTag;
  using ScaleTag = MetaPB::Operator::ScaleTag;
  using ChronoTrigger = MetaPB::utils::ChronoTrigger;

public:
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS>
      Graph;
  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef boost::graph_traits<Graph>::edge_descriptor Edge;
  typedef map<OperatorTag, set<ScaleTag>> OperatorRegistry;
  typedef struct Load {
    OperatorTag op;
    string name;

  } Load;

  Task();
  Task(const Graph);
  // --------------- MetaScheduler interaction -------------
  vector<Schedule> randSchedule(const size_t batchSize) const noexcept;
  double deduceTime_ns(const Schedule &) const noexcept;
  double deduceEnergy_joule(const Schedule &) const noexcept;

  // ---------------------- Execution ----------------------
  void repeatExec(const string &, const Schedule &, int, int) const noexcept;
  void exec(const Schedule &) const noexcept;
  void execAsync(const Schedule &) const noexcept;
  // ------------------ Getter & Setter --------------------
  // reduced operator scaleArg maximum range.
  map<OperatorTag, ScaleTag> getTrainRequirement() const noexcept;
  void dumpAllReports(const string) const noexcept;

private:
  OperatorRegistry reg;
  ChronoTrigger ct;
  Graph g;
  map<Vertex, Load>
};
} // namespace Executor
} // namespace MetaPB

#endif

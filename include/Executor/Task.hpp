#ifndef TASK_HPP
#define TASK_HPP
#include <set>
#include <string>
#include <vector>

using std::set;
using std::string;
using std::vector;

namespace MetaPB {
namespace Executor {

class Task {
  typedef vector<double> Schedule;
  typedef vector<Schedule> ScheduleVec;

  typedef string OperatorTag; 
  typedef string OperatorSign; 


public:
  static vector<Task> instantiateFromPath(const string&) noexcept;

  // --------------- MetaScheduler interaction -------------
  void modelize(const string &modelCachePath) noexcept;
  ScheduleVec randSchedule(const size_t batchSize) const noexcept;
  double deduceTime_ns(const Schedule &) const noexcept;
  double deduceEnergy_joule(const Schedule &) const noexcept;


  // ---------------------- Execution ----------------------
  void exec(const Schedule &) const noexcept;
  void execAsync(const Schedule &) const noexcept;

  // ------------------ Getter & Setter --------------------
  void setTaskByTXT(const string&);
  set<OperatorTag> getOpTagSet();
  set<OperatorSign> getOpSignSet(); 

private:
  set<OperatorSign> opSignSet; 
  set<OperatorTag> opTagSet;
};
} // namespace Executor
} // namespace MetaPB

#endif

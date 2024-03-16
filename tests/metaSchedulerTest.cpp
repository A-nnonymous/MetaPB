#include "MetaScheduler/MetaScheduler.hpp"
#include <map>

using MetaPB::Executor::Task;
using MetaPB::MetaScheduler::MetaScheduler;
using MetaPB::MetaScheduler::perfCost;
using std::map;
using std::string;
using std::vector;

static auto scheduler = MetaScheduler();
static int warmup = 5;
static int repeat = 10;

enum class scheduleType {
  pure_CPU,
  pure_DPU,
  energy_sensitive,
  time_sensitive,
  hybrid
};

static map<scheduleType, string> sType2Str = {
    {scheduleType::time_sensitive, "Time_Sensitive_Schedule"},
    {scheduleType::energy_sensitive, "Energy_Sensitive_Schedule"},
    {scheduleType::hybrid, "Hybrid_Schedule"},
    {scheduleType::pure_CPU, "All_CPU_Schedule"},
    {scheduleType::pure_DPU, "All_DPU_Schedule"},
};

void scheduleAndCheck(const Task &task, const scheduleType sType) noexcept {
  vector<double> schedule;
  perfCost estimatCost;

  switch (sType) {
  case scheduleType::pure_CPU:
    [ schedule, estimateCost ] = scheduler.scheduleAllCPU(task);
    break;
  case scheduleType::pure_DPU:
    [ schedule, estimateCost ] = scheduler.scheduleAllDPU(task);
    break;
  case scheduleType::energy_sensitive:
    [ schedule, estimateCost ] = scheduler.schedule(task.0.0f, 1.0f);
    break;
  case scheduleType::time_sensitive:
    [ schedule, estimateCost ] = scheduler.schedule(task, 1.0f, 0.0f);
    break;
  case scheduleType::hybrid:
    [ schedule, estimateCost ] = scheduler.schedule(task, 0.5f, 0.5f);
    break;
  case default:
    break;
  }

  // using ChronoTrigger to truely execute and gather metrics
  task.test(sType2Str[sType], schedule, warmup, repeat);
}

int main() {
  int warmup = 5;
  int repeat = 10;
  string outputPath = "./";
  auto benchmarkTasks = Task::instantiateFromPath(...);
  MetaScheduler::cachedModelize(benchmarkTasks);

  for (const auto &task : benchmarkTasks) {
    for (const auto &[sType, sName] : sType2Str) {
      scheduleAndCheck(task, sType);
    }
    task.dumpAllReports(outputPath + "/" + task.name());
  }
  return 0;
}

#include "MetaScheduler/MetaScheduler.hpp"

//TODO:
//  MetaScheduler:
  //  1. static method "cachedModelize"
  //  that extract regression modeling range of all tasks and performs it.
  //  2. Method "schedule..." with Alpha and Beta or naive.
// Task:
  //  0. cost structure that combines time cost and energy cost.
  //  1. Method "test" that using ChronoTrigger and reserve report, returning
  //  schedule vector and cost estimation at same time.
  //  2. Method "dumpAllReports" that dump tagged report into given place.

using std::vector;
using std::string;
using MetaPB::MetaScheduler::MetaScheduler;
using MetaPB::Executor::Task;

void fitnessCheck(const perfCost& lhs, const perfCost& rhs) noexcept{

}

int main(){
  int warmup = 5;
  int repeat = 10;
  string outputPath = "./";
  auto benchmarkTasks = Task::instantiateFromPath(...);

  vector<double> timeConsume_second;
  vector<double> energyConsume_joule;
  

  auto scheduler = MetaScheduler();
  MetaScheduler::cachedModelize(benchmarkTasks);

  for(const auto& task : benchmarkTasks){
    const auto [allCPUSchedule, cost_allCPUSchedule_estimate] = 
      scheduler.scheduleAllCPU(task);
    const auto [allDPUSchedule, cost_allDPUSchedule_estimate] = 
      scheduler.scheduleAllDPU(task);
    const auto [tSchedule, cost_tSchedule_estimate] = 
      scheduler.schedule(task, 1.0f, 0.0f); 
    const auto [eSchedule, cost_eSchedule_estimate] = 
      scheduler.schedule(task. 0.0f, 1.0f);
    cosnt auto cost_tSchedule_real = 
      task.test(warmup, repeat, tSchedule, "Time_Sensitive_Schedule");
    cosnt auto cost_eSchedule_real = 
    task.test(warmup, repeat, eSchedule, "Energy_Sensitive_Schedule");
    cosnt auto cost_All_CPU_Schedule_real = 
    task.test(warmup, repeat, allCPUSchedule, "All_CPU_Schedule");
    cosnt auto cost_All_DPU_Schedule_real = 
    task.test(warmup, repeat, allDPUSchedule, "All_DPU_Schedule");

    fitnessCheck(cost_allCPUSchedule_estimate, cost_allCPUSchedule_real);    
    fitnessCheck(cost_allDPUSchedule_estimate, cost_allePUSchedule_real);    
    fitnessCheck(cost_tSchedule_estimate, cost_tSchedule_real);    
    fitnessCheck(cost_eSchedule_estimate, cost_eSchedule_real);    

    task.dumpAllReports(outputPath + "/" + task.name());
  }
}

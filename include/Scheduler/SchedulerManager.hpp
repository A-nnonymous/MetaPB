#ifndef SCHED_MNGR
#define SCHED_MNGR
#include "Scheduler/SchedulerBase.hpp"
namespace MetaPB{
namespace Scheduler{

typedef std::vector<double> Schedule;

enum class SchedulerTag{
  CPUOnly,
  Greedy,
  HEFT,
  MetaPB_PerfFirst,
  MetaPB_Hybrid,
  MetaPB_EffiFirst
};

/// @brief Factory class that produce Scheduler.
struct SchedulerManager{
  SchedulerBase getScheduler(SchedulerTag tag){
    switch(SchedulerTag){
      case SchedulerTag::CPUOnly           : return ;
      case SchedulerTag::Greedy            : return ;
      case SchedulerTag::HEFT              : return ;
      case SchedulerTag::MetaPB_PerfFirst  : return ;
      case SchedulerTag::MetaPB_Hybrid     : return ;
      case SchedulerTag::MetaPB_EffiFirst  : return ;
      default: break;
    }
  }
}; // struct SchedulerManager

} // namespace MetaPB
} // namespace Scheduler

#endif
#ifndef SCHED_MNGR
#define SCHED_MNGR
// TODO: finish these
#include "Scheduler/SchedulerBase.hpp"
#include "Scheduler/CPUOnlyScheduler.hpp"
#include "Scheduler/GreedyScheduler.hpp"
// TODO: adapt these
#include "Scheduler/HEFTScheduler.hpp"
#include "Scheduler/MetaScheduler.hpp"

namespace MetaPB{
namespace Scheduler{

typedef std::vector<double> Schedule;

enum class SchedulerTag{
  CPUOnly, // Only Schedule tasks to CPU
  Greedy,  // Schedule in a short-sight way, depending only on task suitability
  HEFT,   // Schedule by Heterogenous Earliest Finished Time algorithm
  MetaPB_PerfFirst, // Proposed algorithm, in a Performance-first weight
  MetaPB_Hybrid,    // Proposed algorithm, in a pseudo-balanced weight
  MetaPB_EffiFirst  // Proposed algorithm, in a Efficiency-first weight
};

/* TODO: finish all scheduler
/// @brief Factory class that produce Scheduler.
struct SchedulerManager{
  SchedulerBase getScheduler(SchedulerTag tag){
    switch(SchedulerTag){
      case SchedulerTag::CPUOnly           : return CPUOnlyScheduler();
      case SchedulerTag::Greedy            : return GreedyScheduler();
      case SchedulerTag::HEFT              : return HEFTScheduler();
      case SchedulerTag::MetaPB_PerfFirst  : return MetaScheduler(1.0f, 0.0f) ;
      case SchedulerTag::MetaPB_Hybrid     : return MetaScheduler(0.5f, 0.5f) ;
      case SchedulerTag::MetaPB_EffiFirst  : return MetaScheduler(0.0f, 1.0f) ;
    }
  }
}; // struct SchedulerManager
*/

} // namespace MetaPB
} // namespace Scheduler

#endif
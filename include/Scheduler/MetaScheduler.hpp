#ifndef METASCHED_HPP
#define METASCHED_HPP
#include "Scheduler/SchedulerBase.hpp"

namespace MetaPB {
namespace Scheduler {
using OperatorManager = Operator::OperatorManager;
class MetaScheduler {
public:
public:
  virtual Schedule schedule(const TaskGraph &g, OperatorManager &om) noexcept;

private:
  const double Arg_Alpha;
  const double Arg_Beta;
  const size_t OptIterMax;
  const size_t batchSize_MiB;
  const bool isEarlyEndEnable = false;

}; // class MetaScheduler

} // namespace Scheduler
} // namespace MetaPB
#endif

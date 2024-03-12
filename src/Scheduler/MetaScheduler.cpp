#include "Scheduler/MetaScheduler.hpp"

// TODO: 1. Finish the modelLoader logic
//      2. Instantiate the shcedOptimize logic

/* -------------------- Main Logic of MetaPB --------------------------
Pseudo Code:
  load_operator_model(this_machine, perf_model_path)
  for all operator:
    if(!modelized(operator))
      warmup_and_modelize(operator)
    endif
  endfor

  proposed_schedule = random()
  do:
    perf_expectations = regression_perf_model(Task, proposed_schedule)
    prev_propose_value = eval_func(Alpha, Beta, perf_expectations)
    prev_schedule = proposed_schedule
    proposed_schedule = metaOptimizer(prev_schedule, prev_val)
  while(!converge_or_iterEnd(prev_proposed_val)):
  endwhile
  schedule_vector = best(proposed_schedules)
  perf_expectations = regression_perf_model(schedule_vector)

  if(run_sync):
    Result = Task.exec(SYNCHRONOUS, Args, schedule_vector)
  else
    pool = static_thread_pool(max_thread - 1) + DPU_proxy(1)
    scheduler = pool.get_scheduler()
    sender = Task.exec(ASYNCHRONOUS, Args, schedule_vector)
    Result = sync_wait(scheduler.schedule(sender))
  endif
*/
namespace MetaPB {
namespace MetaScheduler {

MetaScheduler::MetaScheduler(const double Arg_Alpha, const double Arg_Beta)
    : Arg_Alpha(Arg_Alpha), Arg_Beta(Arg_Beta) {}

/// @brief Main schedule generator function
/// @param perfModelPathIn User given cached performance model path, default in
/// /tmp/MetaPB_PerfModels/
/// @return An optimized schedule vector.
Schedule MetaScheduler::scheduleGen(string perfModelPathIn) noexcept {
#ifdef DRY_RUN
#else
  // modelizeIfNotLoad(perfModelPath);
  task.modelize(modelCachePath);
  // Invariant: To this point, the whole set of operators in given task is fully
  // modelized.
  auto schedVec =
      task.randSchedule(batchSize); // Random init a schedule to be optimized.
  schedOptimize(schedVec);
#endif // DRY_RUN
  return schedVec;
}

/// @brief Try to load correct model of all operator presented in work object in
/// given path, if not existed,
//  warmup and train them.
void MetaScheduler::modelizeIfNotLoaded() noexcept {
#ifdef DRY_RUN
#else
  // for each operator in task, if given path don't exist such model,
  for (const auto &opTag : task.getOpTagSet()) {
    if (...) { // finder & verification logic
      Operator::modelize(opTag);
    }
  }
#endif // DRY_RUN
}

/// @brief Evaluate performance metric deduced by regression model using
/// weighted combination.
/// @return Velues of given schedule vector.
vector<double>
MetaScheduler::perfEval(const ScheduleVec &schedVec) const noexcept {
#ifdef DRY_RUN
#else
  // The true implementaion of final evaluation function
  vector<double> result();
  result.reserve(batchSize);
  for (const auto &schedule : schedVec) {
    double timeCost_Second = task.deduceTime_ns(schedVec) / 1e9;
    double energyCost_Joule = task.deduceEnergy_joule(schedVec);
    result.emplace_back(Arg_Alpha * timeCost_second +
                        Arg_Beta * energyCost_Joule);
  }
  return result;
#endif // DRY_RUN
}

/// @brief Using metaheuristic optimizer to perform schedule vector exploration.
void MetaScheduler::schedOptimize(const ScheduleVec &schedVec) noexcept {
#ifdef DRY_RUN
#else
  // Plug in the metaheuristic optimizer.
  auto optimizer = Optimizer::generate(optType);
  optimizer.optimize(schedVec, perfEval); // perform optimization.

#ifdef DUMP_OPT_HISTORY
  optimizer.dumpAllReport();
#endif // DUMP_OPT_HISTORY
#endif // DRY_RUN
}

} // namespace MetaScheduler
} // namespace MetaPB

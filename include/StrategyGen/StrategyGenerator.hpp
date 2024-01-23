#ifndef STRATGEN_HPP
#define STRATGEN_HPP
#include "Task/"
namespace MetaPB {
namespace StratGen {

class StrategyGenerator {
public:
  StrategyGenerator() = default;

  // Set the work distribute ratio iterately.
  void genStrategy(Task &inputTask) const noexcept;
}; // class MetaScheduler
} // namespace StratGen
} // namespace MetaPB
#endif

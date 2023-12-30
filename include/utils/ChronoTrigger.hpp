#ifndef CHRNO_TGR_HPP
#define CHRNO_TGR_HPP

#include "MetricsGather.hpp"
#include <chrono>
#include <cmath>
#include <pcm/cpucounters.h>
#include <vector>

namespace MetaPB {
namespace utils {

using pcm::PCM;
using pcm::SocketCounterState;
using pcm::SystemCounterState;
using std::vector;

/// @brief Named after my favorite JRPG game,
// is a reenterable time & performance statistic counter.
class ChronoTrigger {
  typedef std::chrono::high_resolution_clock clock;
  typedef std::chrono::time_point<clock> timePoint;

  /// @brief This struct is what tick-tock directly deals with.
  typedef struct Probe {
    timePoint time;
    SystemCounterState systemState;
    vector<SocketCounterState> socketStates;
  } Probe;

public:
  /// @brief This class should not be initialized without taskNum
  ChronoTrigger() = delete;
  ChronoTrigger(size_t taskNum);

  // In consideration of precision, the taskIdx must be a imm-num
  /// @brief Start counting time for a specific work
  void tick(const size_t taskIdx_In);
  /// @brief stop counting time for a specific work
  void tock(const size_t taskIdx_In);

  /// @brief Uses the overrided operator to correct the idle bias
  /// @param taskIdx_In input taskIdx (0 - taskNum - 1)
  /// @return Unbiased report.
  inline Report getReport(size_t taskIdx_In) const noexcept {
    return reports[taskIdx_In + 1] - reports[0];
  }

private:
  // -------------- Intel PCM related members -------------
  PCM *pcmHandle;
  size_t socketNum;

  const std::size_t taskNum;
  std::vector<Probe> lastProbes;
  std::vector<Report> reports;
};

} // namespace utils
} // namespace MetaPB
#endif

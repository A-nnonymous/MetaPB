#ifndef CHRNO_TGR_HPP
#define CHRNO_TGR_HPP

#include <chrono>
#include <cmath>
#include <pcm/cpucounters.h>
#include <vector>
#include "Quant.hpp"

namespace MetaPB {
namespace utils {

using pcm::PCM;
using std::vector;
using pcm::SocketCounterState;
using pcm::SystemCounterState;

/// @brief Named after my favorite JRPG game,
// is a reenterable time & performance statistic counter.
class ChronoTrigger {
  typedef std::chrono::high_resolution_clock clock;
  typedef std::chrono::time_point<clock> timePoint;

  /// @brief This struct is what tick-tock directly deals with.
  typedef struct Probe{
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
  void tick(const size_t taskIdx);
  /// @brief stop counting time for a specific work
  void tock(const size_t taskIdx);

  inline Report getReport(size_t taskIdx)const noexcept{
    return reports[taskIdx];
  }

private:
  // -------------- Intel PCM related members -------------
  inline static PCM *pcmHandle = PCM::getInstance();
  inline static const size_t socketNum = pcmHandle->getNumSockets();

  const std::size_t taskNum;
  std::vector<Probe> lastProbes;
  std::vector<Report> reports;
};

} // namespace utils
} // namespace MetaPB
#endif

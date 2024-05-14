#ifndef CHRNO_TGR_HPP
#define CHRNO_TGR_HPP

#include "CSVWriter.hpp"
#include "MetricsGather.hpp"
#include "Stats.hpp"
#include <chrono>
#include <cmath>
#include <map>
#include <pcm/cpucounters.h>
#include <vector>

namespace MetaPB {
namespace utils {

using pcm::PCM;
using pcm::SocketCounterState;
using pcm::SystemCounterState;
using std::map;
using std::vector;

/// @brief Named after my favorite JRPG game(OST specificlly),
// is a reentrant time & performance statistic counter & report generator
class ChronoTrigger {
  typedef std::chrono::high_resolution_clock clock;
  typedef std::chrono::time_point<clock> timePoint;

  /// @brief This struct is what tick-tock directly deals with.
  typedef struct Probe {
    timePoint time;
    SystemCounterState systemState;
    vector<SocketCounterState> socketStates;
    Probe() = default;
  } Probe;

public:
  ChronoTrigger();

  /// @brief Unique name of specific task(exclude __BIAS__)
  void tick(const std::string &taskName);
  /// @brief stop counting time for a specific work
  void tock(const std::string &taskName);
  /// @brief Clean all the register.
  void clear() noexcept {
    task2Report.clear();
    task2LastProbe.clear();
  }

  /// @brief Uses the overrided operator to correct the idle bias
  /// @param taskIdx_In input taskIdx (0 - taskNum - 1)
  /// @return Unbiased report.
  inline Report getReport(const std::string &taskName) {
    return task2Report[taskName] - task2Report["__BIAS__"];
  }

  /// @brief Dump all report into a given path, ordered correspond to
  /// metricTag that used in this profiling task;
  /// @param path A path that store all goods, expected to witness
  /// metricTag::size number of csv file.
  void dumpAllReport(std::string path);

private:
  // -------------- Intel PCM related members -------------
  PCM *pcmHandle;
  const size_t socketNum;
  std::map<std::string, Report> task2Report;
  std::map<std::string, Probe> task2LastProbe;

  inline static PCM *getPCMInstance() {
    static PCM *instance = PCM::getInstance();
    return instance;
  }
};

} // namespace utils
} // namespace MetaPB
#endif

#include "utils/ChronoTrigger.hpp"
#define BIAS_CORRECTION_REPEAT 20

namespace MetaPB {
namespace utils {

using std::vector;

ChronoTrigger::ChronoTrigger(size_t taskNum) : taskNum(taskNum) {
  lastProbes.resize(taskNum);
  pcmHandle = PCM::getInstance();
  socketNum = pcmHandle->getNumSockets();
  for (auto &probe : lastProbes) {
    probe.systemState = pcmHandle->getSystemCounterState();
    for (int i = 0; i < socketNum; i++) {
      probe.socketStates.emplace_back(pcmHandle->getSocketCounterState(i));
    }
  }
  reports.resize(
      taskNum + 1,
      Report(socketNum, metricTag::size)); // task0 is for bias correction.

#pragma unroll BIAS_CORRECTION_REPEAT
  for (auto corr = 0; corr < BIAS_CORRECTION_REPEAT; corr++) {
    tick(-1);
    tock(-1);
  }
}

void ChronoTrigger::tick(const size_t taskIdx_In) {
  const size_t taskIdx = taskIdx_In + 1;
  pcmHandle = PCM::getInstance();
  for (auto i = 0; i < socketNum; ++i) {
    if (lastProbes[taskIdx].socketStates.empty()) {
      lastProbes[taskIdx].socketStates.emplace_back(
          pcmHandle->getSocketCounterState(i));
    } else [[likely]] {
      lastProbes[taskIdx].socketStates[i] = pcmHandle->getSocketCounterState(i);
    }
  }
  lastProbes[taskIdx].systemState = pcmHandle->getSystemCounterState();
  // Ensure the time tick is always beneath all others.
  lastProbes[taskIdx].time = clock::now();
}

void ChronoTrigger::tock(const size_t taskIdx_In) {
  // State gathering, time probe first.
  const auto currTimePoint = clock::now();
  const auto currSysState = pcmHandle->getSystemCounterState();
  vector<SocketCounterState> currSocketState(socketNum);
  for (auto i = 0; i < socketNum; i++) {
    currSocketState[i] = pcmHandle->getSocketCounterState(i);
  }

  const size_t taskIdx = taskIdx_In + 1;
  // Stat calculating and register.
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
      currTimePoint - lastProbes[taskIdx].time);
  const double thisDuration_ns = duration.count();
  reports[taskIdx].reportItems[TimeConsume_ns].addData(thisDuration_ns);

  for (auto i = 0; i < metricTag::size; ++i) {
    auto metricIdx = static_cast<metricTag>(i);
    metric2Func[metricIdx](reports[taskIdx].reportItems[i],
                           lastProbes[taskIdx].socketStates, currSocketState,
                           lastProbes[taskIdx].systemState, currSysState);
  }
}
} // namespace utils
} // namespace MetaPB

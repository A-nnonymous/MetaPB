#include "ChonoTrigger.hpp"


namespace MetaPB {
namespace utils {

using std::vector;
using pcm::SocketCounterState;
using pcm::SystemCounterState;
using pcm::getBackendBound;
using pcm::getBadSpeculation;
using pcm::getBytesReadFromMC;
using pcm::getBytesWrittenToMC;
using pcm::getConsumedJoules;
using pcm::getCoreBound;
using pcm::getDRAMConsumedJoules;
using pcm::getFrontendBound;
using pcm::getL3CacheHitRatio;
using pcm::getL3CacheOccupancy;
using pcm::getMemoryBound;
using pcm::getQPItoMCTrafficRatio;

ChronoTrigger::ChronoTrigger(size_t taskNum) : taskNum(taskNum) {
  lastProbes.resize(taskNum);
  for(auto &probe: lastProbes){
    probe.systemState = pcmHandle->getSystemCounterState();
    for(int i = 0; i < socketNum; i++){
      probe.socketStates.emplace_back(pcmHandle->getSocketCounterState(i));
    }
  }
  reports.resize(taskNum, Report(socketNum));
}

void ChronoTrigger::tick(const size_t taskIdx) {
  for (auto i = 0; i < socketNum; ++i) {
    if(lastProbes[taskIdx].socketStates.empty()){
      lastProbes[taskIdx].socketStates.emplace_back(pcmHandle->getSocketCounterState(i));
    }else[[likely]]{
      lastProbes[taskIdx].socketStates[i] = pcmHandle->getSocketCounterState(i);
    }
  }
  lastProbes[taskIdx].systemState = pcmHandle->getSystemCounterState();
  // Ensure the tick is always behind all miscs
  lastProbes[taskIdx].time = clock::now();
}

void ChronoTrigger::tock(const size_t taskIdx) {
  // State gathering, time probe first.
  const auto currTimePoint = clock::now();
  const auto currSysState = pcmHandle->getSystemCounterState();
  vector<SocketCounterState> currSocketState(socketNum);
  for (auto i = 0; i < socketNum; i++) {
    currSocketState[i] = pcmHandle->getSocketCounterState(i);
  }

  // Stat calculating and register.
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
      currTimePoint - lastProbes[taskIdx].time);
  const double thisDuration_ns = duration.count();

  for (auto i = 0; i < socketNum; ++i) {
    reports[taskIdx].FrontendBoundRatio[i].add(getFrontendBound(
        lastProbes[taskIdx].socketStates[i], currSocketState[i]));
    reports[taskIdx].CoreBoundRatio[i].add(getCoreBound(
        lastProbes[taskIdx].socketStates[i], currSocketState[i]));
    reports[taskIdx].MemoryBoundRatio[i].add(getMemoryBound(
        lastProbes[taskIdx].socketStates[i], currSocketState[i]));
    reports[taskIdx].BackendBoundRatio[i].add(getBackendBound(
        currSocketState[i], lastProbes[taskIdx].socketStates[i]));
    reports[taskIdx].SpeculationBoundRatio[i].add(getBadSpeculation(
        lastProbes[taskIdx].socketStates[i], currSocketState[i]));
    reports[taskIdx].LLCHitRate[i].add(getL3CacheHitRatio(
        lastProbes[taskIdx].socketStates[i], currSocketState[i]));
    reports[taskIdx].TotalDRAMCTRLRead_Bytes[i].add(getBytesReadFromMC(
        lastProbes[taskIdx].socketStates[i], currSocketState[i]));
    reports[taskIdx].TotalDRAMCTRLWrite_Bytes[i].add(getBytesWrittenToMC(
        lastProbes[taskIdx].socketStates[i], currSocketState[i]));
    reports[taskIdx].CPUPowerConsumption_Joule[i].add(getConsumedJoules(
        lastProbes[taskIdx].socketStates[i], currSocketState[i]));
    reports[taskIdx].DRAMPowerConsumption_Joule[i].add(getDRAMConsumedJoules(
        lastProbes[taskIdx].socketStates[i], currSocketState[i]));
    reports[taskIdx].LLCOccupancyTrail[i].add(
        getL3CacheOccupancy(currSocketState[i]) -
        getL3CacheOccupancy(lastProbes[taskIdx].socketStates[i]));
  }
  reports[taskIdx].QPITrafficRatio.add(
      getQPItoMCTrafficRatio(currSysState, lastProbes[taskIdx].systemState));
  reports[taskIdx].TimeConsume_ns.add(thisDuration_ns);
}

} // namespace utils
} // namespace MetaPB
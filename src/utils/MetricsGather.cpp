#include "utils/MetricsGather.hpp"

namespace MetaPB {
namespace utils {

static inline void TGet(reportItem &item, const vector<sockState> &soBF,
                        const vector<sockState> &soAFT, const sysState &sysBF,
                        const sysState &sysAFT) {
  // time gathering must be done outside to maintain precision.
}

static inline void FBGet(reportItem &item, const vector<sockState> &soBF,
                         const vector<sockState> &soAFT, const sysState &sysBF,
                         const sysState &sysAFT) {
  vector<double> dataVec(soAFT.size(), 0);
  for (auto i = 0; i < soAFT.size(); i++) {
    dataVec[i] = pcm::getFrontendBound(soBF[i], soAFT[i]);
  }
  item.addData(dataVec);
}
static inline void BBGet(reportItem &item, const vector<sockState> &soBF,
                         const vector<sockState> &soAFT, const sysState &sysBF,
                         const sysState &sysAFT) {
  vector<double> dataVec(soAFT.size(), 0);
  for (auto i = 0; i < soAFT.size(); i++) {
    dataVec[i] = pcm::getBackendBound(soBF[i], soAFT[i]);
  }
  item.addData(dataVec);
}
static inline void CBGet(reportItem &item, const vector<sockState> &soBF,
                         const vector<sockState> &soAFT, const sysState &sysBF,
                         const sysState &sysAFT) {
  vector<double> dataVec(soAFT.size(), 0);
  for (auto i = 0; i < soAFT.size(); i++) {
    dataVec[i] = pcm::getCoreBound(soBF[i], soAFT[i]);
  }
  item.addData(dataVec);
}
static inline void MBGet(reportItem &item, const vector<sockState> &soBF,
                         const vector<sockState> &soAFT, const sysState &sysBF,
                         const sysState &sysAFT) {
  vector<double> dataVec(soAFT.size(), 0);
  for (auto i = 0; i < soAFT.size(); i++) {
    dataVec[i] = pcm::getMemoryBound(soBF[i], soAFT[i]);
  }
  item.addData(dataVec);
}
static inline void SBGet(reportItem &item, const vector<sockState> &soBF,
                         const vector<sockState> &soAFT, const sysState &sysBF,
                         const sysState &sysAFT) {
  vector<double> dataVec(soAFT.size(), 0);
  for (auto i = 0; i < soAFT.size(); i++) {
    dataVec[i] = pcm::getBadSpeculation(soBF[i], soAFT[i]);
  }
  item.addData(dataVec);
}
static inline void LHGet(reportItem &item, const vector<sockState> &soBF,
                         const vector<sockState> &soAFT, const sysState &sysBF,
                         const sysState &sysAFT) {
  vector<double> dataVec(soAFT.size(), 0);
  for (auto i = 0; i < soAFT.size(); i++) {
    dataVec[i] = pcm::getL3CacheHitRatio(soBF[i], soAFT[i]);
  }
  item.addData(dataVec);
}
static inline void LOGet(reportItem &item, const vector<sockState> &soBF,
                         const vector<sockState> &soAFT, const sysState &sysBF,
                         const sysState &sysAFT) {
  vector<double> dataVec(soAFT.size(), 0);
  for (auto i = 0; i < soAFT.size(); i++) {
    dataVec[i] =
        pcm::getL3CacheOccupancy(soAFT[i]) - pcm::getL3CacheOccupancy(soBF[i]);
  }
  item.addData(dataVec);
}
static inline void DRGet(reportItem &item, const vector<sockState> &soBF,
                         const vector<sockState> &soAFT, const sysState &sysBF,
                         const sysState &sysAFT) {
  vector<double> dataVec(soAFT.size(), 0);
  for (auto i = 0; i < soAFT.size(); i++) {
    dataVec[i] = pcm::getBytesReadFromMC(soBF[i], soAFT[i]);
  }
  item.addData(dataVec);
}
static inline void DWGet(reportItem &item, const vector<sockState> &soBF,
                         const vector<sockState> &soAFT, const sysState &sysBF,
                         const sysState &sysAFT) {
  vector<double> dataVec(soAFT.size(), 0);
  for (auto i = 0; i < soAFT.size(); i++) {
    dataVec[i] = pcm::getBytesWrittenToMC(soBF[i], soAFT[i]);
  }
  item.addData(dataVec);
}
static inline void CPGet(reportItem &item, const vector<sockState> &soBF,
                         const vector<sockState> &soAFT, const sysState &sysBF,
                         const sysState &sysAFT) {
  vector<double> dataVec(soAFT.size(), 0);
  for (auto i = 0; i < soAFT.size(); i++) {
    dataVec[i] = pcm::getConsumedJoules(soBF[i], soAFT[i]);
  }
  item.addData(dataVec);
}
static inline void DPGet(reportItem &item, const vector<sockState> &soBF,
                         const vector<sockState> &soAFT, const sysState &sysBF,
                         const sysState &sysAFT) {
  vector<double> dataVec(soAFT.size(), 0);
  for (auto i = 0; i < soAFT.size(); i++) {
    dataVec[i] = pcm::getDRAMConsumedJoules(soBF[i], soAFT[i]);
  }
  item.addData(dataVec);
}
static inline void QTGet(reportItem &item, const vector<sockState> &soBF,
                         const vector<sockState> &soAFT, const sysState &sysBF,
                         const sysState &sysAFT) {
  double data = 0.0f;
  data = pcm::getQPItoMCTrafficRatio(sysBF, sysAFT);
  item.addData(data);
}

/// @brief Register table of all quant-gathering funcitons.
std::map<metricTag, stateGatheringFunc> metric2Func = {
    {TimeConsume_ns, TGet},
    {FrontendBoundRatio, FBGet},
    {BackendBoundRatio, BBGet},
    {CoreBoundRatio, CBGet},
    {MemoryBoundRatio, MBGet},
    {SpeculationBoundRatio, SBGet},
    {LLCHitRate, LHGet},
    {LLCOccupancyTrail, LOGet},
    {TotalDRAMCTRLRead_Bytes, DRGet},
    {TotalDRAMCTRLWrite_Bytes, DWGet},
    {CPUPowerConsumption_Joule, CPGet},
    {DRAMPowerConsumption_Joule, DPGet},
    {QPITrafficRatio, QTGet}};

} // namespace utils
} // namespace MetaPB

#ifndef QUANT_GETTER_HPP
#define QUANT_GETTER_HPP
#include "ReportUtils.hpp"
#include <functional>
#include <pcm/cpucounters.h>
#include <vector>

namespace MetaPB {
namespace utils {
using sysState = pcm::SystemCounterState;
using sockState = pcm::SocketCounterState;

enum metricTag {
  TimeConsume_ns,
  FrontendBoundRatio,         // pcm #4696
  BackendBoundRatio,          // pcm #4661
  CoreBoundRatio,             // pcm #4686
  MemoryBoundRatio,           // pcm #4675
  SpeculationBoundRatio,      // pcm #4735
  LLCHitRate,                 // pcm #3822
  LLCOccupancyTrail,          // pcm #3920
  TotalDRAMCTRLRead_Bytes,    // pcm #4142
  TotalDRAMCTRLWrite_Bytes,   // pcm #4156
  CPUPowerConsumption_Joule,  // pcm #2990
  DRAMPowerConsumption_Joule, // pcm #3020
  QPITrafficRatio,            // pcm #4570
  size                        // tail indicator of size, dont put anything below
};

static std::map<metricTag, std::string> metricNameMap = {
    {TimeConsume_ns, "TimeConsume_ns"},
    {FrontendBoundRatio, "FrontendBoundRatio"},
    {BackendBoundRatio, "BackendBoundRatio"},
    {CoreBoundRatio, "CoreBoundRatio"},
    {MemoryBoundRatio, "MemoryBoundRatio"},
    {SpeculationBoundRatio, "SpeculationBoundRatio"},
    {LLCHitRate, "LLCHitRate"},
    {LLCOccupancyTrail, "LLCOccupancyTrail"},
    {TotalDRAMCTRLRead_Bytes, "TotalDRAMCTRLRead_Bytes"},
    {TotalDRAMCTRLWrite_Bytes, "TotalDRAMCTRLWrite_Bytes"},
    {CPUPowerConsumption_Joule, "CPUPowerConsumption_Joule"},
    {DRAMPowerConsumption_Joule, "DRAMPowerConsumption_Joule"},
    {QPITrafficRatio, "QPITrafficRatio"}};

typedef std::function<void(reportItem &, const vector<sockState> &,
                           const vector<sockState> &, const sysState &,
                           const sysState &)>
    stateGatheringFunc;

extern std::map<metricTag, stateGatheringFunc> metric2Func;

} // namespace utils
} // namespace MetaPB
#endif

#ifndef QUANT_HPP
#define QUANT_HPP

#include <vector>
#include <cstdint>
#include <cmath>

namespace MetaPB {
namespace utils {
using std::vector;

typedef struct Stats {
  size_t rep = 0;
  double mean = 0.0f;
  double variance = 0.0f;
  double stdVar = 0.0f;
  double upperBound = -__DBL_MAX__;
  double lowerBound = __DBL_MAX__;
  double upperBias = 0.0f;
  double lowerBias = 0.0f;
  double sum = 0.0f;

  Stats() = default;
  void add(const double &newVal) {
    // Counter winding.
    rep++;
    const double prevMean = this->mean;
    const double prevVar = this->variance;
    // Incremental mean value maintaining
    this->mean += (1.0 / (double)rep) * (newVal - prevMean);
    // Unbiased incremental variance algorithm
    // Derived from variance calculation formula.
    if (rep != 1) [[likely]] {
      this->variance = ((rep - 2) * prevVar / (rep - 1)) +
                        (newVal - prevMean) * (newVal - prevMean) / rep;
      this->stdVar = sqrt(this->variance);
    }
    // Bound maintaining
    if (newVal > this->upperBound) {
      this->upperBound = newVal;
    }
    if (newVal < this->lowerBound) {
      this->lowerBound = newVal;
    }
    // Bias maintaining
    this->lowerBias = this->mean - lowerBound;
    this->upperBias = upperBound - this->mean;
    // Sum maintaining
    sum += newVal;
  }
} Stats;

/// @brief Contains all metrics that useful for MetaPB
// analyzer.
typedef struct Report {
  // ----------------- Absolute stats ------------------
  Stats TimeConsume_ns; // using std::chrono HRclock
  // ----------------- Socket stats --------------------
  vector<Stats> FrontendBoundRatio;         // pcm #4696
  vector<Stats> BackendBoundRatio;          // pcm #4661
  vector<Stats> CoreBoundRatio;             // pcm #4686
  vector<Stats> MemoryBoundRatio;           // pcm #4675
  vector<Stats> SpeculationBoundRatio;      // pcm #4735
  vector<Stats> LLCHitRate;                 // pcm #3822
  vector<Stats> LLCOccupancyTrail;          // pcm #3920 (transient)
  vector<Stats> TotalDRAMCTRLRead_Bytes;    // pcm #4142
  vector<Stats> TotalDRAMCTRLWrite_Bytes;   // pcm #4156
  vector<Stats> CPUPowerConsumption_Joule;  // pcm #2990
  vector<Stats> DRAMPowerConsumption_Joule; // pcm #3020
  // ----------------- System stats ---------------------
  Stats QPITrafficRatio;                    // pcm #4570

  Report(size_t socketN) {
    FrontendBoundRatio.resize(socketN);
    BackendBoundRatio.resize(socketN);
    CoreBoundRatio.resize(socketN);
    MemoryBoundRatio.resize(socketN);
    SpeculationBoundRatio.resize(socketN);
    LLCHitRate.resize(socketN);
    LLCOccupancyTrail.resize(socketN);
    TotalDRAMCTRLRead_Bytes.resize(socketN);
    TotalDRAMCTRLWrite_Bytes.resize(socketN);
    CPUPowerConsumption_Joule.resize(socketN);
    DRAMPowerConsumption_Joule.resize(socketN);
  }
} Report;
} // namespace utils
} // namespace MetaPB
#endif
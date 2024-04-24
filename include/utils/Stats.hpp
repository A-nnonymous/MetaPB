#ifndef QUANT_HPP
#define QUANT_HPP

#include "Operator/OperatorRegistry.hpp"
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace MetaPB {
namespace utils {

using std::string;
using std::vector;
using OperatorTag = Operator::OperatorTag;

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

typedef struct perfStats{
  double energyCost_Joule = 0.0f;
  double timeCost_Second = 0.0f;
  double dataMovement_MiB = 0;

  perfStats operator+(const perfStats& rhs) const {
      return {energyCost_Joule + rhs.energyCost_Joule, timeCost_Second + rhs.timeCost_Second, dataMovement_MiB + rhs.dataMovement_MiB};
  }

  perfStats operator-(const perfStats& rhs) const {
      return {energyCost_Joule - rhs.energyCost_Joule, timeCost_Second - rhs.timeCost_Second, dataMovement_MiB - rhs.dataMovement_MiB};
  }

  perfStats& operator*=(long unsigned int rhs) {
      energyCost_Joule *= rhs;
      timeCost_Second *= rhs;
      dataMovement_MiB *= rhs;
      return *this;
  }

  perfStats& operator/=(long unsigned int rhs) {
      energyCost_Joule /= rhs;
      timeCost_Second /= rhs;
      dataMovement_MiB /= rhs;
      return *this;
  }
  perfStats operator*(long unsigned int rhs) const {
    perfStats newStats = *this;
    newStats*=rhs;
    return newStats;
  }
  perfStats operator/(long unsigned int rhs) const {
    perfStats newStats = *this;
    newStats/=rhs;
    return newStats;
  }
} perfStats;

typedef std::map<OperatorTag, size_t> regressionTask;

} // namespace utils
} // namespace MetaPB
#endif

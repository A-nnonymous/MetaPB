#ifndef REPORT_UTILS_HPP
#define REPORT_UTILS_HPP
#include "Stats.hpp"
#include <string>
#include <variant>
#include <vector>

namespace MetaPB {
namespace utils {

using std::string;
using std::vector;
typedef struct reportItem {
  string name;
  std::variant<Stats, vector<Stats>> data;

  void addData(const double &newData) { std::get<Stats>(data).add(newData); }

  void addData(const vector<double> &newData) {
    if (data.index() == 0) {
      data = vector<Stats>(newData.size());
    }
    auto &vec = std::get<vector<Stats>>(data);
    for (size_t i = 0; i < vec.size(); ++i) {
      vec[i].add(newData[i]);
    }
  }
  reportItem &operator-=(const reportItem &rhs) {
    if (this->data.index() == 0) {
      auto &myMean = std::get<Stats>(data).mean;
      const auto &rhsMean = std::get<Stats>(rhs.data).mean;
      std::get<Stats>(data).mean -= std::get<Stats>(rhs.data).mean;
    } else {
      auto &myVec = std::get<vector<Stats>>(data);
      const auto &rhsVec = std::get<vector<Stats>>(rhs.data);
      for (auto i = 0; i < myVec.size(); i++) {
        myVec[i].mean -= rhsVec[i].mean;
      }
    }
    return *this;
  }
  friend reportItem operator-(const reportItem &lhs, const reportItem &rhs){
    reportItem result(lhs);
    result -= rhs;
    return result;
  }
} reportItem;

/// @brief Contains all metrics that useful for MetaPB
// analyzer.
typedef struct Report {
  size_t socketNum;
  size_t itemNum;
  vector<reportItem> reportItems;

  Report() = default;
  Report(size_t socketN, size_t itemNum) : itemNum(itemNum) {
    reportItems.resize(itemNum);
  }
  /// @brief This operator is used only on bias correction
  /// @param rhs idle counter bias report
  /// @return Unbiased report
  Report &operator-=(const Report &rhs) {
    for (auto i = 0; i < rhs.itemNum; i++) {
      reportItems[i] -= rhs.reportItems[i];
    }
    return *this;
  }
  friend Report operator-(const Report &lhs, const Report &rhs) {
    Report result(lhs);
    result -= rhs;
    return result;
  }
} Report;

} // namespace utils
} // namespace MetaPB
#endif
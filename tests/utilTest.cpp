#include "utils/ChronoTrigger.hpp"
#include "utils/MetricsGather.hpp"
#include "utils/Stats.hpp"
using namespace MetaPB::utils;
using MetaPB::utils::metricTag;
using MetaPB::utils::Stats;

#include <chrono>
#include <iostream>
#include <vector>

int main() {
  int rep = 10;
  size_t itemNum = ((size_t)1 << 30) / sizeof(int); // 1GiB
  std::vector<int> a(itemNum, 0);
  ChronoTrigger ct;
  for (int r = 0; r < rep; r++) {
    ct.tick("naive");
    for (size_t i = 0; i < itemNum; i++) {
      a[i]++;
    }
    ct.tock("naive");
  }
  auto report = ct.getReport("naive");
  auto timeMean =
      std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data).mean;
  auto energyMeans = std::get<vector<Stats>>(
      report.reportItems[metricTag::CPUPowerConsumption_Joule].data);
  auto energyMeanSum = energyMeans[0].mean + energyMeans[1].mean;
  std::cout << "time mean for 1GiB VA: " << timeMean / 1e9 << std::endl;
  std::cout << "energy mean for 1GiB VA: " << energyMeanSum << std::endl;
  ct.dumpAllReport("./");
}

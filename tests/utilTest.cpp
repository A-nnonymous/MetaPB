#ifdef CT
#include "utils/ChronoTrigger.hpp"
using namespace MetaPB::utils;
#endif

#include <chrono>
#include <iostream>
#include <vector>

int main() {
  int rep = 10;
  size_t itemNum = ((size_t)2 << 30) / sizeof(int); // 1GiB
  std::vector<int> a(itemNum, 0);
#ifdef CT
  /*
  ChronoTrigger ct(2);
  */
  ChronoTrigger ct;
#endif
  for (int r = 0; r < rep; r++) {
    auto start = std::chrono::high_resolution_clock::now();
#ifdef CT
    /*
    ct.tick(0);
    */
    ct.tick("naive" );
#endif
    for (size_t i = 0; i < itemNum; i++) {
      a[i]++;
    }
#ifdef CT
    ct.tock("naive" );
    /*
    ct.tock(0);
    */
#endif
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout << "Costs : " << duration.count() / 1e6 << "ms" << std::endl;
  }
#ifdef CT
  //Report rp = ct.getReport(0);
  Report rp = ct.getReport("naive");
  std::cout << (std::get<Stats>(rp.reportItems[0].data).mean) / 1e6
            << std::endl;
  std::cout << (std::get<Stats>(rp.reportItems[0].data).lowerBound) / 1e6
            << std::endl;
            ct.dumpAllReport("./");
#endif
}

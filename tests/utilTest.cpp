#ifdef CT
#include "utils/ChronoTrigger.hpp"
using namespace MetaPB::utils;
#endif

#include <chrono>
#include <iostream>
#include <vector>

int main() {
  int rep = 10;
  size_t itemNum = ((size_t)1 << 30) / sizeof(int); // 1GiB
  std::vector<int> a(itemNum, 0);
#ifdef CT
  ChronoTrigger ct;
#endif
  for (int r = 0; r < rep; r++) {
#ifdef CT
    ct.tick("naive");
#endif
    for (size_t i = 0; i < itemNum; i++) {
      a[i]++;
    }
#ifdef CT
    ct.tock("naive");
#endif
  }
#ifdef CT
  ct.dumpAllReport("./");
#endif
}

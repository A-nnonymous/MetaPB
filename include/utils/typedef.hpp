#ifndef TYPEDEF_HPP
#define TYPEDEF_HPP

#include <vector>
using std::vector;
namespace MetaPB {
namespace utils {

typedef struct {
  bool isCoarseGrain = true;
  vector<int> order;
  vector<float> offloadRatio;
} Schedule;
} // namespace utils
} // namespace MetaPB
#endif

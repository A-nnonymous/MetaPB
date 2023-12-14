#ifndef UTL_DPU_HELPER_SRC
#define UTL_DPU_HELPER_SRC
#include "utils/DPUHelper.hpp"
namespace utils {

void faultRankRemoving(uint32_t rankIdx) {
  uint32_t ceilDPUIdx = ((rankIdx / 2) + 1) * 128;
  struct dpu_set_t set;
  DPU_ASSERT(dpu_alloc(num_dpus, NULL, &set));
}
} // namespace utils
#endif
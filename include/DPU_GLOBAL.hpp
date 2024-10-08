#ifndef DPU_GLOBAL_MGR_HPP
#define DPU_GLOBAL_MGR_HPP

extern "C" {
#include <dpu.h>
}

#include <cstdint>
#include <iostream>
#include <memory>

struct GLOBAL_DPU_MGR {
  dpu_set_t dpu_set;
  std::uint32_t dpuNum;
  inline static bool isAllocated = false;
  inline static bool isFreed = false;
  GLOBAL_DPU_MGR(const GLOBAL_DPU_MGR &) = delete;

  GLOBAL_DPU_MGR(std::uint32_t DPU_NUM = DPU_ALLOCATE_ALL) {
    if (!isAllocated) {
      std::cout << "###########################################" << std::endl;
      std::cout << "Global UPMEM-PIM DPU manager initialzing..." << std::endl;
      DPU_ASSERT(dpu_alloc(
          DPU_NUM, "sgXferEnable=true,sgXferMaxBlocksPerDpu=15360", &dpu_set));
      std::uint32_t result;
      DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &result));
      std::cout << "Allocated " << result << " DPU(s)\n";
      this->dpuNum = result;
      isAllocated = true;
      isFreed = false;
    }
  }
  const std::uint32_t getDPUNum() const noexcept {
    std::cout << "returned from MGR dpuNum: "<< dpuNum <<std::endl;
    return dpuNum;
  }
  ~GLOBAL_DPU_MGR() noexcept {
    if (!isFreed) {
      DPU_ASSERT(dpu_free(dpu_set));
      std::cout << "All UPMEM-PIM DPU are safely deallocated." << std::endl;
      std::cout << "###########################################" << std::endl;
      isFreed = true;
      isAllocated = false;
    }
  }
};
#endif

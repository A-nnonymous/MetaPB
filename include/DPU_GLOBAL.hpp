extern "C"{
#include <dpu.h>
}

#include <cstdint>
#include <iostream>

struct GLOBAL_DPU_MGR{
  dpu_set_t dpu_set;

  GLOBAL_DPU_MGR(const GLOBAL_DPU_MGR&) = delete;

  GLOBAL_DPU_MGR(){
    std::cout << "###########################################"<<std::endl;
    std::cout << "Global UPMEM-PIM DPU manager initialzing..."<<std::endl;
    DPU_ASSERT(dpu_alloc(DPU_ALLOCATE_ALL, NULL, &dpu_set));
    std::uint32_t result;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &result));
    std::cout<<"Allocated "<<result <<" DPU(s)\n";
  }
  inline const std::uint32_t getDPUNum()const noexcept{
    std::uint32_t result;
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &result));
    return result;
  }
  ~GLOBAL_DPU_MGR()noexcept{
    DPU_ASSERT(dpu_free(dpu_set));
    std::cout << "All UPMEM-PIM DPU are safely deallocated." << std::endl;
    std::cout << "###########################################"<<std::endl;
  }
};

static GLOBAL_DPU_MGR g_DPU_MGR;
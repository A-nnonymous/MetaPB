#include "Operator/OperatorMAP.hpp"

namespace MetaPB {
namespace Operator {

inline void OperatorMAP::execCPU(const CPU_TCB& cpuTCB) const noexcept {
  sg_xfer_context sgInfo = cpuTCB.sgInfo;
  uint32_t dpuPageBaseIdx = sgInfo.dpuPageBaseIdx;
  uint32_t pageBlkCnt = sgInfo.pageBlkCnt;
  get_block_t get_block_info = {.f = &OperatorBase::get_block, .args = &sgInfo, .args_size = sizeof(sgInfo)};
  DPU_ASSERT(dpu_push_sg_xfer(allDPUs, DPU_XFER_TO_DPU, "buffer", 
                              dpuPageBaseIdx * PAGE_SIZE_BYTE,
                              pageBlkCnt * PAGE_SIZE_BYTE, 
                              &get_block_info, DPU_SG_XFER_DEFAULT));
}

} // namespace Operator
} // namespace MetaPB

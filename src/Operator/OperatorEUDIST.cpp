#include "Operator/OperatorEUDIST.hpp"

namespace MetaPB {
namespace Operator {

inline void OperatorEUDIST::execCPU(const CPU_TCB& cpuTCB) const noexcept {
  char* src1 = (char*)cpuTCB.src1PageBase;
  char* src2 = (char*)cpuTCB.src2PageBase;
  char* dst = (char*)cpuTCB.dstPageBase;
  size_t maxOffset = cpuTCB.pageBlkCnt * pageBlkSize;
  uint32_t itemNum = cpuTCB.pageBlkCnt * pageBlkSize / sizeof(float);
  uint32_t pageItemNum = pageBlkSize / sizeof(float);

  omp_set_num_threads(64);
#pragma omp parallel for
  for(size_t offset = 0; offset < maxOffset; offset += PAGE_SIZE_BYTE){
    float* mySrc1 = (float*)(src1 + offset);
    float* mySrc2 = (float*)(src2 + offset);
    float* myDst = (float*)(dst + offset);
    for (int i = 0; i < pageItemNum; i++) {
      // ignoring sqrt because DPU doesn't have hardware sqrt
      myDst[0] += (mySrc2[i] - mySrc1[i]) * (mySrc2[i] - mySrc1[i]);
    }
  }
}
inline void OperatorEUDIST::execDPU(const DPU_TCB& dpuTCB) const noexcept {
  auto DPU_BINARY = getDPUBinaryPath();
  DPU_ASSERT(dpu_load(allDPUs, DPU_BINARY.c_str(), NULL));

  // Copy input arrays
  DPU_TCB args = dpuTCB;
  /*
  args.src1PageIdx = dpuTCB.src1PageIdx;
  args.src2PageIdx = dpuTCB.src2PageIdx;
  args.dstPageIdx =  dpuTCB.dstPageIdx;
  args.pageCnt =     dpuTCB.pageCnt;
  */
  DPU_ASSERT(dpu_broadcast_to(allDPUs, "dpuTCB", 0, &args,
                              sizeof(args), DPU_XFER_DEFAULT));
  DPU_ASSERT(dpu_launch(allDPUs, DPU_SYNCHRONOUS));
  return;
}

} // namespace Operator
} // namespace MetaPB

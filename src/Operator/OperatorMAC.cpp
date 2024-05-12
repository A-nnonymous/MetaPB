#include "Operator/OperatorMAC.hpp"

namespace MetaPB {
namespace Operator {

inline void OperatorMAC::execCPU(const CPU_TCB& cpuTCB) const noexcept {
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
      myDst[0] += src1[i] * src2[i];
    }
  }
}

inline void OperatorMAC::execDPU(const DPU_TCB& dpuTCB) const noexcept {
  auto DPU_BINARY = getDPUBinaryPath();
  DPU_ASSERT(dpu_load(allDPUs, DPU_BINARY.c_str(), NULL));

  mac_args args;

  args.dpuTCB.src1PageIdx = dpuTCB.src1PageIdx;
  args.dpuTCB.src2PageIdx = dpuTCB.src2PageIdx;
  args.dpuTCB.dstPageIdx =  dpuTCB.dstPageIdx;
  args.dpuTCB.pageCnt =     dpuTCB.pageCnt;
  DPU_ASSERT(dpu_broadcast_to(allDPUs, "DPU_INPUT_ARGUMENTS", 0, &args,
                              sizeof(args), DPU_XFER_DEFAULT));
  DPU_ASSERT(dpu_launch(allDPUs, DPU_SYNCHRONOUS));
  return;
}

} // namespace Operator
} // namespace MetaPB

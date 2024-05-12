#include "Operator/OperatorLOOKUP.hpp"

namespace MetaPB {
namespace Operator {

inline void OperatorLOOKUP::execCPU(const CPU_TCB &cpuTCB) const noexcept {
  char *src = (char *)cpuTCB.src1PageBase;
  char *dst = (char *)cpuTCB.dstPageBase;
  size_t maxOffset = cpuTCB.pageBlkCnt * pageBlkSize;
  uint32_t itemNum = cpuTCB.pageBlkCnt * pageBlkSize / sizeof(float);
  uint32_t pageItemNum = DPU_DMA_BFFR_BYTE / sizeof(float);

#pragma omp parallel for
  for (size_t offset = 0; offset < maxOffset; offset += DPU_DMA_BFFR_BYTE) {
    int *mySrc = (int *)(src + offset);
    int *myDst = (int *)(dst + offset);
    myDst[0] = 0;
    for (int i = 0; i < pageItemNum; i++) {
      if (mySrc[i] == (int)this->target)
        myDst[0]++;
    }
  }
}
inline void OperatorLOOKUP::execDPU(const DPU_TCB &dpuTCB) const noexcept {
  auto DPU_BINARY = getDPUBinaryPath();
  DPU_ASSERT(dpu_load(allDPUs, DPU_BINARY.c_str(), NULL));

  // Copy input arrays
  lookup_args args;
  args.target = this->target;
  args.dpuTCB.src1PageIdx = dpuTCB.src1PageIdx;
  args.dpuTCB.src2PageIdx = dpuTCB.src2PageIdx;
  args.dpuTCB.dstPageIdx = dpuTCB.dstPageIdx;
  args.dpuTCB.pageCnt = dpuTCB.pageCnt;

  DPU_ASSERT(dpu_broadcast_to(allDPUs, "DPU_INPUT_ARGUMENTS", 0, &args,
                              sizeof(args), DPU_XFER_DEFAULT));

  DPU_ASSERT(dpu_launch(allDPUs, DPU_SYNCHRONOUS));
  return;
}

} // namespace Operator
} // namespace MetaPB

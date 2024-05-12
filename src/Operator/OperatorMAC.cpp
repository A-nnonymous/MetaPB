#include "Operator/OperatorMAC.hpp"
#include <stdio.h>
extern "C" {
#include <dpu_log_internals.h>
#include <stdio.h>
}

namespace MetaPB {
namespace Operator {

inline void OperatorMAC::execCPU(const CPU_TCB &cpuTCB) const noexcept {
  char *src1 = (char *)cpuTCB.src1PageBase;
  char *src2 = (char *)cpuTCB.src2PageBase;
  char *dst = (char *)cpuTCB.dstPageBase;
  size_t maxOffset = cpuTCB.pageBlkCnt * pageBlkSize;
  uint32_t itemNum = cpuTCB.pageBlkCnt * pageBlkSize / sizeof(float);
  uint32_t pageItemNum = DPU_DMA_BFFR_BYTE / sizeof(float);

  omp_set_num_threads(64);
#pragma omp parallel for
  for (size_t offset = 0; offset < maxOffset; offset += DPU_DMA_BFFR_BYTE) {
    int *mySrc1 = (int *)(src1 + offset);
    int *mySrc2 = (int *)(src2 + offset);
    int *myDst = (int *)(dst + offset);
    myDst[0] = 0;
    for (int i = 0; i < pageItemNum; i++) {
      myDst[0] += mySrc1[i] * mySrc2[i];
    }
  }
}

inline void OperatorMAC::execDPU(const DPU_TCB &dpuTCB) const noexcept {
  auto DPU_BINARY = getDPUBinaryPath();
  DPU_ASSERT(dpu_load(allDPUs, DPU_BINARY.c_str(), NULL));

  mac_args args;

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

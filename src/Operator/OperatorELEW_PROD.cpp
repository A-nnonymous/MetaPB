#include "Operator/OperatorELEW_PROD.hpp"

namespace MetaPB {
namespace Operator {

inline void OperatorELEW_PROD::execCPU(const CPU_TCB& cpuTCB) const noexcept {
  float* src1 = (float*)cpuTCB.src1PageBase;
  float* src2 = (float*)cpuTCB.src2PageBase;
  float* dst = (float*)cpuTCB.dstPageBase;
  uint32_t itemNum = cpuTCB.pageBlkCnt * pageBlkSize / sizeof(float);

  omp_set_num_threads(64);
#pragma omp parallel for
  for (int i = 0; i < itemNum; i++) {
    dst[i] = src1[i] * src2[i];
  }
}

inline void
OperatorELEW_PROD::execDPU(const DPU_TCB& dpuTCB) const noexcept {
  auto DPU_BINARY = getDPUBinaryPath();
  DPU_ASSERT(dpu_load(allDPUs, DPU_BINARY.c_str(), NULL));
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

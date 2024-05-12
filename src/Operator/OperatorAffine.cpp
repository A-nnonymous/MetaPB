#include "Operator/OperatorAFFINE.hpp"

namespace MetaPB {
namespace Operator {

inline void OperatorAFFINE::execCPU(const CPU_TCB& cpuTCB) const noexcept {
  int* src1 = (int*)cpuTCB.src1PageBase;
  int* src2 = (int*)cpuTCB.src2PageBase;
  int* dst =  (int*)cpuTCB.dstPageBase;
  uint32_t itemNum = cpuTCB.pageBlkCnt * pageBlkSize / sizeof(float);

  omp_set_num_threads(64);
#pragma omp parallel for
  for (int i = 0; i < itemNum; i++) {
    dst[i] = src1[i] * (int)weight + src2[i];
  }
}
inline void OperatorAFFINE::execDPU(const DPU_TCB& dpuTCB) const noexcept {
  auto DPU_BINARY = getDPUBinaryPath();
  DPU_ASSERT(dpu_load(allDPUs, DPU_BINARY.c_str(), NULL));

  // Copy input arrays
  affine_args args;
  args.weight = this->weight;
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

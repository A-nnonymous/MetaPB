#include "Operator/OperatorFILTER.hpp"

namespace MetaPB {
namespace Operator {
inline void OperatorFILTER::execCPU(const CPU_TCB& cpuTCB) const noexcept {
  char* src = (char*)cpuTCB.src1PageBase;
  char* dst = (char*)cpuTCB.dstPageBase;
  size_t maxOffset = cpuTCB.pageBlkCnt * pageBlkSize;
  uint32_t itemNum = cpuTCB.pageBlkCnt * pageBlkSize / sizeof(float);
  uint32_t pageItemNum = DPU_DMA_BFFR_BYTE / sizeof(float);
  size_t padding = (kernelSize - 1) / 2;

  // Perform convolution
  omp_set_num_threads(64);
#pragma omp parallel for
  for(size_t offset = 0; offset < maxOffset; offset += DPU_DMA_BFFR_BYTE){
    int* mySrc = (int*)(src + offset);
    int* myDst = (int*)(dst + offset);

    for (size_t i = 0; i < pageItemNum; ++i) {
      int sum = 0;
      for (size_t j = 0; j < kernelSize; ++j) {
        if(i + j < itemNum) sum += mySrc[i + j] / kernelSize;
      }
      myDst[i] = sum / kernelSize;
    }
  }
}

inline void OperatorFILTER::execDPU(const DPU_TCB& dpuTCB) const noexcept {
  auto DPU_BINARY = getDPUBinaryPath();
  DPU_ASSERT(dpu_load(allDPUs, DPU_BINARY.c_str(), NULL));
  filter_args args;
  for (int i = 0; i < 8; i++) {
    args.gaussianKernel[i] = this->gaussianKernel[i];
  }
  args.stride = this->stride;
  args.kernelSize = this->kernelSize;
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

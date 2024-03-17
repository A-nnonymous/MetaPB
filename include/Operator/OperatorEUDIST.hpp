#ifndef OP_EUDIST_HPP
#define OP_EUDIST_HPP

#include "Operator/OperatorBase.hpp"
#include "Operator/dpu/common.h"

namespace MetaPB {
namespace Operator {

class OperatorEUDIST : public OperatorBase {
public:

  OperatorEUDIST(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR) : OperatorBase(g_DPU_MGR) {}
  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }
  inline virtual constexpr int getInputTensorNum() const noexcept override {
    return 2;
  }
  inline virtual void execCPU(const size_t batchSize_MiB,
                              void **memPoolBffrPtrs) const noexcept override {
    size_t inputSize = batchSize_MiB * 1024 * 1024 / sizeof(float);
    float *src1 = static_cast<float *>(memPoolBffrPtrs[0]);
    float *src2 = static_cast<float *>(memPoolBffrPtrs[1]);
    float *dst = static_cast<float *>(memPoolBffrPtrs[2]);
#pragma omp parallel for
    for (int i = 0; i < inputSize; i++) {
      // ignoring sqrt because DPU doesn't have hardware sqrt
      dst[i] += (src1[i] - src2[i]) * (src1[i] - src2[i]);
    }
  }
  inline virtual void
  execDPU(const size_t batchSize_MiB) const noexcept override {
    auto DPU_BINARY = getDPUBinaryPath();
    std::cout << "Loading DPU program "<<DPU_BINARY <<std::endl;
    DPU_ASSERT(dpu_load(allDPUs, DPU_BINARY.c_str(), NULL));
    uint32_t nr_of_dpus;
    DPU_ASSERT(dpu_get_nr_dpus(allDPUs, &nr_of_dpus));
    size_t bytes = batchSize_MiB * (1 << 20);
    const size_t input_size_dpu =
        divceil(bytes, nr_of_dpus); // Input size per DPU (max.)
    const size_t input_size_dpu_8bytes =
        (input_size_dpu % 8) != 0
            ? roundup(input_size_dpu, 8)
            : input_size_dpu; // Input size per DPU (max.), 8-byte aligned

    // Copy input arrays
    common_args args;
    args.inputSize = getInputTensorNum() * input_size_dpu_8bytes;
    args.operandNum = getInputTensorNum();

    DPU_ASSERT(dpu_broadcast_to(allDPUs, "DPU_INPUT_ARGUMENTS", 0, &args,
                                sizeof(args), DPU_XFER_DEFAULT));
    DPU_ASSERT(dpu_launch(allDPUs, DPU_SYNCHRONOUS));
    return;
  }

  virtual inline constexpr bool checkIfIsTrainable() const noexcept override {
    return true;
  }
  virtual inline constexpr bool checkIfIsCPUOnly() const noexcept override {
    return false;
  }

private:
  inline static const std::string OpName = "EUDIST";
};
} // namespace Operator
} // namespace MetaPB
#endif

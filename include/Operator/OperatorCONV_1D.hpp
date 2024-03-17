#ifndef OP_CONV_HPP
#define OP_CONV_HPP

extern "C" {
#include "Operator/dpu/CONV_1D.h"
}

#include "Operator/OperatorBase.hpp"

using MetaPB::Operator::OperatorBase;
namespace MetaPB {
namespace Operator {

class OperatorCONV_1D : public OperatorBase {
public:

  OperatorCONV_1D(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR) : OperatorBase(g_DPU_MGR) {}
  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }
  inline virtual constexpr int getInputTensorNum() const noexcept override {
    return 1;
  }
  inline virtual void execCPU(const size_t batchSize_MiB,
                              void **memPoolBffrPtrs) const noexcept override {
    size_t inputSize = batchSize_MiB * 1024 * 1024 / sizeof(float);
    size_t padding = (kernelSize - 1) / 2;

    // Assuming memPoolBffrPtrs is an array of pointers to float
    float *inputBuffer = static_cast<float *>(memPoolBffrPtrs[0]);
    float *outputBuffer = static_cast<float *>(memPoolBffrPtrs[1]);

    // Perform convolution
    for (size_t i = 0; i < inputSize; ++i) {
      float sum = 0.0f;
      for (size_t j = 0; j < kernelSize; ++j) {
        // Compute the input index, considering padding
        int inputIndex = i - padding + j;
        // Check if the input index is within bounds
        if (inputIndex >= 0 && inputIndex < inputSize) {
          sum += inputBuffer[inputIndex] * gaussianKernel[j];
        }
      }
      outputBuffer[i] = sum;
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
    conv_args args;
    for (int i = 0; i < 8; i++) {
      args.gaussianKernel[i] = this->gaussianKernel[i];
    }
    args.stride = this->stride;
    args.co.inputSize = input_size_dpu * getInputTensorNum();
    args.co.operandNum = 1;
    args.kernelSize = this->kernelSize;

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
  const int kernelSize = 8;
  const int stride = 1;
  const float gaussianKernel[8] = {
      0.000872710786525902, 0.0175288647260302,   0.129521764811203,
      0.352076659676240,    0.352076659676240,    0.129521764811203,
      0.0175288647260302,   0.000872710786525902,
  };

  inline static const std::string OpName = "CONV_1D";
};
} // namespace Operator
} // namespace MetaPB
#endif

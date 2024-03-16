#ifndef OP_CONV_HPP
#define OP_CONV_HPP

#include "Operator/OperatorBase.hpp"

using MetaPB::Operator::OperatorBase;
namespace MetaPB {
namespace Operator {

class OperatorCONV_1D : public OperatorBase {
public:
  OperatorCONV_1D() {}

  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }

  inline virtual void execCPU(const size_t batchSize_MiB,
                              void **memPoolBffrPtrs) const noexcept override {
    size_t inputSize = batchSize_MiB * 1024 * 1024 / sizeof(float);
    size_t padding = (kernelSize - 1) / 2;

    // Assuming memPoolBffrPtrs is an array of pointers to float
    float* inputBuffer = static_cast<float*>(memPoolBffrPtrs[0]);
    float* outputBuffer = static_cast<float*>(memPoolBffrPtrs[1]);

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
    return;
  }

  virtual inline constexpr bool checkIfIsTrainable() const noexcept override {
    return true;
  }
  virtual inline constexpr bool checkIfIsCPUOnly() const noexcept override {
    return true;
  }

private:
  const int kernelSize = 8;
  const int stride = 1;
  const float gaussianKernel[8] = {0.000872710786525902 ,
                                    0.0175288647260302  ,
                                    0.129521764811203   ,
                                    0.352076659676240   ,
                                    0.352076659676240   ,
                                    0.129521764811203   ,
                                    0.0175288647260302  ,
                                    0.000872710786525902,};

  inline static const std::string OpName = "CONV_1D";
};
} // namespace Operator
} // namespace MetaPB
#endif

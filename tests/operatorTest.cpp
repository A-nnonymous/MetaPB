#include "Operator/OperatorRegistry.hpp"
#include "Operator/OperatorManager.hpp"
#include <iostream>
#include <vector>
#include <memory>

using namespace MetaPB::Operator;

/*
class Operator : public OperatorBase {
public:
  Operator() {}

  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }

  inline virtual void
  execCPU(const size_t batchSize_MiB, void** memPoolBffrPtrs) const noexcept override {
    int* a = (int*)memPoolBffrPtrs[0];
    int* b = (int*)memPoolBffrPtrs[1];
    int item = batchSize_MiB * (1<<20)/sizeof(int);
    for(int i = 0; i < item; i++){
      a[i] += b[i];
    }
  }
  inline virtual void
  execDPU(const size_t batchSize_MiB) const noexcept override {
    std::cout << "exec DPU with batch" << batchSize_MiB << std::endl;
    return;
  }

  virtual inline constexpr bool checkIfIsTrainable() const noexcept override {
    return true;
  }
  virtual inline constexpr bool checkIfIsCPUOnly() const noexcept override {
    return true;
  }

private:
  inline static const std::string OpName = "abb";
};
*/

int main() {
  std::unique_ptr<GLOBAL_DPU_MGR> g_DPU_MGR = std::make_unique<GLOBAL_DPU_MGR>();
  OperatorCONV_1D a(g_DPU_MGR);
  int* src1 = (int*) malloc((1<<20) * 512);
  int* src2 = (int*) malloc((1<<20) * 512);
  int* input[2] = {src1, src2};
  if (a.checkIfIsTrainable()) {
    a.trainModel(256, (void**)input);
    std::cout << "a is " << (a.checkIfIsTrained() ? "trained" : "untrain")
              << std::endl;
    std::cout << "deduced perf is : " << a.deducePerf(0, 200).timeCost_Second
              << std::endl;
  }

  return 0;
}

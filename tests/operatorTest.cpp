#include "Operator/OperatorManager.hpp"
#include "Operator/OperatorRegistry.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

using namespace MetaPB::Operator;
using MetaPB::Operator::OperatorTag;

/*
class Operator : public OperatorBase {
public:
  Operator() {}

  inline virtual const std::string get_name() const noexcept override {
    return OpName;
  }

  inline virtual void
  execCPU(const size_t batchSize_MiB, void** memPoolBffrPtrs) const noexcept
override { int* a = (int*)memPoolBffrPtrs[0]; int* b = (int*)memPoolBffrPtrs[1];
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
  /*
  std::unique_ptr<GLOBAL_DPU_MGR> g_DPU_MGR =
  std::make_unique<GLOBAL_DPU_MGR>(); OperatorREDUCE  a(g_DPU_MGR); int* src1 =
  (int*) malloc((1<<20) * 512); int* src2 = (int*) malloc((1<<20) * 512); int*
  src3 = (int*) malloc((1<<20) * 512); int* input[3] = {src1, src2, src3}; if
  (a.checkIfIsTrainable()) { a.trainModel(256, (void**)input); std::cout << "a
  is " << (a.checkIfIsTrained() ? "trained" : "untrain")
              << std::endl;
    std::cout << "deduced perf is : " << a.deducePerf(0, 200).timeCost_Second
              << std::endl;
  }
  */
  /*
  size_t bytes = batchSize_MiB * (1 <<20);
  void* src1 = (void*) malloc(bytes);
  void* src2 = (void*) malloc(bytes);
  void* dst1 = (void*) malloc(bytes);
  void* allBffrPtrs[3] = {src1, src2, dst1};
  auto va = om.getOperator(OperatorTag::ELEW_ADD);
  va->trainModel(batchSize_MiB,allBffrPtrs, 300);
  va->verifyRegression("./", batchSize_MiB);
  */
  return 0;
}

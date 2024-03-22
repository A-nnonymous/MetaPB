#ifndef OP_MNGR
#define OP_MNGR
#include "Executor/TaskGraph.hpp"
#include "Operator/OperatorBase.hpp"
#include "Operator/OperatorCONV_1D.hpp"
#include "Operator/OperatorDOT_ADD.hpp"
#include "Operator/OperatorDOT_PROD.hpp"
#include "Operator/OperatorEUDIST.hpp"
#include "Operator/OperatorLOGIC_END.hpp"
#include "Operator/OperatorLOGIC_START.hpp"
#include "Operator/OperatorLOOKUP.hpp"
#include "Operator/OperatorMAC.hpp"
#include "Operator/OperatorMAP.hpp"
#include "Operator/OperatorREDUCE.hpp"
#include "Operator/OperatorRegistry.hpp"
#include "Operator/OperatorUNDEFINED.hpp"
#include "utils/Stats.hpp"
#include <cstdlib>
#include <map>
#include <memory>
#include <string>

namespace MetaPB {
using perfStats = utils::perfStats;
using regressionTask = utils::regressionTask;
namespace Operator {

struct OperatorManager {
  void instantiateAll() {
    for (const auto &opTag : allOPSet) {
      opMap[opTag] = getOperator(opTag);
    }
  }
  void trainModel(const regressionTask &task) {
    size_t maxMiB = 0;
    for (const auto &[opTag, batchUpBound_MiB] : task) {
      if (batchUpBound_MiB > maxMiB)
        maxMiB = batchUpBound_MiB;
    }
    size_t bytes = maxMiB * size_t(1 << 20);
    void *src1 = (void *)malloc(bytes);
    void *src2 = (void *)malloc(bytes);
    void *dst1 = (void *)malloc(bytes);
    void *allBffrPtrs[3] = {src1, src2, dst1};

    for (auto &[opTag, batchUpBound_MiB] : task) {
      if (!opMap.contains(opTag)) {
        opMap[opTag] = std::move(this->getOperator(opTag));
      }
      std::cout << "Training model of " << tag2Name.at(opTag) << std::endl;
      opMap[opTag]->trainModel(batchUpBound_MiB, allBffrPtrs);
    }
    // Always train xfer operator, bloody experience.
    opMap[OperatorTag::MAP] = std::move(this->getOperator(OperatorTag::MAP));
    opMap[OperatorTag::REDUCE] =
        std::move(this->getOperator(OperatorTag::REDUCE));
    opMap[OperatorTag::MAP]->trainModel(maxMiB, allBffrPtrs);
    opMap[OperatorTag::REDUCE]->trainModel(maxMiB, allBffrPtrs);
    free(dst1);
    free(src2);
    free(src1);
  }

  void trainAll(size_t batchSize_MiB, int iter) {
    size_t bytes = batchSize_MiB * size_t(1 << 20);
    void *src1 = (void *)malloc(bytes);
    void *src2 = (void *)malloc(bytes);
    void *dst1 = (void *)malloc(bytes);
    void *allBffrPtrs[3] = {src1, src2, dst1};
    for (const auto &opSet : allPerfRelOPSet) {
      for (const auto &opTag : opSet) {
        if (!opMap.contains(opTag)) {
          opMap[opTag] = std::move(this->getOperator(opTag));
        }
        std::cout << "Training model of " << tag2Name.at(opTag) << std::endl;
        opMap[opTag]->trainModel(batchSize_MiB, allBffrPtrs, iter);
      }
    }
    free(dst1);
    free(src2);
    free(src1);
  }

  void verifyAll(const std::string &filePath,
                 const size_t batchUpperBound_MiB) {
    trainAll(batchUpperBound_MiB, 200);
    for (const auto &opSet : allPerfRelOPSet) {
      for (const auto &opTag : opSet) {
        opMap[opTag]->verifyRegression(filePath, batchUpperBound_MiB);
      }
    }
  }

  inline void execCPU(OperatorTag opTag, const size_t batchSize_MiB,
                      void **memPoolBffrPtrs) const noexcept {
    opMap.at(opTag)->execCPU(batchSize_MiB, memPoolBffrPtrs);
  }
  inline void execDPU(OperatorTag opTag,
                      const size_t batchSize_MiB) const noexcept {
    opMap.at(opTag)->execDPU(batchSize_MiB);
  }

  perfStats deducePerf(OperatorTag opTag, double offloadRatio,
                       size_t batchSize_MiB) {
    return opMap.at(opTag)->deducePerf(offloadRatio, batchSize_MiB);
  }
  perfStats deducePerfCPU(OperatorTag opTag, size_t batchSize_MiB) const {
    return opMap.at(opTag)->deducePerfCPU(batchSize_MiB);
  }
  perfStats deducePerfDPU(OperatorTag opTag, size_t batchSize_MiB) const {
    return opMap.at(opTag)->deducePerfDPU(batchSize_MiB);
  }

  std::unique_ptr<OperatorBase> getOperator(OperatorTag tag) {
    switch (tag) {
    case OperatorTag::CONV_1D:
      return std::make_unique<OperatorCONV_1D>(g_DPU_MGR);
    case OperatorTag::DOT_ADD:
      return std::make_unique<OperatorDOT_ADD>(g_DPU_MGR);
    case OperatorTag::DOT_PROD:
      return std::make_unique<OperatorDOT_PROD>(g_DPU_MGR);
    case OperatorTag::EUDIST:
      return std::make_unique<OperatorEUDIST>(g_DPU_MGR);
    case OperatorTag::LOGIC_END:
      return std::make_unique<OperatorLOGIC_END>(g_DPU_MGR);
    case OperatorTag::LOGIC_START:
      return std::make_unique<OperatorLOGIC_START>(g_DPU_MGR);
    case OperatorTag::LOOKUP:
      return std::make_unique<OperatorLOOKUP>(g_DPU_MGR);
    case OperatorTag::MAC:
      return std::make_unique<OperatorMAC>(g_DPU_MGR);
    case OperatorTag::MAP:
      return std::make_unique<OperatorMAP>(g_DPU_MGR);
    case OperatorTag::REDUCE:
      return std::make_unique<OperatorREDUCE>(g_DPU_MGR);
    case OperatorTag::UNDEFINED:
      return std::make_unique<OperatorUNDEFINED>(g_DPU_MGR);
    }
  }
  OperatorManager(std::uint32_t dpuNum = DPU_ALLOCATE_ALL) : dpuNum(dpuNum) {
    g_DPU_MGR = std::make_unique<GLOBAL_DPU_MGR>(dpuNum);
  }
  std::map<OperatorTag, std::unique_ptr<OperatorBase>> opMap;
  /*
  inline static std::unique_ptr<GLOBAL_DPU_MGR> g_DPU_MGR =
      std::make_unique<GLOBAL_DPU_MGR>();
      */
  std::unique_ptr<GLOBAL_DPU_MGR> g_DPU_MGR;

  const std::uint32_t dpuNum;
};

} // namespace Operator
} // namespace MetaPB
#endif

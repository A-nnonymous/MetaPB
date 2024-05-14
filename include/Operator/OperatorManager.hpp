#ifndef OP_MNGR
#define OP_MNGR
#include "Executor/TaskGraph.hpp"
#include "Operator/OperatorAFFINE.hpp"
#include "Operator/OperatorBase.hpp"
#include "Operator/OperatorCONV_1D.hpp"
#include "Operator/OperatorELEW_ADD.hpp"
#include "Operator/OperatorELEW_PROD.hpp"
#include "Operator/OperatorEUDIST.hpp"
#include "Operator/OperatorFILTER.hpp"
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
    size_t maxBlk = 0;
    for (const auto &[opTag, dataSizeUpperBound_MiB] : task) {
      size_t blkNum = getNearestPageBlkCnt(dataSizeUpperBound_MiB);
      if (blkNum > maxBlk)
        maxBlk = blkNum;
    }
    for (auto &[opTag, pageBlkUpperBound] : task) {
      if (!opMap.contains(opTag)) {
        opMap[opTag] = std::move(this->getOperator(opTag));
      }
      std::cout << "Training model of " << tag2Name.at(opTag) << std::endl;
      opMap[opTag]->trainModel(maxBlk);
    }
    // Always train xfer operator, bloody experience.
    opMap[OperatorTag::MAP] = std::move(this->getOperator(OperatorTag::MAP));
    opMap[OperatorTag::REDUCE] =
        std::move(this->getOperator(OperatorTag::REDUCE));
    opMap[OperatorTag::MAP]->trainModel(maxBlk);
    opMap[OperatorTag::REDUCE]->trainModel(maxBlk);
  }

  void trainAll(uint32_t pageBlkUpperBound) {
    for (const auto &opSet : allPerfRelOPSet) {
      for (const auto &opTag : opSet) {
        if (!opMap.contains(opTag)) {
          opMap[opTag] = std::move(this->getOperator(opTag));
        }
        std::cout << "Training model of " << tag2Name.at(opTag) << std::endl;
        opMap[opTag]->trainModel(pageBlkUpperBound);
      }
    }
  }

  inline void execCPU(OperatorTag opTag, const CPU_TCB &cpuTCB) const noexcept {
    opMap.at(opTag)->execCPU(cpuTCB);
  }
  inline void execDPU(OperatorTag opTag, const DPU_TCB &dpuTCB) const noexcept {
    opMap.at(opTag)->execDPU(dpuTCB);
  }

  inline perfStats execCPUwithProbe(OperatorTag opTag,
                                    const CPU_TCB &cpuTCB) const noexcept {
    return {opMap.at(opTag)->execCPUwithProbe(cpuTCB)};
  }

  inline perfStats execDPUwithProbe(OperatorTag opTag,
                                    const DPU_TCB &dpuTCB) const noexcept {
    return {opMap.at(opTag)->execDPUwithProbe(dpuTCB)};
  }

  perfStats deducePerfCPU(OperatorTag opTag, const uint32_t pageBlkCnt) const {
    return opMap.at(opTag)->deducePerfCPU(pageBlkCnt);
  }
  perfStats deducePerfDPU(OperatorTag opTag, const uint32_t pageBlkCnt) const {
    return opMap.at(opTag)->deducePerfDPU(pageBlkCnt);
  }

  std::unique_ptr<OperatorBase> getOperator(OperatorTag tag) {
    switch (tag) {
    case OperatorTag::CONV_1D:
      return std::make_unique<OperatorCONV_1D>(g_DPU_MGR);
    case OperatorTag::ELEW_ADD:
      return std::make_unique<OperatorELEW_ADD>(g_DPU_MGR);
    case OperatorTag::ELEW_PROD:
      return std::make_unique<OperatorELEW_PROD>(g_DPU_MGR);
    case OperatorTag::EUDIST:
      return std::make_unique<OperatorEUDIST>(g_DPU_MGR);
    case OperatorTag::LOGIC_END:
      return std::make_unique<OperatorLOGIC_END>(g_DPU_MGR);
    case OperatorTag::LOGIC_START:
      return std::make_unique<OperatorLOGIC_START>(g_DPU_MGR);
    case OperatorTag::LOOKUP:
      return std::make_unique<OperatorLOOKUP>(g_DPU_MGR);
    case OperatorTag::AFFINE:
      return std::make_unique<OperatorAFFINE>(g_DPU_MGR);
    case OperatorTag::MAP:
      return std::make_unique<OperatorMAP>(g_DPU_MGR);
    case OperatorTag::REDUCE:
      return std::make_unique<OperatorREDUCE>(g_DPU_MGR);
    case OperatorTag::FILTER:
      return std::make_unique<OperatorFILTER>(g_DPU_MGR);
    case OperatorTag::MAC:
      return std::make_unique<OperatorMAC>(g_DPU_MGR);
    case OperatorTag::UNDEFINED:
      return std::make_unique<OperatorUNDEFINED>(g_DPU_MGR);
    }
  }
  OperatorManager(std::uint32_t dpuNum = DPU_ALLOCATE_ALL) : dpuNum(dpuNum) {
    g_DPU_MGR = std::make_unique<GLOBAL_DPU_MGR>(dpuNum);
    instantiateAll();
    pageBlkSize = opMap.at(OperatorTag::MAP)->getPageBlkSize();
  }
  inline uint32_t getPageBlkSize() const noexcept { return pageBlkSize; }
  std::map<OperatorTag, std::unique_ptr<OperatorBase>> opMap;
  /*
  inline static std::unique_ptr<GLOBAL_DPU_MGR> g_DPU_MGR =
      std::make_unique<GLOBAL_DPU_MGR>();
      */
  inline uint32_t getNearestPageBlkCnt(size_t inputSize_MiB) const noexcept{
    return ((size_t)inputSize_MiB * (size_t)(1<<20) + pageBlkSize - 1) / pageBlkSize;
  }
  std::unique_ptr<GLOBAL_DPU_MGR> g_DPU_MGR;

  const std::uint32_t dpuNum;
  uint32_t pageBlkSize;
};

} // namespace Operator
} // namespace MetaPB
#endif

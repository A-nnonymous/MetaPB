#ifndef OP_MNGR
#define OP_MNGR
#include "Executor/TaskGraph.hpp"
#include "Operator/OperatorRegistry.hpp"
#include <map>
#include <memory>
#include <string>

namespace MetaPB {
using MetaPB::Executor::perfStats;
using MetaPB::Executor::regressionTask;
namespace Operator {

struct OperatorManager {

  void instantiateOpSet(const regressionTask &task) {
    for (const auto &[opTag, _] : task) {
      auto op = this->getOperator(opTag);
      opMap[opTag] = std::move(op);
    }
  }

  void trainModel(regressionTask task, void** memoryPoolBffrPtrs) {
    for (auto &[opTag, batchUpBound] : task) {
      opMap[opTag]->trainModel(batchUpBound, memoryPoolBffrPtrs);
    }
  }

  perfStats deducePerf(OperatorTag opTag, double offloadRatio,
                       size_t batchSize_MiB) {
    return opMap[opTag]->deducePerf(offloadRatio, batchSize_MiB);
  }

private:
  std::unique_ptr<OperatorBase> getOperator(OperatorTag tag) {
    switch (tag) {
    case OperatorTag::CONV_1D:
      return std::make_unique<OperatorCONV_1D>();
    case OperatorTag::DOT_ADD:
      return std::make_unique<OperatorDOT_ADD>();
    case OperatorTag::DOT_PROD:
      return std::make_unique<OperatorDOT_PROD>();
    case OperatorTag::EUDIST:
      return std::make_unique<OperatorEUDIST>();
    case OperatorTag::LOGIC_END:
      return std::make_unique<OperatorLOGIC_END>();
    case OperatorTag::LOGIC_START:
      return std::make_unique<OperatorLOGIC_START>();
    case OperatorTag::LOOKUP:
      return std::make_unique<OperatorLOOKUP>();
    case OperatorTag::MAC:
      return std::make_unique<OperatorMAC>();
    case OperatorTag::MAP:
      return std::make_unique<OperatorMAP>();
    case OperatorTag::REDUCE:
      return std::make_unique<OperatorREDUCE>();
    case OperatorTag::UNDEFINED:
      return std::make_unique<OperatorUNDEFINED>();
    }
  }

  std::map<OperatorTag, std::unique_ptr<OperatorBase>> opMap;
};
} // namespace Operator
} // namespace MetaPB
#endif
#ifndef OP_MNGR
#define OP_MNGR
#include "Executor/TaskGraph.hpp"
#include "Operator/OperatorAFFINE.hpp"
#include "Operator/OperatorBase.hpp"
#include "Operator/OperatorCONV_1D.hpp"
#include "Operator/OperatorELEW_ADD.hpp"
#include "Operator/OperatorELEW_PROD.hpp"
#include "Operator/OperatorEUDIST.hpp"
#include "Operator/OperatorLOGIC_END.hpp"
#include "Operator/OperatorLOGIC_START.hpp"
#include "Operator/OperatorLOOKUP.hpp"
#include "Operator/OperatorMAP.hpp"
#include "Operator/OperatorREDUCE.hpp"
#include "Operator/OperatorRegistry.hpp"
#include "Operator/OperatorUNDEFINED.hpp"
#include "utils/Stats.hpp"
#include <map>
#include <memory>
#include <string>

namespace MetaPB {
using perfStats = utils::perfStats;
using regressionTask = utils::regressionTask;
namespace Operator {

/*
struct OperatorManager {

void instantiateOpSet(const regressionTask &task) {
  for (const auto &[opTag, _] : task) {
    auto op = this->getOperator(opTag);
    opMap[opTag] = std::move(op);
  }
}

void trainModel(regressionTask task, void **memoryPoolBffrPtrs) {
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
  case OperatorTag::ELEW_ADD:
    return std::make_unique<OperatorELEW_ADD>();
  case OperatorTag::ELEW_PROD:
    return std::make_unique<OperatorELEW_PROD>();
  case OperatorTag::EUDIST:
    return std::make_unique<OperatorEUDIST>();
  case OperatorTag::LOGIC_END:
    return std::make_unique<OperatorLOGIC_END>();
  case OperatorTag::LOGIC_START:
    return std::make_unique<OperatorLOGIC_START>();
  case OperatorTag::LOOKUP:
    return std::make_unique<OperatorLOOKUP>();
  case OperatorTag::AFFINE:
    return std::make_unique<OperatorAFFINE>();
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
*/
} // namespace Operator
} // namespace MetaPB
#endif

#ifndef OP_MNGR
#define OP_MNGR
#include <string>
#include <memory>
#include "Executor/TaskGraph.hpp"
#include "Operator/OperatorBase.hpp"

using regressionTask =  MetaPB::Executor::regressionTask;
using perfStats = MetaPB::Executor::perfStats;
using OperatorBase = MetaPB::Operator::OperatorBase;

namespace MetaPB {
namespace Operator {

enum class OperatorTag {
  VM,           // Vectorized Multiplication
  VA,           // Vectorized Addition
  MAC,          // Vectorized MAC
  EUDIST,       // Vector Euclidean-distance(modified)
  CONV_1D,      // Convolution in 1 dimension
  LOOKUP,       // Table lookup
  DOT_PROD,     // Vector dot product
  LOGIC_START,  // Logical Start Operator
  MAP_START,    // Start with a data mapping
  LOGIC_END,    // Logical End Operator
  REDUCE_END,   // End with a reduce
  UNDEFINED     // Undefined Operator
};

enum class OperatorType {
  CoumputeBound,
  MemoryBound,
  // ------- CPU Only ----------
  Logical,
  Map,
  Reduce,
  Undefined
  // ------- CPU Only ----------
};


struct OperatorManager{
  void instantiateOpSet(const regressionTask& task){
    for(const auto& [opTag, _] : task){
      auto op = this->getOperator(opTag);
      opMap[opTag] = std::move(op);
    }
  }

  void trainModel(regressionTask task){
    for(auto& [opTag, batchUpBound] : task){
      opMap[opTag]->trainModel(batchUpBound);
    }
  }

  perfStats deducePerf(OperatorTag opTag, double offloadRatio, size_t batchSize){
    return opMap[opTag]->deducePerf(offloadRatio,batchSize);
  }

private:
  std::unique_ptr<OperatorBase>
  getOperator(OperatorTag tag){
    switch(tag){
      case OperatorTag::VM         : return make_unique<OperatorVM         >();
      case OperatorTag::VA         : return make_unique<OperatorVA         >();
      case OperatorTag::MAC        : return make_unique<OperatorMAC        >();
      case OperatorTag::EUDIST     : return make_unique<OperatorEUDIST     >();
      case OperatorTag::CONV_1D    : return make_unique<OperatorCONV_1D    >();
      case OperatorTag::LOOKUP     : return make_unique<OperatorLOOKUP     >();
      case OperatorTag::DOT_PROD   : return make_unique<OperatorDOT_PROD   >();
      case OperatorTag::LOGIC_START: return make_unique<OperatorLOGIC_START>();
      case OperatorTag::MAP_START  : return make_unique<OperatorMAP_START  >();
      case OperatorTag::LOGIC_END  : return make_unique<OperatorLOGIC_END  >();
      case OperatorTag::REDUCE_END : return make_unique<OperatorREDUCE_END >();
      case OperatorTag::UNDEFINED  : return make_unique<OperatorUNDEFINED  >();
    }
  }

  std::map<OperatorTag, std::unique_ptr<OperatorBase>> opMap;
};
} // namespace Operator
} // namespace MetaPB
#endif
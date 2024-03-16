#ifndef OP_REGISTRY
#define OP_REGISTRY
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
#include "Operator/OperatorUNDEFINED.hpp"
#include <set>

using std::set;
namespace MetaPB {
namespace Operator {

enum class OperatorTag {
  MAC,         // Vectorized MAC
  EUDIST,      // Vector Euclidean-distance(modified)
  CONV_1D,     // Convolution in 1 dimension
  LOOKUP,      // Table lookup
  DOT_PROD,    // Vector dot product
  DOT_ADD,     // Vector dot adding
  LOGIC_START, // Logical Start Operator
  LOGIC_END,   // Logical End Operator
  MAP,         // Transfer Operator that maps data to DPU
  REDUCE,      // Transfer Operator that gather data from DPU
  UNDEFINED    // Undefined Operator
};

static const set<OperatorTag> computeBoundOPSet = {
    OperatorTag::MAC,
    OperatorTag::EUDIST,
    OperatorTag::CONV_1D,
};
static const set<OperatorTag> memoryBoundOPSet = {
    OperatorTag::LOOKUP,
    OperatorTag::DOT_PROD,
    OperatorTag::DOT_ADD,
};

static const set<OperatorTag> xferOPSet = {OperatorTag::MAP,
                                           OperatorTag::REDUCE};

/// @brief Set of all performance related Operator set.
static const set<set<OperatorTag>> allPerfRelOPSet = {
    computeBoundOPSet, memoryBoundOPSet, xferOPSet};

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

} // namespace Operator
} // namespace MetaPB
#endif
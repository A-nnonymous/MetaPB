#ifndef OP_REGISTRY
#define OP_REGISTRY
#include <map>
#include <set>
#include <string>

using std::map;
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

static const map<OperatorTag, std::string> tag2Name = {
    {OperatorTag::MAC, "MAC"},
    {OperatorTag::EUDIST, "EUDIST"},
    {OperatorTag::CONV_1D, "CONV_1D"},
    {OperatorTag::LOOKUP, "LOOKUP"},
    {OperatorTag::DOT_PROD, "DOT_PROD"},
    {OperatorTag::DOT_ADD, "DOT_ADD"},
    {OperatorTag::LOGIC_START, "LOGIC_START"},
    {OperatorTag::LOGIC_END, "LOGIC_END"},
    {OperatorTag::MAP, "MAP"},
    {OperatorTag::REDUCE, "REDUCE"},
    {OperatorTag::UNDEFINED, "UNDEFINED"},
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
  ComputeBound,
  MemoryBound,
  // ------- CPU Only ----------
  Logical,
  Map,
  Reduce,
  Undefined
  // ------- CPU Only ----------
};

static const map<OperatorType, std::string> opType2Name = {
    {OperatorType::ComputeBound, "ComputeBound"},
    {OperatorType::MemoryBound, "MemoryBound"},
    {OperatorType::Logical, "Logical"},
    {OperatorType::Map, "Map"},
    {OperatorType::Reduce, "Reduce"},
    {OperatorType::Undefined,"Undefined"}
};

} // namespace Operator
} // namespace MetaPB
#endif

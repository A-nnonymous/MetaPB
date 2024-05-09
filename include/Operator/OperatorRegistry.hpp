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
  AFFINE,      // Vectorized AFFINE
  EUDIST,      // Vector Euclidean-distance(modified)
  CONV_1D,     // Convolution in 1 dimension
  LOOKUP,      // Table lookup
  ELEW_PROD,   // Vector dot product
  ELEW_ADD,    // Vector dot adding
  LOGIC_START, // Logical Start Operator
  LOGIC_END,   // Logical End Operator
  MAP,         // Transfer Operator that maps data to DPU
  REDUCE,      // Transfer Operator that gather data from DPU
  MAC,         // Vector Multiply-Accumulate operator
  FILTER,      // Windowed Average Operator
  UNDEFINED    // Undefined Operator
};

static const map<OperatorTag, std::string> tag2Name = {
    {OperatorTag::AFFINE, "AFFINE"},
    {OperatorTag::EUDIST, "EUDIST"},
    {OperatorTag::CONV_1D, "CONV_1D"},
    {OperatorTag::LOOKUP, "LOOKUP"},
    {OperatorTag::ELEW_PROD, "ELEW_PROD"},
    {OperatorTag::ELEW_ADD, "ELEW_ADD"},
    {OperatorTag::LOGIC_START, "LOGIC_START"},
    {OperatorTag::LOGIC_END, "LOGIC_END"},
    {OperatorTag::MAP, "MAP"},
    {OperatorTag::REDUCE, "REDUCE"},
    {OperatorTag::MAC, "MAC"},
    {OperatorTag::FILTER, "FILTER"},
    {OperatorTag::UNDEFINED, "UNDEFINED"},
};

/*
static const set<OperatorTag> computeBoundOPSet = {
    OperatorTag::FILTER,
    OperatorTag::CONV_1D,
};
static const set<OperatorTag> memoryBoundOPSet = {
    OperatorTag::AFFINE, OperatorTag::MAC,       OperatorTag::EUDIST,
    OperatorTag::LOOKUP, OperatorTag::ELEW_PROD, OperatorTag::ELEW_ADD,
};
*/
static const set<OperatorTag> computeBoundOPSet = {
    OperatorTag::FILTER
};
static const set<OperatorTag> memoryBoundOPSet = {
    OperatorTag::AFFINE, OperatorTag::MAC
};

static const set<OperatorTag> xferOPSet = {OperatorTag::MAP,
                                           OperatorTag::REDUCE};

static const set<set<OperatorTag>> hybridOPSet = {computeBoundOPSet,
                                                  memoryBoundOPSet};

/// @brief Set of all performance related Operator set.
static const set<set<OperatorTag>> allPerfRelOPSet = {
    computeBoundOPSet, memoryBoundOPSet, xferOPSet};

static const set<OperatorTag> allOPSet = {
    OperatorTag::AFFINE,      OperatorTag::EUDIST,    OperatorTag::CONV_1D,
    OperatorTag::LOOKUP,      OperatorTag::ELEW_PROD, OperatorTag::ELEW_ADD,
    OperatorTag::LOGIC_START, OperatorTag::LOGIC_END, OperatorTag::MAP,
    OperatorTag::REDUCE,      OperatorTag::MAC,       OperatorTag::FILTER,
    OperatorTag::UNDEFINED};

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
    {OperatorType::Undefined, "Undefined"}};

} // namespace Operator
} // namespace MetaPB
#endif

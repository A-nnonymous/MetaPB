#ifndef OP_REGISTRY
#define OP_REGISTRY

namespace MetaPB{
namespace Operator{

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

} // namespace Operator
} // namespace MetaPB
#endif
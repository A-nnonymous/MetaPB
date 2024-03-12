#include <string>

namespace MetaPB {
namespace Operator {
enum class OperatorTag {

};

typedef struct OperatorTraits {
  std::string name;
  enum class type { computeBound, memoryBound };
} OperatorTraits;

typedef struct TransferTraits {
  std::string name;
  size_t dataSize;
  enum class type { logicDependence, dataDependence };
} TransferTraits;

typedef std::string ScaleTag;
} // namespace Operator
} // namespace MetaPB
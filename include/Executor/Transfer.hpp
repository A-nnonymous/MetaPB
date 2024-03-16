#ifndef XFER_HPP
#define XFER_HPP
namespace MetaPB {
namespace Executor {

typedef struct TransferProperties {
  double dataSize_Ratio = 1.0f;
  // ----- Schedule adjust zone ------
  bool isNeedTransfer = false;
  double prevDRAMRatio = 1.0f;
  double nextDRAMRatio = 1.0f;
  // ----- Schedule adjust zone ------
} TransferProperties;

} // namespace Executor
} // namespace MetaPB
#endif
#ifndef QUANT_HPP
#define QUANT_HPP
namespace utils {

class Quantilizer {
public:
  Quantilizer() = default;
  void tickCPU() noexcept;
  void tockCPU() noexcept;
  void tickDPU() noexcept;
  void tockDPU() noexcept;
  void tickC2D() noexcept;
  void tockC2D() noexcept;
  void tickD2C() noexcept;
  void tockD2C() noexcept;

private:
  double cpuExecTime_MS = 0.0;
  double cpuExecEnergy_J = 0.0;
  double dpuExecTime_MS = 0.0;
  double dpuExecEnergy_J = 0.0;
  double cpu2dpuTime_MS = 0.0;
  double dpu2cpuTime_MS = 0.0;
  double transferEnergy_J = 0.0;
};
} // namespace utils
#endif
#include "experiments/benchmark.hpp"
using namespace benchmarks;
using namespace MetaPB;

namespace benchmarks {
class DPUReallocate : public Benchmark {
public:
  using Benchmark::Benchmark; // Inherit constructors

  void writeCSV(const std::string &dumpPath) const noexcept override { return; }

  void exec() override {
    for (size_t i = 1; i < 2048; i *= 2) {
      Operator::OperatorManager om(i);
      std::cout << "i" << std::endl;
    }
  }
};
} // namespace benchmarks
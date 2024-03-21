#include "experiments/benchmark.hpp"

using namespace benchmarks;
using namespace MetaPB;

namespace benchmarks {
class TMP : public Benchmark {
public:
  using Benchmark::Benchmark;
  void writeCSV(const std::string &dumpPath) const noexcept override {}
  void exec() override {}
};
} // namespace benchmarks
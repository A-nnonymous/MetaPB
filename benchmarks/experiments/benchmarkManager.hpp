#ifndef BENCHMARK_MGR
#define BENCHMARK_MGR
#include "experiments/benchmark.hpp"
#include "helpers/helper.hpp"
#include "workloads/workloads.hpp"

#include "experiments/b_DetailShowoff.hpp"
#include "experiments/b_HybridOP.hpp"
#include "experiments/b_graphBench.hpp"
#include "experiments/b_singleOPDeduce.hpp"

#include "experiments/dummy_template.hpp"
#include "experiments/e_reallocDPU.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

using namespace benchmarks;

namespace benchmarks {
class BenchmarkManager {
public:
  explicit BenchmarkManager(std::string dumpPath = RESULT_DUMP_DIR)
      : dumpPath(std::move(dumpPath)) {}

  template <typename T>
  void addBenchmark(const typename T::Tag &tag,
                    const typename T::Argv &argMap) {
    auto benchmark = std::make_unique<T>(tag, argMap);
    std::string resultPath = dumpPath;
    for (const auto &tagElem : tag) {
      resultPath += tagElem + "/";
    }
    std::filesystem::create_directories(resultPath);
    benchmark2Path.emplace_back(std::move(benchmark), std::move(resultPath));
  }

  void exec() {
    size_t totalBenchmarks = benchmark2Path.size();
    size_t benchmarksCompleted = 0;

    for (auto &[benchmark, path] : benchmark2Path) {
      // Start timer
      auto start = std::chrono::high_resolution_clock::now();

      // Execute benchmark
      benchmark->exec();
      benchmark->writeCSV(path);

      // Stop timer and calculate duration
      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = end - start;

      benchmarksCompleted++;

      // Update progress and print task name
      printProgress(benchmarksCompleted, totalBenchmarks, benchmark->getTag(),
                    elapsed.count());
    }
    std::cout << std::endl;
  }

private:
  void printProgress(size_t completed, size_t total,
                     const Benchmark::Tag &currentTag,
                     double elapsedSeconds) const {
    float progress = (float)completed / total;
    int barWidth = 50;

    std::cout << "\r"; // Move cursor to the beginning of the line
    std::cout << "Current Benchmark: ";
    for (const auto &t : currentTag) {
      std::cout << t << " ";
    }
    std::cout << " | " << std::fixed << std::setprecision(2) << elapsedSeconds
              << "s ";

    std::cout << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
      if (i < pos)
        std::cout << "=";
      else if (i == pos)
        std::cout << ">";
      else
        std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << "% (" << completed << "/"
              << total << ")";
    std::cout.flush();
  }

  std::string dumpPath;
  std::vector<std::pair<std::unique_ptr<Benchmark>, std::string>>
      benchmark2Path;
};

} // namespace benchmarks

#endif

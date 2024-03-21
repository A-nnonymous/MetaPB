#ifndef BENCHMARK
#define BENCHMARK
#include <string>
#include <unordered_map>
#include <vector>
namespace benchmarks {

class Benchmark {
public:
  using Tag = std::vector<std::string>;
  using Argv = std::unordered_map<std::string, std::string>;

  Benchmark(Tag tag, Argv argMap)
      : myTag(std::move(tag)), myArgs(std::move(argMap)) {}

  virtual ~Benchmark() = default;

  virtual void writeCSV(const std::string &dumpPath) const noexcept = 0;
  virtual void exec() = 0;

  const Tag &getTag() const { return myTag; }

protected:
  Argv myArgs; // Made protected to allow derived classes to access it

private:
  Tag myTag;
};
} // namespace benchmarks
#endif
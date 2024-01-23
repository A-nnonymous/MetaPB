#ifndef METASCHE_HPP
#define METASCHE_HPP
#include <string>
#include <unordered_map>

using std::string;
namespace MetaPB {
namespace MetaScheduler {

class MetaScheduler {
public:
  MetaScheduler(const string &);

private:
  const string perfModelPath;

}; // class MetaScheduler

} // namespace MetaScheduler
} // namespace MetaPB
#endif

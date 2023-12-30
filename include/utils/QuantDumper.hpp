
#ifndef QUANT_DUMPER_HPP
#define QUANT_DUMPER_HPP

#include "CSVWriter.hpp"
#include "Quant.hpp"
namespace MetaPB {
namespace utils {

class QuantDumper {
public:
  void dump(const vector<Report> &report, const vector<string> &header,
            const string &filename, const string &filepath) {
    // output file should contains all stats' mean, variance, upperBias,
    // lowerBias,sum in a single column header size == report size + 1(name),
  }
}
} // namespace utils
} // namespace MetaPB
#endif
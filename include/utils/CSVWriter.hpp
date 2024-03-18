#ifndef UTL_CSVWRITER_HPP
#define UTL_CSVWRITER_HPP
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

using std::string;
using std::vector;

namespace MetaPB {
namespace utils {

template <typename dType> class CSVWriter {
public:
  void writeCSV(const vector<vector<dType>> &data,
                const vector<string> &headers, const string &filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Failed to open file: " << filename << std::endl;
      return;
    }

    if (!headers.empty())
      writeRow(headers, file);
    file << "\n";

    for (size_t rowIdx = 0; rowIdx != data.size(); rowIdx++) {
      writeRow(data[rowIdx], file);
      if (rowIdx != data.size() - 1) [[likely]]
        file << "\n";
    }

    file.close();
  }

private:
  template <typename itemType>
  void writeRow(const vector<itemType> &row, std::ofstream &file) {
    std::stringstream rowStream;
    std::copy(row.begin(), row.end(),
              std::ostream_iterator<itemType>(rowStream, ","));
    string rowString = rowStream.str();
    rowString.pop_back(); // Remove the trailing comma
    file << rowString;
  }
};

} // namespace utils.
} // namespace MetaPB.

#endif
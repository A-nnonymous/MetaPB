#include <vector>
#include <string>
#include <utils/CSVWriter.hpp>

using std::string;
using std::vector;
using MetaPB::utils::CSVWriter;

static vector<string> stratigies {"NOT_OFFLOADED", "ALL_OFFLOADED", "HEFT", 
        "MetaPB_Time_Sensitive","MetaPB_Hybrid" , "MetaPB_Energy_Sensitive"};
static vector<string> loads{"H-20", "H-50", "H-80", "M-20", "M-50", "M-80"};
static void testDataGen(vector<vector<string>> &data)noexcept{
  vector<double> s_tAffinity{0.6, 0.7, 0.8, 1.0, 0.9, 0.83};
  vector<double> l_tAffinity{1.0, 0.8, 0.6, 0.9, 0.8, 0.7 };
  vector<double> s_eAffinity{0.5, 0.6, 0.4, 0.8, 0.9, 1.0 };
  vector<double> l_eAffinity{1.0, 0.9, 0.8, 0.9, 0.8, 0.7 };
}
int main(int argc, char **argv) {
  CSVWriter<string> writer;
  vector<vector<string>> data(loads.size(), vector<string>(stratigies.size(), "0"));
  testDataGen(data); 
  writer.writeCSV(data, )
  return 0;
}

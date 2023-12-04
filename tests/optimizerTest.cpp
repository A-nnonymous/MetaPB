#include "utils/CSVWriter.hpp"
#include "Optimizer/OptimizerNaive.hpp"
#include <vector>

/*
vector<double> evalFunc(const vector<vector<double>> &pts){
  vector<double> ret(pts.size(), 0);
  return ret;
}
*/
vector<int> optima {100, 41};
vector<double> evalFunc(const vector<vector<int>> &pts) {
  vector<double> ret(pts.size(), 0);
  for(size_t ptIdx = 0; ptIdx != pts.size(); ptIdx++){
    double value = 0.0;
    for(size_t dimIdx = 0; dimIdx != 2; dimIdx++){
      value += -(abs(optima[dimIdx] - pts[ptIdx][dimIdx]));
    }
    ret[ptIdx] = value;
  }
  return ret;
}

int main() {
  // function<vector<double>(const vector<vector<double>>&)> ef(evalFunc);
  function<vector<double>(const vector<vector<int>> &)> ef(evalFunc);
  // Optimizer::OptimizerNaive<double, double> opt({0.0f, 0.0f, 0.0f},{-10.0f,
  // -10.0f, -10.0f}, {10.0f, 10.0f, 10.0f}, 3,4,10,ef);
  size_t pointNum = 6;
  size_t iterNum = 25;
  size_t dimNum = 2;
  vector<int> lowerLimit {1,1};
  vector<int> upperLimit {100, 50};
  Optimizer::OptimizerNaive<int, double> opt(optima, lowerLimit,upperLimit,dimNum, pointNum,iterNum,ef);
  opt.exec();
  auto cumuHist = opt.getCumulatePointsHistory();
  auto valHist = opt.getValueHistory();

  utils::CSVWriter<int> argCSV;
  vector<string> argHeader = {"Voltage_mv", "Timestep_ms"};
  utils::CSVWriter<double> valCSV;
  vector<string> valHeader;
  for(size_t i = 0; i < pointNum; i++){
    string str = "Point" + std::to_string(i);
    valHeader.emplace_back(str);
  }
  argCSV.writeCSV(cumuHist, argHeader, "./cumuHist.csv");
  valCSV.writeCSV(valHist, valHeader, "./valHist.csv");
  
  return 0;
}
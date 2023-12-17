#include <vector>

#include "Optimizer/OptimizerPSO.hpp"
#include "utils/CSVWriter.hpp"

/*
vector<double> evalFunc(const vector<vector<double>> &pts){
  vector<double> ret(pts.size(), 0);
  return ret;
}
*/
vector<int> optima{100, 41};
vector<double> evalFunc(const vector<vector<int>> &pts) {
  vector<double> ret(pts.size(), 0);
  for (size_t ptIdx = 0; ptIdx != pts.size(); ptIdx++) {
    double value = 0.0;
    for (size_t dimIdx = 0; dimIdx != 2; dimIdx++) {
      value += -(abs(optima[dimIdx] - pts[ptIdx][dimIdx]));
    }
    ret[ptIdx] = value;
  }
  return ret;
}

vector<double> concave(const vector<vector<double>> &pts){
  vector<double> results;
  for(size_t ptIdx = 0; ptIdx != pts.size(); ptIdx++){
    double val = 0.0;
    for(const auto & dim: pts[ptIdx]){
      val += (dim * dim);
    }
    results.emplace_back(val);
  }
  return results;
}
int main() {
  // function<vector<double>(const vector<vector<double>>&)> ef(evalFunc);
  //function<vector<double>(const vector<vector<int>> &)> ef(evalFunc);
  function<vector<double>(const vector<vector<double>> &)> co(concave);
  // Optimizer::OptimizerNaive<double, double> opt({0.0f, 0.0f, 0.0f},{-10.0f,
  // -10.0f, -10.0f}, {10.0f, 10.0f, 10.0f}, 3,4,10,ef);
  size_t pointNum = 100;
  size_t iterNum = 1000;
  size_t dimNum = 10;
  double dt = 0.001;
  double ego = 0.2;
  double omega = 0.7;
  ;
  double vMax = 2 * (200.0/(iterNum * dt));
  auto lowerLimit = vector<double>(pointNum, -100);
  auto upperLimit = vector<double>(pointNum, 100);
  Optimizer::OptimizerPSO<double, double> opt(vMax,omega,dt,ego, lowerLimit, upperLimit,
                                             dimNum, pointNum, iterNum, co);
  opt.exec();
  std::cout<< "Optima is expected to be 0, ended with: "<< opt.getGlobalOptimaValue()<<std::endl;
  std::cout<< "Optima vec is :";
  auto pt = opt.getGlobalOptimaPoint();
  for(const auto& dim : pt){
    std::cout << dim << ", ";
  }
  std::cout <<std::endl;
  auto cumuHist = opt.getCumulatePointsHistory();
  auto valHist = opt.getValueHistory();
  auto optHist = opt.getGlobalConvergeValueVec();

  utils::CSVWriter<double> argCSV;
  vector<string> argHeader = {"Voltage_mv", "Timestep_ms"};
  utils::CSVWriter<double> valCSV;
  vector<string> valHeader;
  for (size_t i = 0; i < pointNum; i++) {
    string str = "Point" + std::to_string(i);
    valHeader.emplace_back(str);
  }
  argCSV.writeCSV(cumuHist, argHeader, "./cumuHist.csv");
  valCSV.writeCSV(valHist, valHeader, "./valHist.csv");
  valCSV.writeCSV({optHist}, {}, "./opt.csv");

  return 0;
}
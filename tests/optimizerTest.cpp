#include <string>
#include <vector>
#include "Optimizer/OptimizerBase.hpp"
#include "Optimizer/OptimizerPSO.hpp"
#include "Optimizer/OptimizerAOA.hpp"
#include "Optimizer/OptimizerRSA.hpp"
#include "utils/CSVWriter.hpp"

using Optimizer::OptimizerBase;
vector<double> concave(const vector<vector<double>> &pts) {
  vector<double> results;
  for (size_t ptIdx = 0; ptIdx != pts.size(); ptIdx++) {
    double val = 0.0;
    for (const auto &dim : pts[ptIdx]) {
      val += (dim * dim);
    }
    results.emplace_back(val);
  }
  return results;
}
vector<int> concaveI(const vector<vector<int>> &pts) {
  vector<int> results;
  for (size_t ptIdx = 0; ptIdx != pts.size(); ptIdx++) {
    int val = 0.0;
    for (const auto &dim : pts[ptIdx]) {
      val += (dim * dim);
    }
    results.emplace_back(val);
  }
  return results;
}


template<typename aType, typename vType>
void testConcave(OptimizerBase<aType,vType> &opt, const vector<string> &argHeader,
                 const vector<string> &valHeader,
                 const string optimizerName){
  opt.exec();
  std::cout << "Optima is expected to be 0, ended with: "
            << opt.getGlobalOptimaValue() << std::endl;
  std::cout << "Optima vec is :";
  auto pt = opt.getGlobalOptimaPoint();
  for (const auto &dim : pt) {
    std::cout << dim << ", ";
  }
  std::cout << std::endl;

  auto cumuHist = opt.getCumulatePointsHistory();
  auto valHist = opt.getValueHistory();
  auto convergeHist = opt.getGlobalConvergeValueVec();

  utils::CSVWriter<aType> argCSV;
  utils::CSVWriter<vType> valCSV;
  argCSV.writeCSV(cumuHist, argHeader, "./cumuHist_"+ optimizerName + ".csv");
  valCSV.writeCSV(valHist, valHeader, "./valHist_"+ optimizerName +".csv");
  valCSV.writeCSV({convergeHist}, {}, "./convergeHist_"+ optimizerName +".csv");
}

int main() {
  size_t pointNum = 20;
  size_t iterNum = 1000;
  size_t dimNum = 10;
  function<vector<double>(const vector<vector<double>> &)> cof(concave);
  function<vector<int>(const vector<vector<int>> &)> coi(concaveI);

  vector<string> argHeader;
  for (size_t i = 0; i < dimNum; i++) {
    string str = "Arg" + std::to_string(i);
    argHeader.emplace_back(str);
  }
  vector<string> valHeader;
  for (size_t i = 0; i < pointNum; i++) {
    string str = "Point" + std::to_string(i);
    valHeader.emplace_back(str);
  }

  double dt = 0.1;
  double ego = 0.2;
  double omega = 0.7;
  double vMax = 2 * (200.0 / (iterNum * dt));

  auto lowerLimitI = vector<int>(pointNum, -100);
  auto upperLimitI = vector<int>(pointNum, 100);
  Optimizer::OptimizerPSO<int, int> optPSO(vMax, omega, dt, ego, lowerLimitI,
                                        upperLimitI, dimNum, pointNum, iterNum,
                                        coi);
  testConcave<int, int>(optPSO, argHeader, valHeader, "PSOI");
  auto lowerLimitF = vector<double>(pointNum, -100.0f);
  auto upperLimitF = vector<double>(pointNum, 100.0f);
  Optimizer::OptimizerAOA<double, double> optAOA(lowerLimitF, upperLimitF,
                                               dimNum, pointNum, iterNum,
                                        cof);
  Optimizer::OptimizerRSA<double, double> optRSA(lowerLimitF, upperLimitF,
                                               dimNum, pointNum, iterNum,
                                        cof);
  testConcave<double, double>(optAOA, argHeader, valHeader, "AOAF");
  testConcave<double, double>(optRSA, argHeader, valHeader, "RSAF");

  return 0;
}

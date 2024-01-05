#include "Optimizer/OptimizerAOA.hpp"
#include "Optimizer/OptimizerBase.hpp"
#include "Optimizer/OptimizerPSO.hpp"
#include "Optimizer/OptimizerRSA.hpp"
#include "Optimizer/OptimizerNaive.hpp"
#include "utils/CSVWriter.hpp"
#include <string>
#include <vector>

using namespace MetaPB;
using Optimizer::OptimizerBase;
vector<double> concave(const vector<vector<double>> &pts) {
  vector<double> results;
  for (size_t ptIdx = 0; ptIdx != pts.size(); ptIdx++) {
    double val = 0.0;
    for (const auto &dim : pts[ptIdx]) {
      val += (dim - 100.0f)*(dim - 100.0f);
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
      val += (dim) * (dim);
    }
    results.emplace_back(val);
  }
  return results;
}

template <typename aType, typename vType>
void testConcave(OptimizerBase<aType, vType> &opt,
                 const vector<string> &argHeader,
                 const vector<string> &valHeader, const string optimizerName) {
  opt.exec();
  std::cout << optimizerName
            << " has optimized costs to: " << opt.getGlobalOptimaValue()
            << ".\n With argVec: ";
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
  argCSV.writeCSV(cumuHist, argHeader, "./cumuHist_" + optimizerName + ".csv");
  valCSV.writeCSV(valHist, valHeader, "./valHist_" + optimizerName + ".csv");
  valCSV.writeCSV({convergeHist}, {},
                  "./convergeHist_" + optimizerName + ".csv");
}

vector<double> evalFunc(const vector<vector<int>> &points){
  vector<double> results;
  const vector<int> optima = {400,52}; // 1000mv 61ms
  for(const auto & point: points){
    double result = 0.0f;
    for(size_t i = 0; i < point.size(); i++){
      result += (optima[i] - point[i])*(optima[i] - point[i]);
    }
    results.emplace_back(result);
  }
  return results;
}
int main() {
  size_t pointNum = 50;
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

  auto lowerLimitI = vector<int>(pointNum, -200);
  auto upperLimitI = vector<int>(pointNum, 200);
  auto lowerLimitF = vector<double>(pointNum, -200.0f);
  auto upperLimitF = vector<double>(pointNum, 200.0f);

  // PSO related extra arguments
  double dt = 0.1;
  double ego = 0.2;
  double omega = 0.7;
  double vMax = 2 * (200.0 / (iterNum * dt));
  Optimizer::OptimizerPSO<int, int> optPSOI(vMax, omega, dt, ego, lowerLimitI,
                                            upperLimitI, dimNum, pointNum,
                                            iterNum, coi);
  Optimizer::OptimizerPSO<double, double> optPSOF(
      vMax, omega, dt, ego, lowerLimitF, upperLimitF, dimNum, pointNum, iterNum,
      cof);

  Optimizer::OptimizerAOA<int, int> optAOAI(lowerLimitI, upperLimitI, dimNum,
                                            pointNum, iterNum, coi);
  Optimizer::OptimizerAOA<double, double> optAOAF(
      lowerLimitF, upperLimitF, dimNum, pointNum, iterNum, cof);

  Optimizer::OptimizerRSA<int, int> optRSAI(lowerLimitI, upperLimitI, dimNum,
                                            pointNum, iterNum, coi);
  Optimizer::OptimizerRSA<double, double> optRSAF(
      lowerLimitF, upperLimitF, dimNum, pointNum, iterNum, cof);

  testConcave<int, int>(optPSOI, argHeader, valHeader, "PSOI");
  testConcave<int, int>(optAOAI, argHeader, valHeader, "AOAI");
  testConcave<int, int>(optRSAI, argHeader, valHeader, "RSAI");
  testConcave<double, double>(optPSOF, argHeader, valHeader, "PSOF");
  testConcave<double, double>(optAOAF, argHeader, valHeader, "AOAF");
  testConcave<double, double>(optRSAF, argHeader, valHeader, "RSAF");
  /*
  const vector<int> lb{200,0};
  const vector<int> ub{1000,70};
  const size_t dimNum = 2;
  const size_t ptNum = 10;
  const size_t iterNum = 100;
  function<vector<int>(const vector<vector<int>> &)> coi(concaveI);
  Optimizer::OptimizerAOA<int,int> opt(lb, ub, dimNum,ptNum,iterNum,coi);
  opt.exec();
  auto cumuHist = opt.getCumulatePointsHistory();
  auto valHist= opt.getValueHistory();
  auto convergeHist = opt.getGlobalConvergeValueVec();
  vector<string> argHeader;
  for (size_t i = 0; i < dimNum; i++) {
    string str = "Arg" + std::to_string(i);
    argHeader.emplace_back(str);
  }
  vector<string> valHeader;
  for (size_t i = 0; i < ptNum; i++) {
    string str = "Point" + std::to_string(i);
    valHeader.emplace_back(str);
  }

  utils::CSVWriter<int> argCSV;
  utils::CSVWriter<int> valCSV;
  argCSV.writeCSV(cumuHist, argHeader, "./cumuHist.csv");
  valCSV.writeCSV(valHist, valHeader, "./valHist.csv");
  valCSV.writeCSV({convergeHist}, {},
                  "./convergeHist.csv");
*/
  return 0;
}

#include "Optimizer/OptimizerNaive.hpp"
#include <vector>

vector<double> evalFunc(const vector<vector<double>> &pts){
  vector<double> ret(pts.size(), 0);
  return ret;
}

int main(){
  function<vector<double>(const vector<vector<double>>&)> ef(evalFunc);
  Optimizer::OptimizerNaive<double, double> opt({-10.0f, -10.0f, -10.0f}, {10.0f, 10.0f, 10.0f}, 3,4,5,ef);
  opt.exec();
  return 0;
}
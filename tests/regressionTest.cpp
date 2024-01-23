#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <xgboost/c_api.h>

#define safe_xgboost(call)                                                     \
  {                                                                            \
    int err = (call);                                                          \
    if (err != 0) {                                                            \
      std::cerr << __FILE__ << ":" << __LINE__ << " error in " << #call << ":" \
                << XGBGetLastError();                                          \
      exit(1);                                                                 \
    }                                                                          \
  }

template <typename T>
std::vector<float>
flattenVectors(const std::vector<std::vector<T>> &twoDVector) {
  std::vector<float> result;

  for (const auto &row : twoDVector) {
    result.insert(result.end(), row.begin(), row.end());
  }

  return result;
}
int main() {
  // 1. Initialize data
  std::vector<float> error{1.0};
  std::vector<float> data = {1.0, 2.0, 7.0, 8.0}; // Representing feature values
  std::vector<float> labels = {5.0, 113.0};       // Representing labels
  // 2. Initialize data
  std::vector<float> dataT = {5.0, 6.0, 3.0,
                              4.0};          // Representing feature values
  std::vector<float> labelsT = {61.0, 25.0}; // Representing labels

  // 2. Initialize DMatrix
  DMatrixHandle dtrain;
  safe_xgboost(
      XGDMatrixCreateFromMat(data.data(), 2, data.size() / 2, std::nan(""),
                             &dtrain)); // Use std::nan("") instead of NAN
  safe_xgboost(
      XGDMatrixSetFloatInfo(dtrain, "label", labels.data(), labels.size()));

  // 3. Set parameters
  std::map<const std::string, const std::string> paramMap = {
      {"objective", "reg:squarederror"},
      {"eval_metric", "rmse"},
      {"max_depth", "5"}};
  BoosterHandle booster;
  safe_xgboost(XGBoosterCreate(&dtrain, 1, &booster));
  for (const auto &[k, v] : paramMap) {
    safe_xgboost(XGBoosterSetParam(booster, k.c_str(), v.c_str()));
  }

  // 4. Train the model
  int num_round = 100; // Number of iterations
  for (int iter = 0; iter < num_round; ++iter) {
    safe_xgboost(XGBoosterUpdateOneIter(booster, iter, dtrain));
  }

  // 5. Perform inference
  DMatrixHandle dtest;
  safe_xgboost(XGDMatrixCreateFromMat(dataT.data(), 2, dataT.size() / 2,
                                      std::nan(""), &dtest));

  bst_ulong out_len;
  const float *predictions;
  safe_xgboost(XGBoosterPredict(
      booster, dtest, 0, 0, 0, &out_len,
      &predictions)); // Modify parameters to match the function signature

  // 6. Print the inference results
  for (bst_ulong i = 0; i < out_len; ++i) {
    std::cout << "Prediction[" << i << "]: " << predictions[i] << std::endl;
  }

  // 7. Release resources
  safe_xgboost(XGDMatrixFree(dtrain));
  safe_xgboost(XGDMatrixFree(dtest));
  safe_xgboost(XGBoosterFree(booster));

  return 0;
}

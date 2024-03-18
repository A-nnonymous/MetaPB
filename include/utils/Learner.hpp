#ifndef REGRESSOR
#define REGRESSOR
#include <iostream>
#include <string>
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
namespace MetaPB {
namespace utils {
class Learner {
private:
  BoosterHandle h_booster;
  int cols;
  int x_norm;
  int y_norm;

public:
  Learner(int cols = 1) : cols(cols), h_booster(nullptr) {}

  void train(float *train_data, float *train_labels, int rows, int iterMax) {
    /*
    float iMax = -10000.0f; float iMin = 10000.0f;
    float lMax = -10000.0f; float lMin = 10000.0f;
    for(int i = 0; i < rows; i++){
      if(train_data[i] > iMax) iMax = train_data[i];
      if(train_labels[i] > lMax) lMax = train_labels[i];
      if(train_labels[i] < lMin) lMin = train_labels[i];
      if(train_data[i] < iMin) iMin = train_data[i];
    }
    x_norm = iMax - iMin;
    y_norm = lMax - lMin;
    std::cout << "x_norm: "<<x_norm<<std::endl;
    std::cout << "y_norm: "<<y_norm<<std::endl;
    for(int i = 0; i < rows; i++){
      train_data[i] /= x_norm;
      train_labels[i] /= y_norm;
    }
    */
    // convert to DMatrix
    DMatrixHandle h_train;
    safe_xgboost(XGDMatrixCreateFromMat(train_data, rows, cols, -1, &h_train));

    // load the labels
    safe_xgboost(XGDMatrixSetFloatInfo(h_train, "label", train_labels, rows));

    // create the booster and load some parameters
    safe_xgboost(XGBoosterCreate(&h_train, 1, &h_booster));
    safe_xgboost(XGBoosterSetParam(h_booster, "booster", "dart"));
    safe_xgboost(XGBoosterSetParam(h_booster, "objective", "reg:squarederror"));
    safe_xgboost(XGBoosterSetParam(h_booster, "max_depth", "5"));
    safe_xgboost(XGBoosterSetParam(h_booster, "eta", "0.1"));
    safe_xgboost(XGBoosterSetParam(h_booster, "lambda", "1"));
    safe_xgboost(XGBoosterSetParam(h_booster, "min_child_weight", "1"));
    safe_xgboost(XGBoosterSetParam(h_booster, "subsample", "0.5"));
    safe_xgboost(XGBoosterSetParam(h_booster, "colsample_bytree", "1"));
    safe_xgboost(XGBoosterSetParam(h_booster, "num_parallel_tree", "1"));

    // perform 200 learning iterations
    for (int iter = 0; iter < iterMax; iter++)
      safe_xgboost(XGBoosterUpdateOneIter(h_booster, iter, h_train));

    // free xgboost internal structures for the training matrix
    safe_xgboost(XGDMatrixFree(h_train));
  }

  void predict(float *test_data, int sample_rows, float *predictions) {
    // convert to DMatrix
    DMatrixHandle h_test;
    safe_xgboost(
        XGDMatrixCreateFromMat(test_data, sample_rows, cols, -1, &h_test));
    bst_ulong out_len;
    const float *f;
    safe_xgboost(XGBoosterPredict(h_booster, h_test, 0, 0, 0, &out_len, &f));

    // copy predictions to the provided array
    for (unsigned int i = 0; i < out_len; i++)
      predictions[i] = f[i];

    // free xgboost internal structures for the test matrix
    safe_xgboost(XGDMatrixFree(h_test));
  }

  inline void saveModel(const std::string &filePath) const noexcept {
    safe_xgboost(XGBoosterSaveModel(h_booster, filePath.c_str()));
  }

  inline void loadModel(const std::string &filePath) noexcept {
    XGBoosterCreate(nullptr, 0, &h_booster);
    safe_xgboost(XGBoosterLoadModel(h_booster, filePath.c_str()));
  }

  ~Learner() {
    // free xgboost internal structures
    if (h_booster) {
      safe_xgboost(XGBoosterFree(h_booster));
    }
  }
};

} // namespace utils
} // namespace MetaPB
#endif
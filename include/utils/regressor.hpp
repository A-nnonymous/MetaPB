#ifndef REGRESSOR
#define REGRESSOR
#include <xgboost/c_api.h>
namespace MetaPB{
namespace utils{
class Learner {
private:
    BoosterHandle h_booster;
    int cols;
public:
    Learner(int cols) : cols(cols), h_booster(nullptr) {}

    void train(float* train_data, float* train_labels, int rows) {
        // convert to DMatrix
        DMatrixHandle h_train;
        XGDMatrixCreateFromMat(train_data, rows, cols, -1, &h_train);

        // load the labels
        XGDMatrixSetFloatInfo(h_train, "label", train_labels, rows);

        // create the booster and load some parameters
        XGBoosterCreate(&h_train, 1, &h_booster);
        XGBoosterSetParam(h_booster, "booster", "dart");
        XGBoosterSetParam(h_booster, "objective", "reg:squarederror");
        XGBoosterSetParam(h_booster, "max_depth", "5");
        XGBoosterSetParam(h_booster, "eta", "0.1");
        XGBoosterSetParam(h_booster, "min_child_weight", "1");
        XGBoosterSetParam(h_booster, "subsample", "0.5");
        XGBoosterSetParam(h_booster, "colsample_bytree", "1");
        XGBoosterSetParam(h_booster, "num_parallel_tree", "1");

        // perform 200 learning iterations
        for (int iter = 0; iter < 300; iter++)
            XGBoosterUpdateOneIter(h_booster, iter, h_train);

        // free xgboost internal structures for the training matrix
        XGDMatrixFree(h_train);
    }

    void predict(float* test_data, int sample_rows, float* predictions) {
        // convert to DMatrix
        DMatrixHandle h_test;
        XGDMatrixCreateFromMat(test_data, sample_rows, cols, -1, &h_test);
        bst_ulong out_len;
        const float *f;
        XGBoosterPredict(h_booster, h_test, 0, 0, 0, &out_len, &f);

        // copy predictions to the provided array
        for (unsigned int i = 0; i < out_len; i++)
            predictions[i] = f[i];

        // free xgboost internal structures for the test matrix
        XGDMatrixFree(h_test);
    }

    ~Learner() {
        // free xgboost internal structures
        if (h_booster) {
            XGBoosterFree(h_booster);
        }
    }
};

} // namespace utils
} // namespace MetaPB
#endif
#include "utils/Learner.hpp"
#include <random>
using namespace std;
using namespace MetaPB::utils;

int main() {
  mt19937_64 rng(random_device{}());
  uniform_real_distribution<float> dist(0,99);
  Learner l;
  constexpr int trainSize = 200;
  constexpr int testSize = 10;
  float trainData[trainSize];
  float trainLabel[trainSize];
  float testSet[testSize];
  for(int i =0 ; i < trainSize; i++){
    trainData[i] = i;
    trainLabel[i] = i;
  }
  for(int i = 0 ; i < testSize; i++){
    testSet[i] = dist(rng);
  }

  l.train(trainData, trainLabel, trainSize);
  float result[testSize];
  l.predict(testSet, testSize, result);
  for(int i = 0; i < testSize; i++){
    cout << testSet[i] << " -- "<< result[i]<<endl;
  }
  return 0;
  
}
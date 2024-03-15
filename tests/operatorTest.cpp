#include "Operator/OperatorBase.hpp"
#include <vector>
#include <iostream>

using namespace MetaPB::Operator;

class Operator : public  OperatorBase {
public:
  Operator(){
    a.resize(1<<30,1);
    b.resize(1<<30,1);
  } 

  std::vector<int> a;
  std::vector<int> b;
  virtual const std::string get_name(){
    return OpName;
  }

  virtual void execCPU(const size_t batchSize_MiB)override{
    std::cout <<"exec with batch:"<< batchSize_MiB <<std::endl;
    int item = batchSize_MiB * (1<<20) / sizeof(int);
    for(int i = 0; i < item; i++){
      a[i] += b[i];
    }
  }
  virtual void execDPU(const size_t batchSize_MiB)override{
    return;
  }

private:
  inline static const std::string OpName = "abb";
};

int main(){
  Operator a;
  a.trainModel(256);
  std::cout << "deduced perf is : "<<a.deducePerf(0,200).timeCost_Second<<std::endl;


  return 0;
}
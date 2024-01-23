#include "Operators/OperatorBase.hpp"

using namespace MetaPB::Operator;


class OperatorFoo: public OperatorBase{
public:
  virtual void exec(const ArgMap& args)override{
    std::cout<< "Hello operatorFOO" <<std::endl;
    execCPU(args);
    execDPU(args);
  }
private:
  virtual void execCPU(const ArgMap&)override{
    std::cout<< "Hello operatorFOO_CPU" <<std::endl;
  }
  virtual void execDPU(const ArgMap&)override{
    std::cout<< "Hello operatorFOO_DPU" <<std::endl;
  }
};

int main(){
  OperatorFoo a;
  OperatorFoo::ArgMap am;
  a.exec(am);
}
#include "Operator/OperatorManager.hpp"
#include "Operator/OperatorRegistry.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

using namespace MetaPB::Operator;
using MetaPB::Operator::OperatorTag;


int main() {
  OperatorManager om;
  om.instantiateAll();
  CPU_TCB cpuTCB;
  DPU_TCB dpuTCB;
  return 0;
}

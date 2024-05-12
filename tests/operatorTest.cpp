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
  auto testOP = OperatorTag::LOOKUP;
  bool isO1 = true;
  const size_t pageBlkSize = om.getPageBlkSize();
  void* src1 = malloc(pageBlkSize);
  void* src2 = malloc(pageBlkSize);
  void* cpuDst = malloc(pageBlkSize);
  void* dpuDst = malloc(pageBlkSize);

  uint32_t itemNum = pageBlkSize / sizeof(int);

  int* src1Int = (int*) src1;
  int* src2Int = (int*) src2;
  int* cpuDstInt = (int*) cpuDst;
  int* dpuDstInt = (int*) dpuDst;

  for(int i = 0; i < itemNum; i++){
    src1Int[i] = 2;
    src2Int[i] = 3;
  }

  CPU_TCB map1TCB; map1TCB.sgInfo = {src1, 0, 1};
  CPU_TCB map2TCB; map2TCB.sgInfo = {src2, 1, 1};
  MetaPB::Operator::DPU_TCB dpuTCB{0,1,2,1};
  CPU_TCB reduceTCB; reduceTCB.sgInfo = {dpuDst, 2, 1};
  CPU_TCB computeTCB{src1, src2, cpuDst, 1, {}};

  // DPU compute
  om.execCPU(OperatorTag::MAP, map1TCB);
  om.execCPU(OperatorTag::MAP, map2TCB);
  om.execDPU(testOP, dpuTCB);
  om.execCPU(OperatorTag::REDUCE, reduceTCB);

  // CPU compute
  om.execCPU(testOP, computeTCB);

  bool flag = true;
  int result = 114514;
  if(!isO1){
    for(int i = 0; i < itemNum; i++){
      if(cpuDstInt[i] != dpuDstInt[i]){
        std::cout << "In [" << i << "]" << ", CPU: " << cpuDstInt[i] << ", DPU: "<<
          dpuDstInt[i] <<std::endl;
        flag = false;
        break;
      }
      result = dpuDstInt[0];
    }
  }else{
    flag = (cpuDstInt[0] == dpuDstInt[0]);
    result = dpuDstInt[0];
  }

  std::cout << "Result is "<< result << (flag? ", correct" : ", wrong") <<std::endl;

  return 0;
}

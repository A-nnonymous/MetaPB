#include "enviroment.hpp"

using namespace benchmarks;

void* memPools{
  malloc(4 * size_t(1<<30)),
  malloc(4 * size_t(1<<30)),
  malloc(4 * size_t(1<<30)),
};

static OperatorManager om;
static HeteroComputePool hcp(10,om,memPools);

TaskGraph genSingleOp_wo_xfer(OperatorTag opTag, size_t batchSize_MiB){

}

TaskGraph genSingleOp_w_map(OperatorTag opTag, size_t batchSize_MiB){

}

TaskGraph genSingleOp_w_mapreduce(OperatorTag opTag, size_t batchSize_MiB){

}

TaskGraph genVAMAC_string(size_t length, size_t batchSize_MiB){

}

void testAllSingleOp_wo_xfer(size_t batchSize_MiB, const std::string& dumpPath){
  for(const auto& op: allPerfRelOPSet){
    const auto tg = genSingleOp_wo_xfer(op, batchSize_MiB);
    std::map<float, perfStats> result;
    for(int i = 0; i <= 10; i++){
      perfStats p = hcp.execWorkload(tg,{false, {0}, {float(i) / 10}}, execType::DO);
    }
    dumpAllRatioStats(result);
  }
}

void testAllSingleOp_w_map(size_t batchSize_MiB, const std::string& dumpPath){
  for(const auto& op: allPerfRelOPSet){
    const auto tg = genSingleOp_w_map(op, batchSize_MiB);
    std::map<float, perfStats> result;
    for(int i = 0; i <= 10; i++){
      perfStats p = hcp.execWorkload(tg,{false, {0,1}, {0.0f , float(i) / 10}}, execType::DO);
    }
    dumpAllRatioStats(result);
  }
}
void testAllSingleOp_w_mapreduce(size_t batchSize_MiB, const std::string& dumpPath){
  for(const auto& op: allPerfRelOPSet){
    const auto tg = genSingleOp_w_mapreduce(op, batchSize_MiB);
    std::map<float, perfStats> result;
    for(int i = 0; i <= 10; i++){
      perfStats p = hcp.execWorkload(tg,{false, {0,1,2}, {0.0f, float(i) / 10, 0.0f}}, execType::DO);
    }
    dumpAllRatioStats(result, dumpPath + "");
  }
}
void testSchedules_VAMAC(size_t batchSize_MiB， const std::string &dumpPath){
  const auto tg = genVAMAC_string(10, batchSize_MiB);
  // test all Scheduler's result and performace, dump the actual schedule
}
int main(){
  om.trainAll()
}
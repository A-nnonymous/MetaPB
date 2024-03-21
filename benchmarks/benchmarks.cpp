#include "environment.hpp"
#include "helper.hpp"
#include "workloads.hpp"
#include "defs.hpp"

using execType = MetaPB::Executor::execType;
using TaskGraph = MetaPB::Executor::TaskGraph;
using TaskNode = MetaPB::Executor::TaskNode;
using Graph = MetaPB::Executor::Graph;
using TaskProperties = MetaPB::Executor::TaskProperties;
using TransferProperties = MetaPB::Executor::TransferProperties;
using OperatorType = MetaPB::Operator::OperatorType;
using OperatorTag = MetaPB::Operator::OperatorTag;
using OperatorManager = MetaPB::Operator::OperatorManager;
using MetaPB::Executor::HeteroComputePool;
using MetaPB::Operator::tag2Name;
using MetaPB::utils::regressionTask;
using MetaPB::utils::Schedule;
using perfStats = MetaPB::utils::perfStats;
using MetaPB::Operator::allPerfRelOPSet;

void* memPools[3]{
  malloc(24 * size_t(1<<30)),
  malloc(24 * size_t(1<<30)),
  malloc(24 * size_t(1<<30)),
};

static OperatorManager om;
static HeteroComputePool hcp(50,om,memPools);


/*

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
void testSchedules_VAMAC(size_t batchSize_MiB, const std::string &dumpPath){
  const auto tg = genVAMAC_string(10, batchSize_MiB);
  // test all Scheduler's result and performace, dump the actual schedule
}
*/

namespace benchmarks{
struct benchmarks{
  
};
}
int main(){
  return 0;
}
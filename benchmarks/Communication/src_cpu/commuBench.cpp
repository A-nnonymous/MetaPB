#include <omp.h>
#include "utils/ChronoTrigger.hpp"
#include "utils/DPUInterface.hpp"
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <utility>

using namespace MetaPB::utils::DPUInterface;
using MetaPB::utils::ChronoTrigger;
using std::string;
using std::thread;
using std::vector;
typedef vector<uint32_t> dpuData;
typedef vector<dpuData> rankData;

// TODO: 1. kernel VA.VM inputsize_perf/energy(64core)
// 2.
inline const string getDPUBinaryPath() {
  std::filesystem::path executablePath =
      std::filesystem::read_symlink("/proc/self/exe");
  const string parentPath = executablePath.parent_path().parent_path();
  const string dpuPath = parentPath + "/dpu_bin/commuDPU";
  return dpuPath;
}

void DPUWisePull(DpuSet *Rank,const size_t idx, vector<vector<uint32_t>> & result, size_t bufferSize) {
  vector<vector<uint32_t>> raw(1,vector<uint32_t>(bufferSize,0));
  auto& dpu = Rank->dpus()[idx];
  dpu->async().copy(raw, DPU_MRAM_HEAP_POINTER_NAME, 0);
  std::cout<<"idx"<<idx <<" finished async pull\n";
  result[idx] = raw[0];
}

void DPUWisePush(const vector<vector<uint32_t>> &data, DpuSet *Rank,const size_t idx) {
  auto& dpu = Rank->dpus()[idx];
  dpu->async().copy(DPU_MRAM_HEAP_POINTER_NAME, 0, data[idx]);
  std::cout<<"idx"<<idx <<" finished async push\n";
}

void RankWisePull(DpuSet *Rank, vector<vector<uint32_t>> & result) {
  Rank->async().copy(result, DPU_MRAM_HEAP_POINTER_NAME, 0);
}

void RankWisePush(const vector<vector<uint32_t>> &data, DpuSet *Rank) {
  Rank->async().copy(DPU_MRAM_HEAP_POINTER_NAME, 0, data);
}

void threadRankPush(DpuSet *wholeSet, const vector<rankData> & src, size_t idx) {
  wholeSet->ranks()[idx]->async().copy(DPU_MRAM_HEAP_POINTER_NAME, 0, src[idx]);
}

void threadRankPull(vector<rankData> &result, DpuSet *wholeSet, size_t idx) {
  wholeSet->ranks()[idx]->async().copy(result[idx], DPU_MRAM_HEAP_POINTER_NAME, 0);
}

void summonPush(DpuSet *wholeSet, const vector<rankData> & src, size_t usedRank) {
  vector<thread> tv;
  for(auto i = 0; i < usedRank;i++){
    tv.emplace_back(threadRankPush, wholeSet, std::ref(src), i);
  }
  for(auto &t : tv){
    t.join();
  }
}

void summonPull(vector<rankData> &result, DpuSet *wholeSet, size_t usedRank) {
  vector<thread> tv;
  auto rankNum = wholeSet->ranks().size();
  for(auto i = 0; i < usedRank;i++){
    tv.emplace_back(threadRankPull, std::ref(result), wholeSet, i);
  }
  for(auto &t : tv){
    t.join();
  }
}

int main(int argc, char **argv) {
  ChronoTrigger ct;
  const auto maxThreadCount = thread::hardware_concurrency();
  constexpr size_t mram_occupy_Byte = ((size_t)32 << 20); // 32MiB
  constexpr size_t elementPerDPU = mram_occupy_Byte / sizeof(std::uint32_t);
  auto wholeSys = DpuSet::allocateRanks();
  const auto rankDPUsVec = wholeSys.getRankWiseDPUNums();
  vector<DpuSet*> rankPtrVec = wholeSys.ranks();
  const auto dpuNum = wholeSys.getDPUNums();
  const auto rankNum = rankPtrVec.size();
  const size_t wholeSize_Byte = dpuNum * mram_occupy_Byte;
  const double wholeSize_GiB = ((double)wholeSize_Byte / (1 << 30));

  std::cout << "Allocated " << rankDPUsVec.size() << " ranks\n";
  std::cout << "Total transferring data size is: " << wholeSize_GiB << " GiB\n";
  const auto dpuPath = getDPUBinaryPath();
  wholeSys.load(dpuPath);

/*
  const vector<vector<std::uint32_t>> buffer(64,
               vector<std::uint32_t>(elementPerDPU, 666)); // whole rank parallel
               */
  dpuData dullBlock = dpuData(elementPerDPU, 666);
  vector<rankData> RankBuffers;

  #pragma omp parallel for
  for(auto rankIdx = 0; rankIdx < rankNum; rankIdx++){
    auto thisDPUNum = rankDPUsVec[rankIdx];
    RankBuffers.emplace_back(rankData(thisDPUNum, dullBlock));
  }
  
  /*
  constexpr size_t choice = 0;
  const auto thisDPUNum = rankDPUsVec[choice];
  vector<vector<std::uint32_t>> result(thisDPUNum,vector<uint32_t> (elementPerDPU,0));
  */
  constexpr int warmup = 5;
  constexpr int rep = 20;

  for(auto usedRank = 1; usedRank < rankNum; usedRank<<=1){
    for (int i = 0; i < rep+warmup; i++) {
      if(i >= warmup) ct.tick("RankWisePush_32MiB_" + std::to_string(usedRank) + "Rank");
      summonPush(&wholeSys, RankBuffers,usedRank);
      if(i >= warmup) ct.tick("RankWisePush_32MiB_" + std::to_string(usedRank) + "Rank");

      if(i >= warmup) ct.tick("RankWisePull_32MiB_" + std::to_string(usedRank) + "Rank");
      summonPull(RankBuffers, &wholeSys,usedRank);
      if(i >= warmup) ct.tock("RankWisePull_32MiB_" + std::to_string(usedRank) + "Rank");
      
      if(i >= warmup) ct.tick("Sync_" + std::to_string(usedRank) + "Rank");
      wholeSys.async().sync();
      if(i >= warmup) ct.tock("Sync_" + std::to_string(usedRank) + "Rank");
      std::cout << i<<std::endl;
    }
  }
  ct.dumpAllReport("/output/transferData");
  return 0;
}

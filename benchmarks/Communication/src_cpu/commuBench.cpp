#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <DPUInterface/DPUInterface.hpp>

using namespace MetaPB::DPUInterface;
using std::string;
using std::vector;

inline const string getDPUBinaryPath() {
  std::filesystem::path executablePath =
      std::filesystem::read_symlink("/proc/self/exe");
  const string parentPath = executablePath.parent_path().parent_path();
  const string dpuPath = parentPath + "/dpu_bin/commuDPU";
  return dpuPath;
}


template<typename T>
void elementWisePush(const std::vector<T> &data, const DpuSet* entity){
  entity->copy(DPU_MRAM_HEAP_POINTER_NAME,0,data);
}

template<typename T>
void elementWisePull(const std::vector<T> &data, const DpuSet* entity){
  entity->copy(DPU_MRAM_HEAP_POINTER_NAME,0,data);
}


int main(int argc, char **argv) {
  constexpr size_t mram_occupy_Byte = ((size_t)64 << 20); // 32MiB
  constexpr size_t elementPerDPU = mram_occupy_Byte / sizeof(std::uint32_t);

  const auto dpuPath = getDPUBinaryPath();
  auto dpuset = DpuSet::allocateRanks();
  dpuset.load(dpuPath);
  const auto dpuNum = dpuset.getDPUNums();
  const auto rankNum = dpuset.getDPURankNums();
  const auto rankNVec = dpuset.getRankWiseDPUNums();
  auto rankPtrVec = dpuset.ranks();
  auto dpuPtrVec = dpuset.dpus();
  std::cout << "Allocated " << rankNum << " ranks, " << dpuNum << " DPUs"
            << std::endl;
  
  const size_t wholeSize_Byte = dpuNum * mram_occupy_Byte;
  const double wholeSize_GiB = ((double)wholeSize_Byte / (1 << 30));

  std::vector<std::uint32_t> src(elementPerDPU, 666);
  std::vector<std::vector<std::uint32_t>> deBuffer(
      dpuNum, std::vector<std::uint32_t>(elementPerDPU, 666)); //expected to be the same at last
  constexpr int warmup = 5;
  constexpr int rep = 20;

  std::cout<< "Starting Rank-wise copy testing"
  for (int i = 0; i < rep; i++) {
    t.tick(0);
    dpu.copy(DPU_MRAM_HEAP_POINTER_NAME, 0, src, elementPerDPU);
    t.tock(0);

    t.tick(1);
    dpu.copy(result, elementPerDPU, DPU_MRAM_HEAP_POINTER_NAME);
    t.tock(1);
  }

  std::vector<std::thread> threads;
  for (int th = 0; th < numDevices; ++th) {
      threads.emplace_back(elementWisePush, std::ref(deBuffer[th]));
  }
  // 等待所有线程完成
  for (std::thread& t : threads) {
      t.join();
  }
  return 0;
}

// TODO:
//  1. Re-write model constructing with only interpolation.
//  2. Re-write model caching/loading related code
#include "Operator/OperatorBase.hpp"
#include <algorithm>
#include <cstdlib>

namespace MetaPB {
namespace Operator {

using utils::metricTag;
using utils::Stats;

std::string OperatorBase::getDPUBinaryPath() const noexcept {
  std::filesystem::path executablePath =
      std::filesystem::read_symlink("/proc/self/exe");
  const string parentPath = executablePath.parent_path().parent_path();
  const string dpuPath = parentPath + "/dpu_bin/" + get_name();
  return dpuPath;
}

perfStats OperatorBase::execCPUwithProbe(const CPU_TCB &cpuTCB) noexcept {
  const float dataSize_MiB = (size_t)cpuTCB.pageBlkCnt * (size_t)pageBlkSize / (float)(1 << 20);
  string taskName = "CPU_" + get_name() + std::to_string(dataSize_MiB) + "MiB";
  cpuExecJob2Name[dataSize_MiB] = taskName;
  for (int rep = 0; rep < PREPROBE_WARMUP_REP + PROBE_REP; rep++) {
    if (rep >= PREPROBE_WARMUP_REP)
      ct.tick(taskName);
    execCPU(cpuTCB);
    if (rep >= PREPROBE_WARMUP_REP)
      ct.tock(taskName);
  }
  auto report = ct.getReport(taskName);
  auto timeMean =
      std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data).mean /
      1e9;
  auto energyMeans = std::get<vector<Stats>>(
      report.reportItems[metricTag::CPUPowerConsumption_Joule].data);
  auto energyMeanSum = energyMeans[0].mean + energyMeans[1].mean;
  ct.clear();
  return {energyMeanSum, timeMean};
}

perfStats OperatorBase::execDPUwithProbe(const DPU_TCB &dpuTCB) noexcept {
  // Attention, a page in DPU MRAM is corresponding to a pageBlk in CPU DRAM
  const float dataSize_MiB = (size_t)dpuTCB.pageCnt * (size_t)pageBlkSize / (float)(1 << 20);
  string taskName = "DPU_" + get_name() + std::to_string(dataSize_MiB) + "MiB";
  dpuExecJob2Name[dataSize_MiB] = taskName;

  for (int rep = 0; rep < PREPROBE_WARMUP_REP + PROBE_REP; rep++) {
    if (rep >= PREPROBE_WARMUP_REP)
      ct.tick(taskName);
    execDPU(dpuTCB);
    if (rep >= PREPROBE_WARMUP_REP)
      ct.tock(taskName);
  }
  auto report = ct.getReport(taskName);
  auto timeMean =
      std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data).mean /
      1e9;
  auto energyMean = timeMean * DPU_ENERGY_CONSTANT_PER_SEC;

  return {energyMean, timeMean};
}

// -------------------- Checkpoint strategy -------------------------
void OperatorBase::savePerfSamples(const perfStats src[],
                                   const std::string &path) const noexcept {
  std::ofstream dataFile(path);
  std::cout << "saving perf sample file to: " << path << std::endl;
  dataFile << "dataSize_MiB,timeCost_Second,energyCost_joule\n";
  size_t pageCnt_step = (deducePageBlkUpperBound) / (PERF_SAMPLE_POINT - 1);
  int sampleIdx = 0;
  for (uint32_t pageCnt = 0; pageCnt <= deducePageBlkUpperBound;
       pageCnt += pageCnt_step) {
    float dataSize_MiB = pageCnt * pageBlkSize / (float)(1 << 20);
    dataFile << std::to_string(dataSize_MiB) << ","
             << std::to_string(src[sampleIdx].timeCost_Second) << ","
             << std::to_string(src[sampleIdx].energyCost_Joule);
    sampleIdx++;
    if (pageCnt + pageCnt_step <= deducePageBlkUpperBound)
      dataFile << "\n";
  }
}

void OperatorBase::loadPerfSamples(perfStats dst[],
                                   const std::string &path) const noexcept {
  std::ifstream file(path);

  std::string line, word;
  getline(file, line); // Skip header line
  int sampleIdx = 0;
  while (getline(file, line)) {
    std::istringstream s(line);
    perfStats stats;

    // Read data size
    getline(s, word, ','); // pass the datasize part

    // Read time cost
    getline(s, word, ',');
    stats.timeCost_Second = std::stod(word);

    getline(s, word, ',');
    stats.energyCost_Joule = std::stod(word);

    dst[sampleIdx++] = stats;
  }

  file.close();
}

bool OperatorBase::loadModelCacheIfExist(
    const uint32_t pageUpperBound) noexcept {
  float dataSize_MiB = pageUpperBound * (std::size_t)pageBlkSize / (float)(1 << 20);
  std::string modelTagPostfix =
      this->get_name() + "_" + std::to_string(dataSize_MiB) + "_MiB_" +
      std::to_string(PERF_SAMPLE_POINT) + "_sample.csv";

  std::string modelPathPrefix = REGRESSION_MODEL_CACHE_PATH;

  if (checkIfIsTrainable()) {
    std::string CPUPerfSamplePath =
        modelPathPrefix + "CPUPerfSamples_" + modelTagPostfix;
    std::cout << "loading " << CPUPerfSamplePath << std::endl;
    if (std::filesystem::exists({CPUPerfSamplePath})) {
      loadPerfSamples(CPUPerfSamples, CPUPerfSamplePath);
    } else {
      return false;
    }
    if (!checkIfIsCPUOnly()) {
      std::string DPUPerfSamplePath =
          modelPathPrefix + "DPUPerfSamples_" + modelTagPostfix;
      std::cout << "loading " << DPUPerfSamplePath << std::endl;
      if (std::filesystem::exists({DPUPerfSamplePath})) {
        loadPerfSamples(DPUPerfSamples, DPUPerfSamplePath);
      } else {
        return false;
      }
    }
  }
  this->isTrained = true;
  this->deducePageBlkUpperBound = pageUpperBound;
  std::cout << modelTagPostfix << " is loaded" << std::endl;
  return true;
}

// ---------------- Model training -----------------------
void OperatorBase::trainModel(const uint32_t pageBlkUpperBound) noexcept {
  if (!loadModelCacheIfExist(pageBlkUpperBound) && checkIfIsTrainable()) {
    std::cout << "Model cache not exist, start training" << std::endl;
    std::uint32_t pageStep = pageBlkUpperBound / (PERF_SAMPLE_POINT - 1);
    const std::uint32_t pageBlkSize = getPageBlkSize();
    int sampleIdx = 0;

    void *src1 = malloc((size_t)pageBlkUpperBound * pageBlkSize);
    void *src2 = malloc((size_t)pageBlkUpperBound * pageBlkSize);
    void *dst = malloc((size_t)pageBlkUpperBound * pageBlkSize);
    std::cout << (size_t)pageBlkUpperBound << " -- " << pageBlkSize<<std::endl;

    CPU_TCB cpuTCB{src1, src2, dst};
    cpuTCB.sgInfo = {src1, 0, 0};
    for (std::uint32_t pageBlkCnt = 0; pageBlkCnt <= pageBlkUpperBound;
         pageBlkCnt += pageStep) {
      float dataSize_MiB = pageBlkCnt * (std::size_t)pageBlkSize / (float)(1 << 20);
      std::cout << "Training with pageBlkCnt " <<pageBlkCnt <<  " dataSize: " << dataSize_MiB << " MiB\n";
      cpuTCB.pageBlkCnt = pageBlkCnt; cpuTCB.sgInfo.pageBlkCnt = pageBlkCnt;

      CPUPerfSamples[sampleIdx] = execCPUwithProbe(cpuTCB);
      if (!checkIfIsCPUOnly()) {
        DPU_TCB dpuTCB{pageBlkCnt, 2 * pageBlkCnt, 3 * pageBlkCnt, pageBlkCnt};
        DPUPerfSamples[sampleIdx] = execDPUwithProbe(dpuTCB);
      }
      sampleIdx++;
    }
    this->deducePageBlkUpperBound = pageBlkUpperBound;
    this->isTrained = true;
    float dataSizeUpperbound_MiB = pageBlkUpperBound * (size_t)pageBlkSize / (float)(1 << 20);
    std::cout << "pageBlkUpperBound = " << pageBlkUpperBound 
              << ", pageBlkSize = " << pageBlkSize
              << ", dataSize upperBound = " << dataSizeUpperbound_MiB<<std::endl;
    std::string modelTagPostfix =
        this->get_name() + "_" + std::to_string(dataSizeUpperbound_MiB) +
        "_MiB_" + std::to_string(PERF_SAMPLE_POINT) + "_sample.csv";

    std::string modelPathPrefix = REGRESSION_MODEL_CACHE_PATH;
    std::string CPUPerfSamplePath =
        modelPathPrefix + "CPUPerfSamples_" + modelTagPostfix;
    std::string DPUPerfSamplePath =
        modelPathPrefix + "DPUPerfSamples_" + modelTagPostfix;
    savePerfSamples(CPUPerfSamples, CPUPerfSamplePath);
    savePerfSamples(DPUPerfSamples, DPUPerfSamplePath);
    free(src1);
    free(src2);
    free(dst);
  }
}

} // namespace Operator
} // namespace MetaPB

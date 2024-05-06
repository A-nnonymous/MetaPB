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

perfStats OperatorBase::execCPUwithProbe(const size_t batchSize_MiB,
                                         void **memPoolBffrPtrs) noexcept {
  const float dataSize_MiB = (float)batchSize_MiB;
  string taskName = "CPU_" + get_name() + std::to_string(dataSize_MiB) + "MiB";
  cpuExecJob2Name[dataSize_MiB] = taskName;
  for (int rep = 0; rep < PREPROBE_WARMUP_REP + PROBE_REP; rep++) {
    if (rep >= PREPROBE_WARMUP_REP)
      ct.tick(taskName);
    execCPU(batchSize_MiB, memPoolBffrPtrs);
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

perfStats OperatorBase::execDPUwithProbe(const size_t batchSize_MiB) noexcept {
  const float dataSize_MiB = (float)batchSize_MiB;
  string taskName = "DPU_" + get_name() + std::to_string(dataSize_MiB) + "MiB";
  dpuExecJob2Name[dataSize_MiB] = taskName;

  for (int rep = 0; rep < PREPROBE_WARMUP_REP + PROBE_REP; rep++) {
    if (rep >= PREPROBE_WARMUP_REP)
      ct.tick(taskName);
    execDPU(batchSize_MiB);
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
  size_t step_MiB =
      (deduceSizeUpperBound_MiB - BATCH_LOWERBOUND_MB) / PERF_SAMPLE_POINT;
  int sampleIdx = 0;
  for (size_t dataSize_MiB = BATCH_LOWERBOUND_MB;
       dataSize_MiB <= deduceSizeUpperBound_MiB; dataSize_MiB += step_MiB) {
    dataFile << std::to_string(dataSize_MiB) << ","
             << std::to_string(src[sampleIdx].timeCost_Second) << ","
             << std::to_string(src[sampleIdx].energyCost_Joule);
    sampleIdx++;
    if (dataSize_MiB + step_MiB <= deduceSizeUpperBound_MiB)
      dataFile << "\n";
  }
}

void OperatorBase::cacheModel(const size_t batchSize_MiB) const noexcept {
  std::string modelTagPostfix =
      this->get_name() + "_" + std::to_string(batchSize_MiB) + "_MiB_" +
      std::to_string(PERF_SAMPLE_POINT) + "_sample.csv";
  std::string modelPathPrefix = REGRESSION_MODEL_CACHE_PATH;
  std::filesystem::path modelPath{modelPathPrefix};
  if (!std::filesystem::exists(modelPath))
    std::filesystem::create_directories(modelPath);
  if (checkIfIsTrainable()) {
    savePerfSamples(CPUPerfSamples,
                    modelPathPrefix + "CPUPerfSamples_" + modelTagPostfix);
    if (!checkIfIsCPUOnly()) {
      savePerfSamples(DPUPerfSamples,
                      modelPathPrefix + "DPUPerfSamples_" + modelTagPostfix);
    }
  }
  std::cout << modelTagPostfix << " is cached" << std::endl;
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

bool OperatorBase::loadModelCacheIfExist(const size_t batchSize_MiB) noexcept {
  std::string modelTagPostfix =
      this->get_name() + "_" + std::to_string(batchSize_MiB) + "_MiB_" +
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
  this->deduceSizeUpperBound_MiB = batchSize_MiB;
  std::cout << modelTagPostfix << " is loaded" << std::endl;
  return true;
}

// ---------------- Model training -----------------------
void OperatorBase::trainModel(const size_t batchUpperBound_MiB,
                              void **memPoolBffrPtrs,
                              const int iterMax) noexcept {
  if (!loadModelCacheIfExist(batchUpperBound_MiB) && checkIfIsTrainable()) {
    std::cout << "Model cache not exist, start training" << std::endl;
    size_t upperBound_MiB = batchUpperBound_MiB;
    size_t step_MiB =
        (upperBound_MiB - BATCH_LOWERBOUND_MB) / PERF_SAMPLE_POINT;
    int sampleIdx = 0;
    for (size_t batchSize_MiB = BATCH_LOWERBOUND_MB;
         batchSize_MiB <= batchUpperBound_MiB; batchSize_MiB += step_MiB) {
      std::cout << "Training with data batchsize: " << batchSize_MiB
                << " MiB\n";
      CPUPerfSamples[sampleIdx] =
          execCPUwithProbe(batchSize_MiB, memPoolBffrPtrs);
      if (!checkIfIsCPUOnly())
        DPUPerfSamples[sampleIdx] = execDPUwithProbe(batchSize_MiB);

      sampleIdx++;
    }
    this->deduceSizeUpperBound_MiB = upperBound_MiB;
    this->isTrained = true;
    std::string modelTagPostfix =
        this->get_name() + "_" + std::to_string(batchUpperBound_MiB) + "_MiB_" +
        std::to_string(PERF_SAMPLE_POINT) + "_sample.csv";

    std::string modelPathPrefix = REGRESSION_MODEL_CACHE_PATH;
    std::string CPUPerfSamplePath =
        modelPathPrefix + "CPUPerfSamples_" + modelTagPostfix;
    std::string DPUPerfSamplePath =
        modelPathPrefix + "DPUPerfSamples_" + modelTagPostfix;
    savePerfSamples(CPUPerfSamples, CPUPerfSamplePath);
    savePerfSamples(DPUPerfSamples, DPUPerfSamplePath);

    /*
    const size_t cpuQuantSize = cpuExecJob2Name.size();
    float cpuInputSize[cpuQuantSize];
    float cpuTimeCost[cpuQuantSize];
    float cpuEnergyCost[cpuQuantSize];

    int counter = 0; // forget these code... as you never seen these
    for (const auto &[dataSize_MiB, taskName] : cpuExecJob2Name) {
      cpuInputSize[counter] = dataSize_MiB;
      const auto report = ct.getReport(taskName);
      cpuTimeCost[counter] =
          std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data)
              .mean /
          1e9;
      auto energyMeans = std::get<vector<Stats>>(
          report.reportItems[metricTag::CPUPowerConsumption_Joule].data);
      cpuEnergyCost[counter] = energyMeans[0].mean + energyMeans[1].mean;
      counter++;
    }
    std::cout << "Training CPU quant regression model for operator "
              << get_name() << std::endl;
    CPUPerfLearner.train(cpuInputSize, cpuTimeCost, cpuQuantSize, iterMax);
    CPUEnergyLearner.train(cpuInputSize, cpuEnergyCost, cpuQuantSize, iterMax);

    if (!checkIfIsCPUOnly()) {
      const size_t dpuQuantSize = dpuExecJob2Name.size();
      float dpuInputSize[dpuQuantSize];
      float dpuTimeCost[dpuQuantSize];
      counter = 0;
      for (const auto &[dataSize_MiB, taskName] : dpuExecJob2Name) {
        dpuInputSize[counter] = dataSize_MiB;
        const auto report = ct.getReport(taskName);
        dpuTimeCost[counter] =
            std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data)
                .mean /
            1e9;
        counter++;
      }
      std::cout << "Training DPU quant regression model for operator "
                << get_name() << std::endl;
      DPUPerfLearner.train(dpuInputSize, dpuTimeCost, dpuQuantSize, iterMax);
    }

    std::cout << "Regression model of " << get_name() << " is trained after "
              << REGRESSION_TRAINING_ITER << " iteration." << std::endl;

    this->isTrained = true;
    this->deduceSizeUpperBound_MiB = batchUpperBound_MiB;
    cacheModel(batchUpperBound_MiB);
  } else {
    this->isTrained = true;
    this->deduceSizeUpperBound_MiB = batchUpperBound_MiB;
    std::cout << "warmup Operator " << get_name()
              << "'s Model cache \n";
    for(int i = 0; i < PERF_SAMPLE_POINT; i++){
      size_t step_MiB =
        (this->deduceSizeUpperBound_MiB - BATCH_LOWERBOUND_MB) /
  PERF_SAMPLE_POINT; std::cout << "Deducing:" << i <<std::endl;
      CPUPerfSamples[i] = this->deducePerfCPU(BATCH_LOWERBOUND_MB + i *
  step_MiB); if(!checkIfIsCPUOnly())DPUPerfSamples[i] =
  this->deducePerfDPU(BATCH_LOWERBOUND_MB + i * step_MiB);
    }
    std::cout << "Operator " << get_name()
              << "'s Model cache hits sucessfully\n";
  }
  */
  }
}

void OperatorBase::dumpMetrics(const size_t batchSize_MiB,
                               void **memPoolBffrPtrs,
                               const std::string dumpPath) noexcept {
  vector<perfStats> hybridPerf;
  for (int i = 0; i <= 10; i++) {
    const size_t cpuBatch = i * batchSize_MiB / 10;
    const size_t dpuBatch = batchSize_MiB - cpuBatch;
    perfStats cpuStat = execCPUwithProbe(cpuBatch, memPoolBffrPtrs);
    perfStats dpuStat = execDPUwithProbe(dpuBatch);
    perfStats combine = {
        cpuStat.energyCost_Joule + dpuStat.energyCost_Joule,
        std::max(cpuStat.timeCost_Second, dpuStat.timeCost_Second)};
    hybridPerf.emplace_back(combine);
  }
}

/* // Legacy decision tree model
perfStats OperatorBase::deducePerfCPU(const size_t batchSize_MiB) noexcept {
  if (!isTrained || !checkIfIsTrainable()) {
    return {};
  } else {
    float cpuBatchSize_MiB[1] = {(float)batchSize_MiB};
    float cpuTimePredict[1];
    float cpuEnergyPredict[1];

    CPUPerfLearner.predict(cpuBatchSize_MiB, 1, cpuTimePredict);
    CPUEnergyLearner.predict(cpuBatchSize_MiB, 1, cpuEnergyPredict);

    perfStats result;
    result.energyCost_Joule = cpuEnergyPredict[0];
    result.timeCost_Second = cpuTimePredict[0];
    return result;
  }
}

perfStats OperatorBase::deducePerfDPU(const size_t batchSize_MiB) noexcept {
  if (!isTrained || !checkIfIsTrainable()) {
    return {};
  } else {
    perfStats result;
    float dpuBatchSize_MiB[1] = {(float)batchSize_MiB};
    float dpuTimePredict[1];
    DPUPerfLearner.predict(dpuBatchSize_MiB, 1, dpuTimePredict);
    result.timeCost_Second = dpuTimePredict[0];
    result.energyCost_Joule = dpuTimePredict[0] * DPU_ENERGY_CONSTANT_PER_SEC;
    return result;
  }
}
*/

} // namespace Operator
} // namespace MetaPB

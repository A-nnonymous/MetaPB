#include "Operator/OperatorBase.hpp"
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

  return {timeMean, energyMeanSum};
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

  return {timeMean, energyMean};
}

void OperatorBase::trainModel(const size_t batchUpperBound_MiB,
                              void **memPoolBffrPtrs,
                              const int iterMax) noexcept {
  if (!loadModelCacheIfExist(batchUpperBound_MiB) && checkIfIsTrainable()) {
    std::cout << "loading failed, start training" << std::endl;
    size_t upperBound_MiB = batchUpperBound_MiB;
    size_t step_MiB =
        (upperBound_MiB - BATCH_LOWERBOUND_MB) / PERF_SAMPLE_POINT;
    for (size_t batchSize_MiB = BATCH_LOWERBOUND_MB;
         batchSize_MiB < batchUpperBound_MiB; batchSize_MiB += step_MiB) {
      std::cout << "Training with data batchsize: " << batchSize_MiB
                << " MiB\n";
      execCPUwithProbe(batchSize_MiB, memPoolBffrPtrs);
      if (!checkIfIsCPUOnly())
        execDPUwithProbe(batchSize_MiB);
    }

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
  }
  cacheModel(batchUpperBound_MiB);
  ct.dumpAllReport("./");
}

void OperatorBase::verifyRegression(const std::string &filePath,
                                    const size_t batchUpperBound_MiB) {

  std::string command = "mkdir -p " + filePath;
  system(command.c_str());
  utils::CSVWriter<float> w;
  std::vector<std::string> header = {"dataSize_MiB", "Time_Second"};
  std::string reportsPostfix = "Scatters_" + get_name() + ".csv";
  std::string regressionPostfix = "Regression_" + get_name() + ".csv";
  std::vector<std::vector<float>> cpuPerfReal;
  std::vector<std::vector<float>> cpuEnergyReal;
  std::vector<std::vector<float>> cpuPerfRegress;
  std::vector<std::vector<float>> cpuEnergyRegress;
  std::vector<std::vector<float>> dpuPerfReal;
  std::vector<std::vector<float>> dpuPerfRegress;
  for (const auto &[dataSize_MiB, taskName] : cpuExecJob2Name) {
    const auto report = ct.getReport(taskName);
    float cpuTimeCost =
        std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data)
            .mean /
        1e9;
    auto energyMeans = std::get<vector<Stats>>(
        report.reportItems[metricTag::CPUPowerConsumption_Joule].data);
    float cpuEnergyCost = energyMeans[0].mean + energyMeans[1].mean;
    cpuPerfReal.emplace_back(std::vector<float>{dataSize_MiB, cpuTimeCost});
    cpuEnergyReal.emplace_back(std::vector<float>{dataSize_MiB, cpuEnergyCost});
    cpuPerfRegress.emplace_back(std::vector<float>{
        dataSize_MiB, deducePerfCPU(dataSize_MiB).timeCost_Second});
    cpuEnergyRegress.emplace_back(std::vector<float>{
        dataSize_MiB, deducePerfCPU(dataSize_MiB).energyCost_Joule});
  }

  for (const auto &[dataSize_MiB, taskName] : dpuExecJob2Name) {
    const auto report = ct.getReport(taskName);
    auto dpuTimeCost =
        std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data)
            .mean /
        1e9;
    dpuPerfReal.emplace_back(std::vector<float>{dataSize_MiB, dpuTimeCost});
    dpuPerfRegress.emplace_back(std::vector<float>{
        dataSize_MiB, deducePerfDPU(dataSize_MiB).timeCost_Second});
  }
  for (float batch = (float)BATCH_LOWERBOUND_MB; batch < batchUpperBound_MiB;
       batch += (batchUpperBound_MiB - BATCH_LOWERBOUND_MB) / 100.0) {
    cpuPerfRegress.emplace_back(
        std::vector<float>{batch, deducePerfCPU(batch).timeCost_Second});
    cpuEnergyRegress.emplace_back(
        std::vector<float>{batch, deducePerfCPU(batch).energyCost_Joule});
    dpuPerfRegress.emplace_back(
        std::vector<float>{batch, deducePerfDPU(batch).timeCost_Second});
  }
  w.writeCSV(cpuPerfReal, header, filePath + "CPU_perf_" + reportsPostfix);
  w.writeCSV(cpuEnergyReal, header, filePath + "CPU_energy_" + reportsPostfix);
  w.writeCSV(cpuPerfRegress, header,
             filePath + "CPU_perf_" + regressionPostfix);
  w.writeCSV(cpuEnergyRegress, header,
             filePath + "CPU_energy_" + regressionPostfix);

  w.writeCSV(dpuPerfReal, header, filePath + "DPU_perf_" + reportsPostfix);
  w.writeCSV(dpuPerfRegress, header,
             filePath + "DPU_perf_" + regressionPostfix);
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

perfStats OperatorBase::deducePerf(const double offloadRatio,
                                   const size_t batchSize_MiB) noexcept {
  if (!isTrained || !checkIfIsTrainable()) {
    return {};
  } else {
    float cpuBatchSize_MiB[1] = {(1 - offloadRatio) * batchSize_MiB};
    float cpuTimePredict[1];
    float cpuEnergyPredict[1];

    CPUPerfLearner.predict(cpuBatchSize_MiB, 1, cpuTimePredict);
    CPUEnergyLearner.predict(cpuBatchSize_MiB, 1, cpuEnergyPredict);

    perfStats result;

    if (checkIfIsCPUOnly()) {
      result.energyCost_Joule = cpuEnergyPredict[0];
      result.timeCost_Second = cpuTimePredict[0];
    } else {
      float dpuBatchSize_MiB[1] = {(offloadRatio)*batchSize_MiB};
      float dpuTimePredict[1];
      DPUPerfLearner.predict(dpuBatchSize_MiB, 1, dpuTimePredict);
      result.timeCost_Second = std::max(cpuTimePredict[0], dpuTimePredict[0]);
      result.energyCost_Joule =
          cpuEnergyPredict[0] + dpuTimePredict[0] * DPU_ENERGY_CONSTANT_PER_SEC;
    }

    return result;
  }
}
perfStats OperatorBase::deducePerfCPU(const size_t batchSize_MiB) noexcept {
  if (!isTrained || !checkIfIsTrainable()) {
    std::cout << "Operator " << get_name() << " is deducing without training"
              << std::endl;
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
    std::cout << "Operator " << get_name() << " is deducing without training"
              << std::endl;
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

void OperatorBase::cacheModel(const size_t batchSize_MiB) const noexcept {
  std::string modelTagPostfix =
      this->get_name() + "_" + std::to_string(batchSize_MiB) + "_MiB.bin";
  std::string modelPathPrefix = REGRESSION_MODEL_CACHE_PATH;
  std::filesystem::path modelPath{modelPathPrefix};
  if (!std::filesystem::exists(modelPath))
    std::filesystem::create_directories(modelPath);
  if (checkIfIsTrainable()) {
    CPUPerfLearner.saveModel(modelPathPrefix + "CPUPerfModel_" +
                             modelTagPostfix);
    CPUEnergyLearner.saveModel(modelPathPrefix + "CPUEnergyModel_" +
                               modelTagPostfix);
    if (!checkIfIsCPUOnly()) {
      DPUPerfLearner.saveModel(modelPathPrefix + "DPUPerfModel_" +
                               modelTagPostfix);
    }
  }
  std::cout << modelTagPostfix << " is cached" << std::endl;
}

bool OperatorBase::loadModelCacheIfExist(const size_t batchSize_MiB) noexcept {
  std::string modelTagPostfix =
      get_name() + "_" + std::to_string(batchSize_MiB) + "_MiB.bin";
  std::string modelPathPrefix = REGRESSION_MODEL_CACHE_PATH;
  std::cout << "loading " << modelPathPrefix << "*" << modelTagPostfix
            << std::endl;

  if (checkIfIsTrainable()) {
    std::string CPUPerfModelPath =
        modelPathPrefix + "CPUPerfModel_" + modelTagPostfix;
    std::string CPUEnergyModelPath =
        modelPathPrefix + "CPUEnergyModel_" + modelTagPostfix;
    if (std::filesystem::exists({CPUPerfModelPath}))
      CPUPerfLearner.loadModel(CPUPerfModelPath);
    else
      return false;
    if (std::filesystem::exists({CPUEnergyModelPath}))
      CPUEnergyLearner.loadModel(CPUEnergyModelPath);
    else
      return false;
    if (!checkIfIsCPUOnly()) {
      std::string DPUPerfModelPath =
          modelPathPrefix + "DPUPerfModel_" + modelTagPostfix;
      if (std::filesystem::exists({DPUPerfModelPath}))
        DPUPerfLearner.loadModel(DPUPerfModelPath);
      else
        return false;
    }
  }
  this->isTrained = true;
  std::cout << modelTagPostfix << " is loaded" << std::endl;
  return true;
}

} // namespace Operator
} // namespace MetaPB

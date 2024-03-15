#include "Operator/OperatorBase.hpp"
#include "utils/MetricsGather.hpp"
#include "utils/Stats.hpp"

namespace MetaPB{
namespace Operator{

using utils::metricTag;
using utils::Stats; 

perfStats OperatorBase::execCPUwithProbe(const size_t batchSize_MiB) noexcept{
  const float dataSize_MiB = (float)batchSize_MiB * sizeof(int) / (1<<20); 
  string taskName = "CPU_" + get_name() + 
                    std::to_string(dataSize_MiB) + "MiB";
  cpuExecJob2Name[dataSize_MiB] = taskName;
  for(int rep = 0; rep < PREPROBE_WARMUP_REP + PROBE_REP; rep++){
    if(rep >= PREPROBE_WARMUP_REP)ct.tick(taskName);
    execCPU(batchSize_MiB);
    if(rep >= PREPROBE_WARMUP_REP)ct.tock(taskName);
  }
  auto report = ct.getReport(taskName);
  auto timeMean = std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data).mean / 1e9;
  auto energyMeans = std::get<vector<Stats>>(report.reportItems[metricTag::CPUPowerConsumption_Joule].data);
  auto energyMeanSum = energyMeans[0].mean + energyMeans[1].mean;

  return{timeMean, energyMeanSum};
}

perfStats OperatorBase::execDPUwithProbe(const size_t batchSize_MiB) noexcept{
  const float dataSize_MiB = (float)batchSize_MiB * sizeof(int) / (1<<20); 
  string taskName = "DPU_" + get_name() + 
                    std::to_string(dataSize_MiB) + "MiB";
  dpuExecJob2Name[dataSize_MiB] = taskName;

  for(int rep = 0; rep < PREPROBE_WARMUP_REP + PROBE_REP; rep++){
    if(rep >= PREPROBE_WARMUP_REP)ct.tick(taskName);
    execDPU(batchSize_MiB);
    if(rep >= PREPROBE_WARMUP_REP)ct.tock(taskName);
  }
  auto report = ct.getReport(taskName);
  auto timeMean = std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data).mean / 1e9;
  auto energyMean = timeMean * DPU_ENERGY_CONSTANT_PER_SEC;

  return{timeMean, energyMean};
}

void OperatorBase::trainModel(const size_t batchUpperBound_MiB)noexcept{
  size_t upperBound_MiB = batchUpperBound_MiB;
  size_t step_MiB = (upperBound_MiB - BATCH_LOWERBOUND_MB)/ PERF_SAMPLE_POINT;
  for(size_t batchSize_MiB = BATCH_LOWERBOUND_MB;
      batchSize_MiB < batchUpperBound_MiB;
      batchSize_MiB += step_MiB){
    execCPUwithProbe(batchSize_MiB);
    execDPUwithProbe(batchSize_MiB);
  }

  const size_t cpuQuantSize = cpuExecJob2Name.size();
  const size_t dpuQuantSize = dpuExecJob2Name.size();
  std::cout<< "learning with dataset size:" << cpuQuantSize<<std::endl;
  float cpuInputSize[cpuQuantSize];
  float cpuTimeCost[cpuQuantSize];
  float cpuEnergyCost[cpuQuantSize];
  float dpuInputSize[dpuQuantSize];
  float dpuTimeCost[dpuQuantSize];

  int counter = 0; // forget these code... as you never seen these
  for(const auto &[dataSize_MiB, taskName] : cpuExecJob2Name){
    cpuInputSize[counter] = dataSize_MiB;
    const auto report = ct.getReport(taskName);
    cpuTimeCost[counter] = std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data).mean / 1e9;
    auto energyMeans = std::get<vector<Stats>>(report.reportItems[metricTag::CPUPowerConsumption_Joule].data);
    cpuEnergyCost[counter] = energyMeans[0].mean + energyMeans[1].mean;
    counter++;
  }
  counter = 0;
  for(const auto &[dataSize_MiB, taskName] : dpuExecJob2Name){
    dpuInputSize[counter] = dataSize_MiB;
    const auto report = ct.getReport(taskName);
    dpuTimeCost[counter] = std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data).mean / 1e9;
    counter++;
  }

  CPUPerfLearner.train(cpuInputSize,cpuTimeCost,1);
  CPUEnergyLearner.train(cpuInputSize,cpuEnergyCost,1);
  DPUPerfLearner.train(dpuInputSize,dpuTimeCost,1);

}

void OperatorBase::dumpMetrics(const size_t batchSize_MiB, const std::string dumpPath)noexcept{
  vector<perfStats> hybridPerf;
  for(int i = 0; i <= 10; i++){
    const size_t cpuBatch = i * batchSize_MiB / 10;
    const size_t dpuBatch = batchSize_MiB - cpuBatch;
    perfStats cpuStat = execCPUwithProbe(cpuBatch);
    perfStats dpuStat = execDPUwithProbe(dpuBatch);
    perfStats combine = {cpuStat.energyCost_Joule + dpuStat.energyCost_Joule,
                         max(cpuStat.timeCost_Second, dpuStat.timeCost_Second)};
    hybridPerf.emplace_back(combine);
  }
}

perfStats OperatorBase::deducePerf(const double offloadRatio, const size_t batchSize_MiB)noexcept{
  float cpuBatchSize_MiB[1] = {(1-offloadRatio) * batchSize_MiB};
  float dpuBatchSize_MiB[1] = {(offloadRatio) * batchSize_MiB};
  
  float cpuTimePredict[1];
  float cpuEnergyPredict[1];
  float dpuTimePredict[1];
  
  CPUPerfLearner.predict(cpuBatchSize_MiB,1,cpuTimePredict);
  CPUEnergyLearner.predict(cpuBatchSize_MiB,1,cpuEnergyPredict);
  DPUPerfLearner.predict(cpuBatchSize_MiB,1,dpuTimePredict);

  perfStats result;
  result.timeCost_Second = max(cpuTimePredict[0], dpuTimePredict[0]);
  result.energyCost_Joule = cpuEnergyPredict[0] + dpuTimePredict[0] * DPU_ENERGY_CONSTANT_PER_SEC;
  return result;
}



} // namespace Operator
} // namespace MetaPB
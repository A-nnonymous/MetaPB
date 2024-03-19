#include "Scheduler/MetaScheduler.hpp"

namespace MetaPB {
namespace Scheduler{

  std::vector<float> MetaScheduler::evalSchedules(const vector<vector<float>> &ratioVecs){
    const int agentNum = ratioVecs.size();
    std::vector<perfStats> stats(agentNum);
    std::vector<HeteroComputePool> pools;
    std::vector<Schedule> proposedSchedule(ratioVecs.size());
    for(auto& schedule: proposedSchedule){
      schedule.offloadRatio.resize(ratioVecs[0].size(),0.0f);
    }
    std::vector<float> evalResult(ratioVecs.size(), 0.0f);
    for(size_t i = 0; i < ratioVecs.size(); i++){
      proposedSchedule[i].order = this->HEFTorder;
      proposedSchedule[i].isCoarseGrain = false;
      for(size_t j = 0; j < ratioVecs[i].size(); j++){
        proposedSchedule[i].offloadRatio[j] = ratioVecs[i][j]/2.0f + 0.5f;
      }
    }
    for (int i = 0; i < agentNum; i++) {
      pools.emplace_back(std::move(
          HeteroComputePool{ratioVecs[0].size(), this->om, this->dummyPool}));
    }

    #pragma omp parallel for
    for (int i = 0; i < agentNum; i++) {
      stats[i] = pools[i].execWorkload(tg, proposedSchedule[i], execType::MIMIC);
    }
    for(int i = 0; i< stats.size(); i++){
      float scheduleEval = (Arg_Alpha * stats[i].timeCost_Second + Arg_Beta * stats[i].energyCost_Joule);
      evalResult[i] = scheduleEval;
    }
    return evalResult;
  }

  Schedule MetaScheduler::schedule() noexcept{
    size_t nTask = boost::num_vertices(tg.g);

    const double dt = 0.1;
    const double ego = 0.2;
    const double omega = 0.7;
    const double vMax = 2 * (200.0 / (OptIterMax * dt));

    const std::vector<float> lowerLimit(nTask, -1.0f);
    const std::vector<float> upperLimit(nTask, 1.0f);
    const size_t dimNum = nTask;
    const int pointNum = 10;
    const int iterNum = OptIterMax;

std::vector<std::unique_ptr<OptimizerBase>> oVec;
oVec.push_back(std::make_unique<Optimizer::OptimizerPSO<float, float>>(
    vMax, omega, dt, ego, lowerLimit, upperLimit, dimNum, pointNum, iterNum,
    [this](const std::vector<std::vector<float>>& ratioVecs) {
        return this->evalSchedules(ratioVecs);
    }));

oVec.push_back(std::make_unique<Optimizer::OptimizerAOA<float, float>>(
    lowerLimit, upperLimit, dimNum, pointNum, iterNum,
    [this](const std::vector<std::vector<float>>& ratioVecs) {
        return this->evalSchedules(ratioVecs);
    }));

oVec.push_back(std::make_unique<Optimizer::OptimizerRSA<float, float>>(
    lowerLimit, upperLimit, dimNum, pointNum, iterNum,
    [this](const std::vector<std::vector<float>>& ratioVecs) {
        return this->evalSchedules(ratioVecs);
    }));
    
    #pragma omp parallel for 
    for(int i = 0; i < oVec.size(); i++){
      #pragma omp task untied
      oVec[i]->exec();
    }
    float bestVal = 666666666.6f;
    std::vector<float> ratio(nTask,0.0f);
    for(const auto& opt : oVec){
      if(opt->getGlobalOptimaValue() < bestVal){
        ratio = opt->getGlobalOptimaPoint();
        bestVal = opt->getGlobalOptimaValue();
      }
    }
      std::cout <<"META Schedule result: \n";
    std::vector<float> actualRatio(nTask,0.0f);
    for(int i = 0; i< nTask; i++){
      std::cout << ratio[i]/2.0f + 0.5f<<",";
      actualRatio[i] = ratio[i]/2.0f + 0.5f;
    }
    std::cout <<std::endl;
    return {false, this->HEFTorder, actualRatio};
  }

} // Scheduler
} // MetaPB

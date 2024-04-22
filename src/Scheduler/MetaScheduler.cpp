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
        proposedSchedule[i].offloadRatio[j] = ratioVecs[i][j]/200.0f + 0.5f;
      }
    }
    for (int i = 0; i < agentNum; i++) {
      pools.emplace_back(std::move(
          HeteroComputePool{ratioVecs[0].size(), this->om, this->dummyPool}));
    }
    omp_set_num_threads(agentNum);
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < agentNum; i++) {
      stats[i] = pools[i].execWorkload(tg, proposedSchedule[i], execType::MIMIC);
    }
    for(int i = 0; i< stats.size(); i++){
      float scheduleEval = (Arg_Alpha * 200 * stats[i].timeCost_Second + Arg_Beta * stats[i].energyCost_Joule);
      evalResult[i] = scheduleEval;
    }
    return evalResult;
  }

  Schedule MetaScheduler::schedule() noexcept{
    size_t nTask = boost::num_vertices(tg.g);

    const double dt = 0.07;
    const double ego = 0.6;
    const double omega = 0.8;
    const double vMax = 2 * (200.0 / (OptIterMax * dt));

    const std::vector<float> lowerLimit(nTask, -100.0f);
    const std::vector<float> upperLimit(nTask, 100.0f);
    const size_t dimNum = nTask;
    const int pointNum = 20;
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
    
    float bestVal = 666666666.6f;
    std::vector<float> ratio(nTask,0.0f);
        #pragma omp parallel
    {
        #pragma omp single
        {
          //for(int i = 0; i < oVec.size(); i++){
          for(int i = 0; i < 2;i++){
                // 创建untied任务
                #pragma omp task untied
                {
                  oVec[i]->exec();
                }
            }
        }
    }
    int bestOptimizerIdx = -1; // init illegal value
    //for(int i = 0; i < oVec.size();i++){
    for(int i = 0; i < 2;i++){
      std::cout << "score "<< i << "=" << oVec[i]->getGlobalOptimaValue()<<"\n";
      if(oVec[i]->getGlobalOptimaValue() < bestVal){
        bestOptimizerIdx = i;
        bestVal = oVec[i]->getGlobalOptimaValue();
      }
    }
    ratio = oVec[bestOptimizerIdx]->getGlobalOptimaPoint();
    bestVal = oVec[bestOptimizerIdx]->getGlobalOptimaValue();
    
    /*
    int bestOptimizerIdx = 2; 
    oVec[bestOptimizerIdx]->exec();
    ratio = oVec[bestOptimizerIdx]->getGlobalOptimaPoint();
    bestVal = oVec[bestOptimizerIdx]->getGlobalOptimaValue();
    */

    // ---------- showoff use --------------
    switch(bestOptimizerIdx){
      case 0:
        optInfo.optimizerName = "PSO";break;
      case 1:
        optInfo.optimizerName = "AOA";break;
      case 2:
        optInfo.optimizerName = "RSA";break;
      default:
        break;
    }
    optInfo.totalPtFrm = oVec[bestOptimizerIdx]->getCumulatePointsHistory();
    optInfo.totalValHist =oVec[bestOptimizerIdx]->getValueHistory(); 
    optInfo.convergePtFrm = oVec[bestOptimizerIdx]->getGlobalConvergePointtVec(); 
    optInfo.convergeValFrm = oVec[bestOptimizerIdx]->getGlobalConvergeValueVec(); 
    // ---------- showoff use --------------

      std::cout <<"META Schedule result: \n";
    std::vector<float> actualRatio(nTask,0.0f);
    for(int i = 0; i< nTask; i++){
      std::cout << ratio[i]/200.0f + 0.5f<<",";
      actualRatio[i] = ratio[i]/200.0f + 0.5f;
    }
    std::cout <<std::endl;
    return {false, this->HEFTorder, actualRatio};
  }

} // Scheduler
} // MetaPB

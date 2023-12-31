#include "utils/ChronoTrigger.hpp"
#define BIAS_CORRECTION_REPEAT 20

namespace MetaPB {
namespace utils {

using std::vector;

ChronoTrigger::ChronoTrigger() 
: 
  pcmHandle(PCM::getInstance()),
  socketNum(pcmHandle->getNumSockets()) {

// Repeatly run tick-tock and count the bias to a report.
#pragma unroll BIAS_CORRECTION_REPEAT
  for (auto corr = 0; corr < BIAS_CORRECTION_REPEAT; corr++) {
    tick("__BIAS__");
    tock("__BIAS__");
  }
}

void ChronoTrigger::tick(const std::string &taskName) {
  // If this task haven't registered yet, allocate space and register it.
  if(!task2LastProbe.contains(taskName)){
    task2LastProbe[taskName] = Probe();
    task2LastProbe[taskName].socketStates.resize(socketNum);
  }
  Probe& thisProbe = task2LastProbe[taskName];
  //pcmHandle = PCM::getInstance();
  for (auto i = 0; i < socketNum; ++i) {
    thisProbe.socketStates[i] = pcmHandle->getSocketCounterState(i);
  }
  thisProbe.systemState = pcmHandle->getSystemCounterState();
  // Ensure the time tick is always beneath all others.
  thisProbe.time = clock::now();
}

void ChronoTrigger::tock(const std::string &taskName) {
  // State gathering, time probe first.
  const auto currTimePoint = clock::now();
  const auto currSysState = pcmHandle->getSystemCounterState();
  vector<SocketCounterState> currSocketState(socketNum);
  for (auto i = 0; i < socketNum; i++) {
    currSocketState[i] = pcmHandle->getSocketCounterState(i);
  }


  const Probe &taskLastProbe= task2LastProbe[taskName];
  if(!task2Report.contains(taskName)){
    task2Report[taskName] = Report(socketNum, metricTag::size);
  }
  Report &thisReport = task2Report[taskName];
  // Stat calculating and register.
  const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
      currTimePoint - taskLastProbe.time);
  const double thisDuration_ns = duration.count();
  thisReport.reportItems[TimeConsume_ns].addData(thisDuration_ns);

  for (auto i = 0; i < metricTag::size; ++i) {
    auto metricIdx = static_cast<metricTag>(i);
    metric2Func[metricIdx](thisReport.reportItems[i],
                           taskLastProbe.socketStates, currSocketState,
                           taskLastProbe.systemState, currSysState);
  }
}

void ChronoTrigger::dumpAllReport(std::string path){
  CSVWriter<std::string> writer;
  const std::vector<std::string> header = {
    "taskName",
    "repeat",
    "sum",
    "mean",
    "variance",
    "stdVar",
    "upperBound",
    "lowerBound",
    "upperBias",
    "lowerBias",
  };
  if(!path.ends_with("/")) path += "/";
  // Each metric corresponds to a individual file
  for(const auto& [tag, metricName]: metricNameMap){
    std::string filename = metricName + ".csv";
    vector<vector<std::string>> completeData;
    // For each metric, gather all reports' data
    for(const auto& [taskName, report] : task2Report){
      vector<vector<std::string>> items;
      const reportItem &metric = report.reportItems[tag];
        // singlular metric, one task one metric
        if(metric.data.index() == 0){
          const Stats &data = std::get<Stats>(metric.data);
          items.resize(1);
          items[0].resize(metricTag::size + 1);
          items[0] = {
            taskName,
            std::to_string(data.rep),
            std::to_string(data.sum),
            std::to_string(data.mean),
            std::to_string(data.variance),
            std::to_string(data.stdVar),
            std::to_string(data.upperBound),
            std::to_string(data.lowerBound),
            std::to_string(data.upperBias),
            std::to_string(data.lowerBias)
          };
        }else[[likely]]{ // plural metric (socket-wise)
          const vector<Stats> &datavec = 
                std::get<vector<Stats>>(metric.data);
          items.resize(datavec.size());
          for(auto i = 0; i < items.size(); ++i){
            items[i].resize(metricTag::size + 1);
            items[i] = {
              taskName + "_" + std::to_string(i),
              std::to_string(datavec[i].rep),
              std::to_string(datavec[i].sum),
              std::to_string(datavec[i].mean),
              std::to_string(datavec[i].variance),
              std::to_string(datavec[i].stdVar),
              std::to_string(datavec[i].upperBound),
              std::to_string(datavec[i].lowerBound),
              std::to_string(datavec[i].upperBias),
              std::to_string(datavec[i].lowerBias)
            };
          }
        }
      completeData.insert(completeData.end(),items.begin(),items.end());
    }
    writer.writeCSV(completeData,header, path + filename);
  }
}

} // namespace utils
} // namespace MetaPB

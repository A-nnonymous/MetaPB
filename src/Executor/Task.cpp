#include "Task/Task.hpp"

namespace MetaPB {
namespace Executor {
Task::Task(){

}
Task::Task(const string&){

}

// --------------- MetaScheduler interaction -------------
void Task::modelize(const string &modelCachePath) noexcept{

}
ScheduleVec Task::randSchedule(const size_t batchSize) const noexcept{

}
double Task::deduceTime_ns(const Schedule &) const noexcept{

}
double Task::deduceEnergy_joule(const Schedule &) const noexcept{

}


// ---------------------- Execution ----------------------
void Task::exec(const Schedule &) const noexcept{

}
void Task::execAsync(const Schedule &) const noexcept{

}

// ------------------ Getter & Setter --------------------
void Task::setTaskByTXT(const string&)noexcept{

}
set<OperatorTag> Task::getOpTagSet()const noexcept{

}
set<OperatorSign> Task::getOpSignSet()const noexcept{

}

} // namespace Executor
} // namespace MetaPB

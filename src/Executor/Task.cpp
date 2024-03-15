/*
#include "Task/Task.hpp"

namespace MetaPB {
namespace Executor {
Task::Task() {}
Task::Task(const string &) {}

// --------------- MetaScheduler interaction -------------
void Task::modelize(const string &modelCachePath) noexcept {}
ScheduleVec Task::randSchedule(const size_t batchSize) const noexcept {}
double Task::deduceTime_ns(const Schedule &) const noexcept {}
double Task::deduceEnergy_joule(const Schedule &) const noexcept {}

// ---------------------- Execution ----------------------
void Task::exec(const Schedule &) const noexcept {}
void Task::execAsync(const Schedule &) const noexcept {}

// ------------------ Getter & Setter --------------------
void Task::setTaskByTXT(const string &) noexcept {}
set<OperatorTag> Task::getOpTagSet() const noexcept {}
set<OperatorSign> Task::getOpSignSet() const noexcept {}

size_t Task::find_max_node(const std::string &filename) {
  std::ifstream file(filename.c_str());
  if (!file.is_open()) {
    std::cerr << "Unable to open file: " << filename << std::endl;
    return -1;
  }

  std::string line;
  int max_node = 0;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string start_node, end_node;
    if (!(std::getline(iss, start_node, ',') && std::getline(iss, end_node))) {
      continue; // Skip malformed lines
    }
    max_node = std::max(max_node,
                        std::max(std::stoi(start_node), std::stoi(end_node)));
  }
  return max_node;
}

static Task load_graph_from_csv(const std::string &filename) {
  std::ifstream file(filename.c_str());
  if (!file.is_open()) {
    std::cerr << "Unable to open file: " << filename << std::endl;
    return {};
  }
  int max_node = find_max_node(filename);

  Graph g(max_node);
  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string start_node, end_node;
    if (!std::getline(iss, start_node, ',')) {
      std::cerr << "Error reading start node from line: " << line << std::endl;
      continue;
    }
    if (!std::getline(iss, end_node)) {
      std::cerr << "Error reading end node from line: " << line << std::endl;
      continue;
    }
    int start_node_id = std::stoi(start_node);
    int end_node_id = std::stoi(end_node);

    add_edge(start_node_id - 1, end_node_id - 1,
             g); // Assuming nodes are 1-indexed in the CSV
  }
  return {g};
}

} // namespace Executor
} // namespace MetaPB

*/
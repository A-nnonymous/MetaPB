#ifndef BENCH
#define BENCH

#include "Executor/HeteroComputePool.hpp"
#include "Executor/TaskGraph.hpp"
#include "Scheduler/MetaScheduler.hpp"
#include "Scheduler/HEFTScheduler.hpp"
#include "Operator/OperatorManager.hpp"
#include "Operator/OperatorRegistry.hpp"
#include "omp.h"
#include "utils/Stats.hpp"
#include "utils/typedef.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <vector>

namespace benchmarks{
using execType = MetaPB::Executor::execType;
using TaskGraph = MetaPB::Executor::TaskGraph;
using TaskNode = MetaPB::Executor::TaskNode;
using Graph = MetaPB::Executor::Graph;
using TaskProperties = MetaPB::Executor::TaskProperties;
using TransferProperties = MetaPB::Executor::TransferProperties;
using OperatorType = MetaPB::Operator::OperatorType;
using OperatorTag = MetaPB::Operator::OperatorTag;
using OperatorManager = MetaPB::Operator::OperatorManager;
using MetaPB::Executor::HeteroComputePool;
using MetaPB::Operator::tag2Name;
using MetaPB::utils::regressionTask;
using MetaPB::utils::Schedule;
using perfStats = MetaPB::utils::perfStats;
} // namespace benchmarks

#endif
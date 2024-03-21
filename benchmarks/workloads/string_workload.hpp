#ifndef STRING_WL
#define STRING_WL
#include "helpers/helper.hpp"

using namespace benchmarks;
using MetaPB::Executor::Graph;
using MetaPB::Executor::TaskNode;
using MetaPB::Executor::TaskProperties;
using MetaPB::Executor::TransferProperties;
using MetaPB::Operator::OperatorTag;
using MetaPB::Operator::OperatorType;

TaskGraph genInterleavedWorkload(const size_t batchSize_MiB, int opNum) {
  TaskProperties memBound = {OperatorTag::DOT_ADD, OperatorType::Undefined,
                           batchSize_MiB, "", ""};
  TaskProperties end = {OperatorTag::LOGIC_END, OperatorType::Logical,
                           batchSize_MiB, "black", "END"};
  TransferProperties logicConnect = {1.0f, true};
  Graph g;
  auto opNode = boost::add_vertex(op, g); 
  auto endNode = boost::add_vertex(end,g); 
  boost::add_edge(opNode, endNode,logicConnect, g);
  return {g, "SINGLE_w_reduce"};
}
#endif
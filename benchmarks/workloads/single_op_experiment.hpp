#ifndef SINGLE_OP_WORKLOADS
#define SINGLE_OP_WORKLOADS

#include "helpers/helper.hpp"
using MetaPB::Executor::Graph;
using MetaPB::Executor::TaskNode;
using MetaPB::Executor::TaskProperties;
using MetaPB::Executor::TransferProperties;
using MetaPB::Operator::OperatorTag;
using MetaPB::Operator::OperatorType;

TaskGraph genSingleOp_w_reduce(const OperatorTag &opTag,
                               const size_t batchSize_MiB) {
  TaskProperties op = {opTag, OperatorType::Undefined, batchSize_MiB, "", "OP"};
  TaskProperties end = {OperatorTag::LOGIC_END, OperatorType::Logical,
                        batchSize_MiB, "black", "END"};
  TransferProperties logicConnect = {1.0f, true};
  Graph g;
  auto opNode = boost::add_vertex(op, g);
  auto endNode = boost::add_vertex(end, g);
  boost::add_edge(opNode, endNode, logicConnect, g);
  return {g, "SINGLE_w_reduce"};
}
TaskGraph genSingleOp_wo_reduce(const OperatorTag &opTag,
                                const size_t batchSize_MiB) {
  TaskProperties op = {opTag, OperatorType::Undefined, batchSize_MiB, "", ""};
  Graph g;
  auto opNode = boost::add_vertex(op, g);
  return {g, "SINGLE_wo_reduce"};
}

#endif
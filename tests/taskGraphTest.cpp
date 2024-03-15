#include "Executor/TaskGraph.hpp"
#include "Operator/OperatorRegistry.hpp"
#include <bits/stdc++.h>
#include <cmath>
#include <iostream>
#include <vector>

using namespace MetaPB::Executor;
using namespace std;
using OperatorType = MetaPB::Operator::OperatorType;
using OperatorTag = MetaPB::Operator::OperatorTag;
using MetaPB::Executor::op_property_writer;
using MetaPB::Executor::xfer_property_writer;

inline const std::string getCordinateStr(int row, int col) noexcept{
  return "[" + std::to_string(row) + "," + std::to_string(col) + "]";
}

TaskGraph genGEA(int matrixSize, size_t batchSize_MiB) {
  int N = matrixSize;
  Graph g;

  TaskProperties geaNode = {OperatorTag::MAC,
                            OperatorType::CoumputeBound, 
                            batchSize_MiB,
                            "blue", "MAC"};
  TaskProperties geaDiagonal = {OperatorTag::VM,
                            OperatorType::MemoryBound, 
                            batchSize_MiB,
                            "blue", "VM"};
                          
  TransferProperties geaTransfer = {1.0f,true};

  std::map<std::pair<int, int>, Task> rc2Task;
  for (int row = 0; row < N - 1; ++row) { // N - 1 stage
      for (int col = row; col < N; ++col) { 
        rc2Task[{row, col}] = boost::add_vertex( g);
      }
  }
  for (int row = 0; row < N - 1; ++row) { // N - 1 stage
    for (int col = row; col < N; ++col) { // N - (row + 1) operatrion each stage
      if (col == row) [[unlikely]] {      // main diagonal node.
        for (int i = col + 1; i < N; ++i) {
          if (rc2Task.contains({row, col}) && 
              rc2Task.contains({row, i})){
              g[rc2Task[{row,col}]] = geaDiagonal;
              g[rc2Task[{row,col}]].name = getCordinateStr(row,col);
              boost::add_edge(rc2Task[{row, col}], rc2Task[{row, i}], geaTransfer,
                              g);
              }
        }
      } else [[likely]] { // off-diagonal nodes.
        if (rc2Task.contains({row, col}) && 
            rc2Task.contains({row + 1, col})){
            g[rc2Task[{row,col}]] = geaNode;
            g[rc2Task[{row,col}]].name = getCordinateStr(row,col);
            boost::add_edge(rc2Task[{row, col}], rc2Task[{row + 1, col}],
                            geaTransfer, g);
        }
      }
    }
  }
  
  return {g, "GEA_" + std::to_string(N)};
}
TaskGraph genFFT(int signalLength, size_t batchSize_MiB) {
  std::uint32_t N = signalLength;
  // Bitwise magic to find next exp2, learned from a English AMDer
  N--;
  N |= N >> 1;
  N |= N >> 2;
  N |= N >> 4;
  N |= N >> 8;
  N |= N >> 16;
  N++;
  std::uint32_t log2N = std::log2(N);

  TaskProperties startNode = {OperatorTag::LOGIC_START,
                              OperatorType::Logical,
                              0,
                              "yellow", "START"};
  TaskProperties endNode = {OperatorTag::LOGIC_END,
                              OperatorType::Logical,
                              0,
                              "yellow", "END"};

  TaskProperties fftNode = {OperatorTag::MAC,
                            OperatorType::CoumputeBound,
                            4096,
                            "red", "MAC"};

  TransferProperties fftTransfer = {1.0f, true};

  TransferProperties logicDependence = {0, false};

  // log2N + 1 stage
  Graph g(N * (log2N + 1) + 2);

  for (int stage = 0; stage < log2N; ++stage) {
    int numBlocks = N / (1 << (stage + 1));
    int blockSize = 1 << (stage + 1);
    for (int block = 0; block < numBlocks; ++block) {
      for (int k = 0; k < blockSize / 2; ++k) {
        int u = block * blockSize + k + 1;
        int v = u + blockSize / 2;
        int uh, ul, vh, vl;
        uh = u + stage * N;
        ul = u + (stage + 1) * N;
        vh = v + stage * N;
        vl = v + (stage + 1) * N;
        boost::add_edge(uh, vl, fftTransfer, g);
        boost::add_edge(vh, ul, fftTransfer, g);
        boost::add_edge(uh, ul, fftTransfer, g);
        boost::add_edge(vh, vl, fftTransfer, g);
      }
    }
  }
  // 遍历所有顶点
  Graph::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    g[*vi] = fftNode;
  }

  // Universial start and end point adding.
  for (int i = 0; i < N; i++) {
    boost::add_edge(0, i + 1, g);
    boost::add_edge(N * log2N + i + 1, N * (log2N + 1) + 1, g);
  }
  g[0] = startNode;
  g[N * (log2N + 1) + 1]= endNode;

  return {g, "FFT_" + std::to_string(N)};
}

int main(){
  for(int i = 4; i <= 16; i *=2){
    TaskGraph fft = genFFT(i, 1);
    fft.printGraph("./");
  }
  for(int i = 5; i < 8; i++){
    TaskGraph gea = genGEA(i, 1);
    gea.printGraph("./");
  }
  return 0;
}
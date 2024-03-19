#include "Executor/TaskGraph.hpp"
#include <cmath>
#include <iostream>
#include <vector>

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
                              "orange", "START"};

  TaskProperties fftNode = {OperatorTag::MAC,
                            OperatorType::ComputeBound,
                            4096,
                            "red", "MAC"};

  TransferProperties fftTransfer = {batchSize_MiB * sizeof(float), true};

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
    g[*vi].name = "MAC";
  }

  // Universial start and end point adding.
  for (int i = 0; i < N; i++) {
    boost::add_edge(0, i + 1, g);
    boost::add_edge(N * log2N + i + 1, N * (log2N + 1) + 1, g);
  }
  g[0].name = "START";
  g[N * (log2N + 1) + 1].name = "END";

  std::ofstream dot_file("graph.dot");
  boost::write_graphviz(dot_file, g, vertex_property_writer(g),
                        edge_property_writer(g));

  return {g};
}
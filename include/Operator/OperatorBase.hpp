#ifndef OP_BASE_HPP
#define OP_BASE_HPP

/*
#define PREPROBE_WARMUP_REP 3
#define PROBE_REP 10
*/
#define PREPROBE_WARMUP_REP 0
#define PROBE_REP 1

#define BATCH_LOWERBOUND_MB 128
//#define PERF_SAMPLE_POINT 16
#define PERF_SAMPLE_POINT 16
#define REGRESSION_TRAINING_ITER 200
#define REGRESSION_MODEL_CACHE_PATH "/tmp/MetaPB/perfModel/"
#define DPU_ENERGY_CONSTANT_PER_SEC 280.0

#ifndef PAGE_SIZE_BYTE
#define PAGE_SIZE_BYTE 4096
#endif

#define divceil(n, m) (((n)-1) / (m) + 1)
#define roundup(n, m) ((n / m) * m + m)

#include "DPU_GLOBAL.hpp"
#include "omp.h"
#include "utils/CSVWriter.hpp"
#include "utils/ChronoTrigger.hpp"
#include "utils/Learner.hpp"
#include "utils/MetricsGather.hpp"
#include "utils/Stats.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace MetaPB {
namespace Operator {

using perfStats = utils::perfStats;
using Learner = utils::Learner;
using ChronoTrigger = utils::ChronoTrigger;

typedef struct sg_xfer_context {
  void *cpuPageBlkBaseAddr; // target cpu page block base address
  uint32_t dpuPageBaseIdx = 0;
  uint32_t pageBlkCnt = 0;
} sg_xfer_context;

typedef struct CPU_TCB {
  // Compute related metadata.
  void *src1PageBase;
  void *src2PageBase;
  void *dstPageBase;
  uint32_t pageBlkCnt = 0;
  // Scatter-Gather Xfer related metadata
  // For MAP and REDUCE only
  sg_xfer_context sgInfo;
} CPU_TCB;

typedef struct DPU_TCB {
  unsigned int src1PageIdx;
  unsigned int src2PageIdx;
  unsigned int dstPageIdx;
  unsigned int pageCnt;
  DPU_TCB &operator=(const DPU_TCB &other) {
    if (this != &other) {
      this->src1PageIdx = other.src1PageIdx;
      this->src2PageIdx = other.src2PageIdx;
      this->dstPageIdx = other.dstPageIdx;
      this->pageCnt = other.pageCnt;
    }
    return *this;
  }
} DPU_TCB;

/// @brief This class is the uniformed interface of all operator.
class OperatorBase {
public:
  OperatorBase(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR)
      : allDPUs(g_DPU_MGR->dpu_set), dpuNum(2530),
        pageBlkSize(dpuNum * PAGE_SIZE_BYTE) {
        }

  inline virtual void execCPU(const CPU_TCB &cpuTCB) const noexcept = 0;
  inline virtual void execDPU(const DPU_TCB &dpuTCB) const noexcept = 0;

  // -------------- Optimization phase utilities ----------------
  perfStats execCPUwithProbe(const CPU_TCB &cpuTCB) noexcept;
  perfStats execDPUwithProbe(const DPU_TCB &dpuTCB) noexcept;

  void trainModel(const uint32_t pageUpperBound) noexcept;

  /// @brief All our deducing has a finest granular -- page block, which
  /// consists of dpuNum x 4K Pages
  /// @param pageBlkCnt
  /// @return
  inline perfStats deducePerfCPU(const uint32_t pageBlkCnt) const noexcept {
    size_t pageBlkStep = this->deducePageBlkUpperBound / (PERF_SAMPLE_POINT -1);

    if (pageBlkCnt == 0) [[unlikely]] {
      return CPUPerfSamples[0]; // function calling overhead.
    }
    if (pageBlkCnt >= this->deducePageBlkUpperBound) [[unlikely]] {
      return CPUPerfSamples[PERF_SAMPLE_POINT];
    }

    size_t index = pageBlkCnt / pageBlkStep ;

    size_t x0 = index * pageBlkStep;
    size_t x1 = x0 + pageBlkStep;

    perfStats y0 = CPUPerfSamples[index];
    perfStats y1 = CPUPerfSamples[index + 1];

    perfStats y = y0 + (y1 - y0) * (pageBlkCnt - x0) / (x1 - x0);

    return y;
  }

  inline perfStats deducePerfDPU(const uint32_t pageBlkCnt) const noexcept {
    size_t pageBlkStep = this->deducePageBlkUpperBound / (PERF_SAMPLE_POINT - 1);

    if (pageBlkCnt == 0) [[unlikely]] {
      return DPUPerfSamples[0]; // function calling overhead.
    }
    if (pageBlkCnt >= this->deducePageBlkUpperBound) [[unlikely]] {
      return DPUPerfSamples[PERF_SAMPLE_POINT];
    }

    size_t index = pageBlkCnt / pageBlkStep ;

    size_t x0 = index * pageBlkStep;
    size_t x1 = x0 + pageBlkStep;

    perfStats y0 = DPUPerfSamples[index];
    perfStats y1 = DPUPerfSamples[index + 1];

    perfStats y = y0 + (y1 - y0) * (pageBlkCnt - x0) / (x1 - x0);
    return y;
  }

  std::string getDPUBinaryPath() const noexcept;
  inline virtual const std::string get_name() const noexcept = 0;
  inline virtual constexpr int getInputTensorNum() const noexcept = 0;
  virtual inline constexpr bool checkIfIsTrainable() const noexcept = 0;
  virtual inline constexpr bool checkIfIsCPUOnly() const noexcept = 0;
  virtual inline const bool checkIfIsTrained() const noexcept {
    return isTrained;
  }

  inline const uint32_t getPageBlkSize() const { 
    std::cout << "Returned pageBlkSize = " << dpuNum << " x " << PAGE_SIZE_BYTE << " = "<< dpuNum * PAGE_SIZE_BYTE << std::endl;
    return 2530*4096; }

protected:
  dpu_set_t &allDPUs;
  const uint32_t dpuNum;
  const uint32_t pageBlkSize;
  inline static bool get_block(struct sg_block_info *out, uint32_t dpu_index,
                               uint32_t block_index, void *args) {

    sg_xfer_context *sgArgs = (sg_xfer_context *)args;

    if (block_index >= sgArgs->pageBlkCnt) {
      return false;
    }

    out->length = PAGE_SIZE_BYTE;

    out->addr = (uint8_t *)sgArgs->cpuPageBlkBaseAddr +
                PAGE_SIZE_BYTE * (block_index * 2530 + dpu_index);
    return true;
  }

private:
  void savePerfSamples(const perfStats[],
                       const std::string &path) const noexcept;
  void loadPerfSamples(perfStats[], const std::string &path) const noexcept;
  bool loadModelCacheIfExist(const uint32_t pageUpperBound) noexcept;

  // storing all datasize to taskname
  std::map<float, std::string> cpuExecJob2Name;
  std::map<float, std::string> dpuExecJob2Name;

  size_t deducePageBlkUpperBound = 0;

  // Caches that used to store deduce results and do interpolation
  perfStats CPUPerfSamples[PERF_SAMPLE_POINT + 1]; // +1 for lowerbound
  perfStats DPUPerfSamples[PERF_SAMPLE_POINT + 1];

  inline static bool isTrained = false;

  ChronoTrigger ct;

};

} // namespace Operator
} // namespace MetaPB
#endif

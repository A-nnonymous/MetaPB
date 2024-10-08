#ifndef DPU_HPP
#define DPU_HPP
// TODOs:
// 1. Refractor the root class DPUSetOps
// 2. Purge unecessary friend relations.

#include <atomic>
#include <climits>
#include <cstdarg>
#include <cstdint>
#include <functional>
#include <ostream>
#include <string>
#include <vector>

extern "C" {
#include <dpu.h>
#include <dpu_log_internals.h>
#include <dpu_management.h>
}

/**
 * @brief Contains all that is needed to manage DPUs.
 */
namespace MetaPB {
namespace utils {
namespace DPUInterface {

/**
 * @brief Constant used to allocate all available DPUs in
 *        DpuSet::allocate and DpuSet::allocateRanks.
 */
const std::uint32_t ALLOCATE_ALL = DPU_ALLOCATE_ALL;

template <class F>
static bool __get_block(struct sg_block_info *out, uint32_t dpu_index,
                        uint32_t block_index, void *args) {
  auto f = static_cast<F *>(args);
  return (*f)(out, dpu_index, block_index);
}

/**
 * @brief Exception thrown by the methods of this module.
 */
class DpuError : public std::exception {
  friend class DpuProgram;
  friend class DpuSetOps;
  friend class DpuSet;
  friend class DpuSetAsync;

public:
  /**
   * @return Returns a C-style character string describing the general cause
   *         of the current error.
   */
  virtual const char *what() const noexcept override { return msg; }

private:
  dpu_error_t errorId;
  const char *msg;

  explicit DpuError(dpu_error_t ErrorId) : errorId(ErrorId) {
    msg = dpu_error_to_string(errorId);
  }

  ~DpuError() { free((void *)msg); }

  static void throwOnErr(dpu_error_t Error) {
    if (Error != DPU_OK) {
      throw DpuError(Error);
    }
  }
};

/**
 * @brief Representation of a symbol in a DPU program.
 */
class DpuSymbol {
  friend class DpuProgram;
  friend class DpuSetOps;

public:
  /**
   * @brief Construct DPU symbol from explicit address and size.
   * @param Address address of the DPU symbol
   * @param Size size of the DPU symbol (in bytes)
   */
  DpuSymbol(std::uint32_t Address, std::uint32_t Size)
      : cSymbol({.address = Address, .size = Size}) {}

private:
  DpuSymbol() {}
  struct dpu_symbol_t cSymbol;
};

/**
 * @brief Representation of a DPU program.
 */
class DpuProgram {
  friend class DpuSet;

public:
  /**
   * @brief Fetch the DPU symbol of the given name.
   * @param SymbolName the DPU symbol name
   * @return the DPU symbol
   * @throws DpuError when the symbol cannot be found
   */
  DpuSymbol get(const std::string &SymbolName) {
    DpuSymbol symbol;
    DpuError::throwOnErr(
        dpu_get_symbol(cProgram, SymbolName.c_str(), &symbol.cSymbol));
    return symbol;
  }

private:
  struct dpu_program_t *cProgram{nullptr};
};

class DpuSet;
class DpuSetAsync;

/**
 * @brief Function used in DpuSetAsync::call as callback.
 */
using CallbackFn = std::function<void(DpuSet &, std::uint32_t)>;

/**
 * @brief Operations on a DPU set that can be run synchronously or
 * asynchronously.
 */
class DpuSetOps {
  friend class DpuSet;
  friend class DpuSetAsync;

public:
  /**
   * @brief Copy the same data to all the DPUs in the set.
   * @param DstSymbol the name of the destination DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param SrcBuffer the source host buffer
   * @param Size the number of bytes to copy
   * @throws DpuError when the symbol does not exist or if the symbol is not big
   * enough for the data
   */
  template <typename T>
  void copy(const std::string &DstSymbol, std::uint32_t Offset,
            const std::vector<T> &SrcBuffer, size_t Size) {
    dpu_xfer_flags_t flags = async ? DPU_XFER_ASYNC : DPU_XFER_DEFAULT;
    DpuError::throwOnErr(dpu_broadcast_to(cSet, DstSymbol.c_str(), Offset,
                                          SrcBuffer.data(), Size, flags));
  }

  /**
   * @brief Copy the same data to all the DPUs in the set.
   * @param DstSymbol the name of the destination DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param SrcBuffer the source host buffer
   * @throws DpuError when the symbol does not exist or if the symbol is not big
   * enough for the data
   */
  template <typename T>
  void copy(const std::string &DstSymbol, std::uint32_t Offset,
            const std::vector<T> &SrcBuffer) {
    copy(DstSymbol, Offset, SrcBuffer, SrcBuffer.size() * sizeof(T));
  }

  /**
   * @brief Copy the same data to all the DPUs in the set.
   * @param DstSymbol the name of the destination DPU symbol
   * @param SrcBuffer the source host buffer
   * @param Size the number of bytes to copy
   * @throws DpuError when the symbol does not exist or if the symbol is not big
   * enough for the data
   */
  template <typename T>
  void copy(const std::string &DstSymbol, const std::vector<T> &SrcBuffer,
            size_t Size) {
    copy(DstSymbol, 0, SrcBuffer, Size);
  }

  /**
   * @brief Copy the same data to all the DPUs in the set.
   * @param DstSymbol the name of the destination DPU symbol
   * @param SrcBuffer the source host buffer
   * @throws DpuError when the symbol does not exist or if the symbol is not big
   * enough for the data
   */
  template <typename T>
  void copy(const std::string &DstSymbol, const std::vector<T> &SrcBuffer) {
    copy(DstSymbol, 0, SrcBuffer, SrcBuffer.size() * sizeof(T));
  }

  /**
   * @brief Copy the same data to all the DPUs in the set.
   * @param DstSymbol the destination DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param SrcBuffer the source host buffer
   * @param Size the number of bytes to copy
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(DpuSymbol &DstSymbol, std::uint32_t Offset,
            const std::vector<T> &SrcBuffer, size_t Size) {
    dpu_xfer_flags_t flags = async ? DPU_XFER_ASYNC : DPU_XFER_DEFAULT;
    DpuError::throwOnErr(dpu_broadcast_to_symbol(
        cSet, DstSymbol.cSymbol, Offset, SrcBuffer.data(), Size, flags));
  }

  /**
   * @brief Copy the same data to all the DPUs in the set.
   * @param DstSymbol the destination DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param SrcBuffer the source host buffer
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(DpuSymbol &DstSymbol, std::uint32_t Offset,
            const std::vector<T> &SrcBuffer) {
    copy(DstSymbol, Offset, SrcBuffer, SrcBuffer.size() * sizeof(T));
  }

  /**
   * @brief Copy the same data to all the DPUs in the set.
   * @param DstSymbol the destination DPU symbol
   * @param SrcBuffer the source host buffer
   * @param Size the number of bytes to copy
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(DpuSymbol &DstSymbol, const std::vector<T> &SrcBuffer,
            size_t Size) {
    copy(DstSymbol, 0, SrcBuffer, Size);
  }

  /**
   * @brief Copy the same data to all the DPUs in the set.
   * @param DstSymbol the destination DPU symbol
   * @param SrcBuffer the source host buffer
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(DpuSymbol &DstSymbol, const std::vector<T> &SrcBuffer) {
    copy(DstSymbol, 0, SrcBuffer, SrcBuffer.size() * sizeof(T));
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set.
   * @param DstSymbol the name of the destination DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param SrcBuffers the source host buffers (one per DPU in the set)
   * @param Size the number of bytes to copy
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(const std::string &DstSymbol, std::uint32_t Offset,
            const std::vector<std::vector<T>> &SrcBuffers, size_t Size) {
    struct dpu_set_t dpu;
    std::uint32_t dpuIdx;

    DPU_FOREACH(cSet, dpu, dpuIdx) {
      DpuError::throwOnErr(
          dpu_prepare_xfer(dpu, (void *)SrcBuffers[dpuIdx].data()));
    }

    dpu_xfer_flags_t flags = async ? DPU_XFER_ASYNC : DPU_XFER_DEFAULT;
    DpuError::throwOnErr(dpu_push_xfer(cSet, DPU_XFER_TO_DPU, DstSymbol.c_str(),
                                       Offset, Size, flags));
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set.
   * @param DstSymbol the name of the destination DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param SrcBuffers the source host buffers (one per DPU in the set)
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(const std::string &DstSymbol, std::uint32_t Offset,
            const std::vector<std::vector<T>> &SrcBuffers) {
    if (SrcBuffers.size() == 0) {
      DpuError::throwOnErr(DPU_ERR_INVALID_MEMORY_TRANSFER);
    }

    std::uint32_t nrElements = SrcBuffers[0].size();
    for (auto buf : SrcBuffers) {
      if (nrElements != buf.size()) {
        DpuError::throwOnErr(DPU_ERR_INVALID_MEMORY_TRANSFER);
      }
    }

    copy(DstSymbol, Offset, SrcBuffers, nrElements * sizeof(T));
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set.
   * @param DstSymbol the name of the destination DPU symbol
   * @param SrcBuffers the source host buffers (one per DPU in the set)
   * @param Size the number of bytes to copy
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(const std::string &DstSymbol,
            const std::vector<std::vector<T>> &SrcBuffers, size_t Size) {
    copy(DstSymbol, 0, SrcBuffers, Size);
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set.
   * @param DstSymbol the name of the destination DPU symbol
   * @param SrcBuffers the source host buffers (one per DPU in the set)
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(const std::string &DstSymbol,
            const std::vector<std::vector<T>> &SrcBuffers) {
    copy(DstSymbol, 0, SrcBuffers);
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set with a
   * scatter/gather transfer.
   * @param DstSymbol the name of the destination DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param get_block_info the function to get the block information
   * @param Size the number of bytes to copy
   * @param length_check check the length of the buffers
   * @throws DpuError when the symbol is not big enough for the data
   */
  void copyScatterGather(const std::string &DstSymbol, std::uint32_t Offset,
                         get_block_t &get_block_info, size_t Size,
                         bool length_check = true) {
    dpu_sg_xfer_flags_t flags = async ? DPU_SG_XFER_ASYNC : DPU_SG_XFER_DEFAULT;
    if (!length_check) {
      flags = static_cast<dpu_sg_xfer_flags_t>(
          flags | DPU_SG_XFER_DISABLE_LENGTH_CHECK);
    }
    DpuError::throwOnErr(dpu_push_sg_xfer(cSet, DPU_XFER_TO_DPU,
                                          DstSymbol.c_str(), Offset, Size,
                                          &get_block_info, flags));
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set with a
   * scatter/gather transfer.
   * @param DstSymbol the name of the destination DPU symbol
   * @param get_block_info the function to get the block information
   * @param Size the number of bytes to copy
   * @param length_check check the length of the buffer
   * @throws DpuError when the symbol is not big enough for the data
   */
  void copyScatterGather(const std::string &DstSymbol,
                         get_block_t &get_block_info, size_t Size,
                         bool length_check = true) {
    copyScatterGather(DstSymbol, 0, get_block_info, Size, length_check);
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set with a
   * scatter/gather transfer.
   * @param DstSymbol the name of the destination DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param f the function to get the block information
   * @param Size the number of bytes to copy
   * @param length_check check the length of the buffers
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <class F>
  void copyScatterGather(const std::string &DstSymbol, std::uint32_t Offset,
                         F f, size_t Size, bool length_check = true) {
    get_block_t get_block_info{__get_block<F>, &f, sizeof(f)};
    copyScatterGather(DstSymbol, Offset, get_block_info, Size, length_check);
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set with a
   * scatter/gather transfer.
   * @param DstSymbol the name of the destination DPU symbol
   * @param f the function to get the block information
   * @param Size the number of bytes to copy
   * @param length_check check the length of the buffers
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <class F>
  void copyScatterGather(const std::string &DstSymbol, F f, size_t Size,
                         bool length_check = true) {
    copyScatterGather(DstSymbol, 0, f, Size, length_check);
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set.
   * @param DstSymbol the destination DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param SrcBuffers the source host buffers (one per DPU in the set)
   * @param Size the number of bytes to copy
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(DpuSymbol &DstSymbol, std::uint32_t Offset,
            const std::vector<std::vector<T>> &SrcBuffers, size_t Size) {
    struct dpu_set_t dpu;
    std::uint32_t dpuIdx;

    DPU_FOREACH(cSet, dpu, dpuIdx) {
      DpuError::throwOnErr(
          dpu_prepare_xfer(dpu, (void *)SrcBuffers[dpuIdx].data()));
    }

    dpu_xfer_flags_t flags = async ? DPU_XFER_ASYNC : DPU_XFER_DEFAULT;
    DpuError::throwOnErr(dpu_push_xfer_symbol(
        cSet, DPU_XFER_TO_DPU, DstSymbol.cSymbol, Offset, Size, flags));
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set.
   * @param DstSymbol the destination DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param SrcBuffers the source host buffers (one per DPU in the set)
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(DpuSymbol &DstSymbol, std::uint32_t Offset,
            const std::vector<std::vector<T>> &SrcBuffers) {
    if (SrcBuffers.size() == 0) {
      DpuError::throwOnErr(DPU_ERR_INVALID_MEMORY_TRANSFER);
    }

    std::uint32_t nrElements = SrcBuffers[0].size();
    for (auto buf : SrcBuffers) {
      if (nrElements != buf.size()) {
        DpuError::throwOnErr(DPU_ERR_INVALID_MEMORY_TRANSFER);
      }
    }

    copy(DstSymbol, Offset, SrcBuffers, nrElements * sizeof(T));
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set.
   * @param DstSymbol the destination DPU symbol
   * @param SrcBuffers the source host buffers (one per DPU in the set)
   * @param Size the number of bytes to copy
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(DpuSymbol &DstSymbol, const std::vector<std::vector<T>> &SrcBuffers,
            size_t Size) {
    copy(DstSymbol, 0, SrcBuffers, Size);
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set.
   * @param DstSymbol the destination DPU symbol
   * @param SrcBuffers the source host buffers (one per DPU in the set)
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(DpuSymbol &DstSymbol,
            const std::vector<std::vector<T>> &SrcBuffers) {
    copy(DstSymbol, 0, SrcBuffers);
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set with a
   * scatter/gather transfer.
   * @param DstSymbol the destination DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param get_block_info the function to get the block information
   * @param Size the number of bytes to copy
   * @param length_check check the length of the buffers
   */
  void copyScatterGather(DpuSymbol &DstSymbol, std::uint32_t Offset,
                         get_block_t &get_block_info, size_t Size,
                         bool length_check = true) {
    dpu_sg_xfer_flags_t flags = async ? DPU_SG_XFER_ASYNC : DPU_SG_XFER_DEFAULT;
    if (!length_check) {
      flags = static_cast<dpu_sg_xfer_flags_t>(
          flags | DPU_SG_XFER_DISABLE_LENGTH_CHECK);
    }
    DpuError::throwOnErr(dpu_push_sg_xfer_symbol(cSet, DPU_XFER_TO_DPU,
                                                 DstSymbol.cSymbol, Offset,
                                                 Size, &get_block_info, flags));
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set with a
   * scatter/gather transfer.
   * @param DstSymbol the name of the destination DPU symbol
   * @param get_block_info the function to get the block information
   * @param Size the number of bytes to copy
   * @param length_check check the length of the buffer
   */
  void copyScatterGather(DpuSymbol &DstSymbol, get_block_t &get_block_info,
                         size_t Size, bool length_check = true) {
    copyScatterGather(DstSymbol, 0, get_block_info, Size, length_check);
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set with a
   * scatter/gather transfer.
   * @param DstSymbol the destination DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param f the function to get the block information
   * @param Size the number of bytes to copy
   * @param length_check check the length of the buffers
   */
  template <class F>
  void copyScatterGather(DpuSymbol &DstSymbol, std::uint32_t Offset, F f,
                         size_t Size, bool length_check = true) {
    get_block_t get_block_info{__get_block<F>, &f, sizeof(f)};
    copyScatterGather(DstSymbol, Offset, get_block_info, Size, length_check);
  }

  /**
   * @brief Copy the different buffers to the DPUs in the set with a
   * scatter/gather transfer.
   * @param DstSymbol the destination DPU symbol
   * @param f the function to get the block information
   * @param Size the number of bytes to copy
   * @param length_check check the length of the buffers
   */
  template <class F>
  void copyScatterGather(DpuSymbol &DstSymbol, F f, size_t Size,
                         bool length_check = true) {
    copyScatterGather(DstSymbol, 0, f, Size, length_check);
  }

  /**
   * @brief Copy data from the DPUs in the set.
   * @param DstBuffers the destination host buffers (one per DPU in the set)
   * @param Size the number of bytes to copy
   * @param SrcSymbol the name of the source DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(std::vector<std::vector<T>> &DstBuffers, size_t Size,
            const std::string &SrcSymbol, std::uint32_t Offset) {
    struct dpu_set_t dpu;
    std::uint32_t dpuIdx;

    DPU_FOREACH(cSet, dpu, dpuIdx) {
      DpuError::throwOnErr(dpu_prepare_xfer(dpu, DstBuffers[dpuIdx].data()));
    }

    dpu_xfer_flags_t flags = async ? DPU_XFER_ASYNC : DPU_XFER_DEFAULT;
    DpuError::throwOnErr(dpu_push_xfer(cSet, DPU_XFER_FROM_DPU,
                                       SrcSymbol.c_str(), Offset, Size, flags));
  }

  /**
   * @brief Copy data from the DPUs in the set.
   * @param DstBuffers the destination host buffers (one per DPU in the set)
   * @param Size the number of bytes to copy
   * @param SrcSymbol the name of the source DPU symbol
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(std::vector<std::vector<T>> &DstBuffers, size_t Size,
            const std::string &SrcSymbol) {
    copy(DstBuffers, Size, SrcSymbol, 0);
  }

  /**
   * @brief Copy data from the DPUs in the set.
   * @param DstBuffers the destination host buffers (one per DPU in the set)
   * @param SrcSymbol the name of the source DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(std::vector<std::vector<T>> &DstBuffers,
            const std::string &SrcSymbol, std::uint32_t Offset) {
    if (DstBuffers.size() == 0) {
      DpuError::throwOnErr(DPU_ERR_INVALID_MEMORY_TRANSFER);
    }

    std::uint32_t nrElements = DstBuffers[0].size();
    for (auto buf : DstBuffers) {
      if (nrElements != buf.size()) {
        DpuError::throwOnErr(DPU_ERR_INVALID_MEMORY_TRANSFER);
      }
    }

    copy(DstBuffers, nrElements * sizeof(T), SrcSymbol, Offset);
  }

  /**
   * @brief Copy data from the DPUs in the set.
   * @param DstBuffers the destination host buffers (one per DPU in the set)
   * @param SrcSymbol the name of the source DPU symbol
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(std::vector<std::vector<T>> &DstBuffers,
            const std::string &SrcSymbol) {
    copy(DstBuffers, SrcSymbol, 0);
  }

  /**
   * @brief Copy data from the DPUs in the set with a scatter-gather transfer.
   * @param get_block_info the callback function to get the destination buffers
   * @param Size the number of bytes to copy
   * @param SrcSymbol the name of the source DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param length_check check the length of the buffers
   * @throws DpuError when the symbol is not big enough for the data
   */
  void copyScatterGather(get_block_t &get_block_info, size_t Size,
                         const std::string &SrcSymbol, std::uint32_t Offset,
                         bool length_check = true) {
    dpu_sg_xfer_flags_t flags = async ? DPU_SG_XFER_ASYNC : DPU_SG_XFER_DEFAULT;
    if (!length_check) {
      flags = static_cast<dpu_sg_xfer_flags_t>(
          flags | DPU_SG_XFER_DISABLE_LENGTH_CHECK);
    }
    DpuError::throwOnErr(dpu_push_sg_xfer(cSet, DPU_XFER_FROM_DPU,
                                          SrcSymbol.c_str(), Offset, Size,
                                          &get_block_info, flags));
  }

  /**
   * @brief Copy data from the DPUs in the set with a scatter-gather transfer.
   * @param get_block_info the callback function to get the destination buffers
   * @param Size the number of bytes to copy
   * @param SrcSymbol the name of the source DPU symbol
   * @param length_check check the length of the buffers
   * @throws DpuError when the symbol is not big enough for the data
   */
  void copyScatterGather(get_block_t &get_block_info, size_t Size,
                         const std::string &SrcSymbol,
                         bool length_check = true) {
    copyScatterGather(get_block_info, Size, SrcSymbol, 0, length_check);
  }

  /**
   * @brief Copy data from the DPUs in the set with a scatter-gather transfer.
   * @param f the callback function to get the destination buffers
   * @param Size the number of bytes to copy
   * @param SrcSymbol the name of the source DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param length_check check the length of the buffers
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <class F>
  void copyScatterGather(F f, size_t Size, const std::string &SrcSymbol,
                         std::uint32_t Offset, bool length_check = true) {
    get_block_t get_block_info{__get_block<F>, &f, sizeof(f)};
    copyScatterGather(get_block_info, Size, SrcSymbol, Offset, length_check);
  }

  /**
   * @brief Copy data from the DPUs in the set with a scatter-gather transfer.
   * @param f the callback function to get the destination buffers
   * @param Size the number of bytes to copy
   * @param SrcSymbol the name of the source DPU symbol
   * @param length_check check the length of the buffers
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <class F>
  void copyScatterGather(F f, size_t Size, const std::string &SrcSymbol,
                         bool length_check = true) {
    copyScatterGather(f, Size, SrcSymbol, 0, length_check);
  }

  /**
   * @brief Copy data from the DPUs in the set.
   * @param DstBuffers the destination host buffers (one per DPU in the set)
   * @param Size the number of bytes to copy
   * @param SrcSymbol the source DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(std::vector<std::vector<T>> &DstBuffers, size_t Size,
            DpuSymbol &SrcSymbol, std::uint32_t Offset) {
    struct dpu_set_t dpu;
    std::uint32_t dpuIdx;

    DPU_FOREACH(cSet, dpu, dpuIdx) {
      DpuError::throwOnErr(dpu_prepare_xfer(dpu, DstBuffers[dpuIdx].data()));
    }

    dpu_xfer_flags_t flags = async ? DPU_XFER_ASYNC : DPU_XFER_DEFAULT;
    DpuError::throwOnErr(dpu_push_xfer_symbol(
        cSet, DPU_XFER_FROM_DPU, SrcSymbol.cSymbol, Offset, Size, flags));
  }

  /**
   * @brief Copy data from the DPUs in the set.
   * @param DstBuffers the destination host buffers (one per DPU in the set)
   * @param Size the number of bytes to copy
   * @param SrcSymbol the source DPU symbol
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(std::vector<std::vector<T>> &DstBuffers, size_t Size,
            DpuSymbol &SrcSymbol) {
    copy(DstBuffers, Size, SrcSymbol, 0);
  }

  /**
   * @brief Copy data from the DPUs in the set.
   * @param DstBuffers the destination host buffers (one per DPU in the set)
   * @param SrcSymbol the source DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(std::vector<std::vector<T>> &DstBuffers, DpuSymbol &SrcSymbol,
            std::uint32_t Offset) {
    if (DstBuffers.size() == 0) {
      DpuError::throwOnErr(DPU_ERR_INVALID_MEMORY_TRANSFER);
    }

    size_t nrElements = DstBuffers[0].size();
    for (auto buf : DstBuffers) {
      if (nrElements != buf.size()) {
        DpuError::throwOnErr(DPU_ERR_INVALID_MEMORY_TRANSFER);
      }
    }

    copy(DstBuffers, nrElements * sizeof(T), SrcSymbol, Offset);
  }

  /**
   * @brief Copy data from the DPUs in the set.
   * @param DstBuffers the destination host buffers (one per DPU in the set)
   * @param SrcSymbol the source DPU symbol
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <typename T>
  void copy(std::vector<std::vector<T>> &DstBuffers, DpuSymbol &SrcSymbol) {
    copy(DstBuffers, SrcSymbol, 0);
  }

  /**
   * @brief Copy data from the DPUs in the set with a scatter-gather transfer.
   * @param get_block_info the callback function to get the destination buffers
   * @param Size the number of bytes to copy
   * @param SrcSymbol the name of the source DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param length_check check the length of the buffers
   * @throws DpuError when the symbol is not big enough for the data
   */
  void copyScatterGather(get_block_t &get_block_info, size_t Size,
                         DpuSymbol &SrcSymbol, std::uint32_t Offset,
                         bool length_check = true) {
    dpu_sg_xfer_flags_t flags = async ? DPU_SG_XFER_ASYNC : DPU_SG_XFER_DEFAULT;
    if (!length_check) {
      flags = static_cast<dpu_sg_xfer_flags_t>(
          flags | DPU_SG_XFER_DISABLE_LENGTH_CHECK);
    }
    DpuError::throwOnErr(dpu_push_sg_xfer_symbol(cSet, DPU_XFER_FROM_DPU,
                                                 SrcSymbol.cSymbol, Offset,
                                                 Size, &get_block_info, flags));
  }

  /**
   * @brief Copy data from the DPUs in the set with a scatter-gather transfer.
   * @param get_block_info the callback function to get the destination buffers
   * @param Size the number of bytes to copy
   * @param SrcSymbol the name of the source DPU symbol
   * @param length_check check the length of the buffers
   * @throws DpuError when the symbol is not big enough for the data
   */
  void copyScatterGather(get_block_t &get_block_info, size_t Size,
                         DpuSymbol &SrcSymbol, bool length_check = true) {
    copyScatterGather(get_block_info, Size, SrcSymbol, 0, length_check);
  }

  /**
   * @brief Copy data from the DPUs in the set with a scatter-gather transfer.
   * @param f the callback function to get the destination buffers
   * @param Size the number of bytes to copy
   * @param SrcSymbol the name of the source DPU symbol
   * @param Offset offset from the start of the symbol where to start the copy
   * @param length_check check the length of the buffers
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <class F>
  void copyScatterGather(F f, size_t Size, DpuSymbol &SrcSymbol,
                         std::uint32_t Offset, bool length_check = true) {
    get_block_t get_block_info{__get_block<F>, &f, sizeof(f)};
    copyScatterGather(get_block_info, Size, SrcSymbol, Offset, length_check);
  }

  /**
   * @brief Copy data from the DPUs in the set with a scatter-gather transfer.
   * @param f the callback function to get the destination buffers
   * @param Size the number of bytes to copy
   * @param SrcSymbol the name of the source DPU symbol
   * @param length_check check the length of the buffers
   * @throws DpuError when the symbol is not big enough for the data
   */
  template <class F>
  void copyScatterGather(F f, size_t Size, DpuSymbol &SrcSymbol,
                         bool length_check = true) {
    copyScatterGather(f, Size, SrcSymbol, 0, length_check);
  }

  /**
   * @brief Execute a DPU program.
   * @pre The DPU program must be previously loaded with DpuSet::load.
   * @throws DpuError when the DPU execution triggers a fault.
   */
  void exec() {
    dpu_launch_policy_t policy = async ? DPU_ASYNCHRONOUS : DPU_SYNCHRONOUS;
    DpuError::throwOnErr(dpu_launch(cSet, policy));
  }

private:
  struct dpu_set_t cSet;
  bool async;

  DpuSetOps(const struct dpu_set_t &CSet, bool Async)
      : cSet(CSet), async(Async) {}
};

/**
 * @brief A set of DPUs.
 *
 * Operations on DPUs are synchronous.
 */
class DpuSet : public DpuSetOps {
  friend class DpuSetOps;
  friend class DpuSetAsync;

public:
  ~DpuSet() {
    if (manageCSet) {
      for (auto rank : _ranks) {
        delete rank;
      }
      for (auto dpu : _dpus) {
        delete dpu;
      }
      dpu_free(cSet);
    }
  }

  /**
   * @return the DPUs of the set
   */
  std::vector<DpuSet *> &dpus() { return _dpus; }

  /**
   * @return the DPU ranks of the set
   */
  std::vector<DpuSet *> &ranks() { return _ranks; }

  // ------------------- Added getter functions --------------------//
  inline const std::uint32_t getDPUNums() const noexcept {
    return _dpus.size();
  }
  inline const std::uint32_t getDPURankNums() const noexcept {
    return _ranks.size();
  }
  const std::vector<std::uint32_t> getRankWiseDPUNums() const noexcept {
    std::vector<std::uint32_t> result;
    result.reserve(_ranks.size());
    std::uint32_t temp;
    for (const auto &rank : _ranks) {
      dpu_get_nr_dpus(rank->cSet, &temp);
      result.emplace_back(temp);
    }
    return result;
  }
  // ------------------- ---------------------- --------------------//
  /**
   * @brief Allocate a number of DPUs with the given profile.
   * @param NrDpus the number of DPUs to allocate (defaults to ALLOCATE_ALL)
   * @param Profile the specific properties for the DPUs to allocate (defaults
   * to an empty profile)
   * @return the set of DPUs
   * @throws DpuError when the DPUs could not be allocated
   */
  static DpuSet allocate(std::uint32_t NrDpus = ALLOCATE_ALL,
                         const std::string &Profile = "") {
    struct dpu_set_t cSet;
    DpuError::throwOnErr(dpu_alloc(NrDpus, Profile.c_str(), &cSet));
    return DpuSet(cSet);
  }

  /**
   * @brief Allocate a number of DPU ranks with the given profile.
   * @param NrRanks the number of DPU ranks to allocate (defaults to
   * ALLOCATE_ALL)
   * @param Profile the specific properties for the DPUs to allocate (defaults
   * to an empty profile)
   * @return the set of DPUs
   * @throws DpuError when the DPUs could not be allocated
   */
  static DpuSet allocateRanks(std::uint32_t NrRanks = ALLOCATE_ALL,
                              const std::string &Profile = "") {
    struct dpu_set_t cSet;
    DpuError::throwOnErr(dpu_alloc_ranks(NrRanks, Profile.c_str(), &cSet));
    return DpuSet(cSet);
  }

  /**
   * @brief Load a DPU program on each DPU of the set.
   * @param Executable the path to the DPU program
   * @return a representation of the DPU program containing the known symbols
   * @throws DpuError when the DPU program could not be loaded
   */
  DpuProgram load(const std::string &Executable) {
    DpuProgram Program;
    DpuError::throwOnErr(dpu_load(cSet, Executable.c_str(), &Program.cProgram));
    return Program;
  }

  /**
   * @brief Display the DPU logs on the given stream.
   * @param LogStream where to display the logs
   * @throws DpuError when the DPU logs could not be fetched or displayed
   */
  void log(std::ostream &LogStream) {
    for (DpuSet *dpuSet : _dpus) {
      struct dpu_t *dpu = dpu_from_set(dpuSet->cSet);
      DpuError::throwOnErr(ostreamPrint(&LogStream, DPU_LOG_FORMAT_HEADER));
      DpuError::throwOnErr(dpulog_read_for_dpu_(dpu, ostreamPrint, &LogStream));
    }
  }

  /**
   * @return an interface of the DPU set to execute asynchronous DPU operations
   */
  DpuSetAsync async();

private:
  bool manageCSet;
  std::vector<DpuSet *> _dpus;
  std::vector<DpuSet *> _ranks;

  DpuSet(struct dpu_set_t CSet, bool ManageCSet = true,
         bool DetectChildren = true)
      : DpuSetOps(CSet, false), manageCSet(ManageCSet) {

    if (DetectChildren) {
      struct dpu_set_t cRank;
      DPU_RANK_FOREACH(CSet, cRank) {
        DpuSet *rank = new DpuSet(cRank, false, false);
        struct dpu_set_t cDpu;

        DPU_FOREACH(cRank, cDpu) {
          DpuSet *dpu = new DpuSet(cDpu, false, false);

          dpu->_dpus.push_back(dpu);
          dpu->_ranks.push_back(rank);
          rank->_dpus.push_back(dpu);
          _dpus.push_back(dpu);
        }

        rank->_ranks.push_back(rank);
        _ranks.push_back(rank);
      }
    }
  }

  static dpu_error_t ostreamPrint(void *Arg, const char *Fmt, ...) {
    std::ostream *LogStream = (std::ostream *)Arg;
    char *str;
    va_list ap;
    va_start(ap, Fmt);
    if (vasprintf(&str, Fmt, ap) == -1) {
      va_end(ap);
      return DPU_ERR_SYSTEM;
    }

    *LogStream << str;

    free(str);
    return DPU_OK;
  }
};

/**
 * @brief Interface of a DPU set for asynchronous operations.
 */
class DpuSetAsync : public DpuSetOps {
  friend class DpuSet;

  struct CallContext {
    CallbackFn callback;
    std::atomic_uint count;
  };

public:
  /**
   * @brief Call the given function on each DPU rank, or the whole set
   * @param Callback the function to be called
   * @param IsBlocking whether the other asynchronous operations
                       on the DPU rankhave to wait the callback before starting
   * @param SingleCall whether the function is called only once for the whole
   set
   * @throws DpuError when the callback could not run properly
   */
  void call(const CallbackFn &Callback, bool IsBlocking, bool SingleCall) {
    std::uint32_t flags = DPU_CALLBACK_ASYNC;
    if (!IsBlocking) {
      flags |= DPU_CALLBACK_NONBLOCKING;
    }
    if (SingleCall) {
      flags |= DPU_CALLBACK_SINGLE_CALL;
    }

    CallContext *context = new CallContext;
    context->callback = Callback;
    context->count = SingleCall ? 1 : set->_ranks.size();

    DpuError::throwOnErr(dpu_callback(cSet, cbWrapper, (void *)context,
                                      (dpu_callback_flags_t)flags));
  }

  /**
   * @brief Call the given function on each DPU rank.
   *
   * Only synchronous DPU operations are available in a callback.
   *
   * @param Callback the function to be called
   * @throws DpuError when the callback could not run properly
   */
  void call(const CallbackFn &Callback) { call(Callback, true, false); }

  /**
   * @brief Wait for the end of all queued asynchronous operations.
   * @throws DpuError when any asynchronous operation throws a DpuError
   */
  void sync() { DpuError::throwOnErr(dpu_sync(set->cSet)); }

private:
  DpuSet *set;

  explicit DpuSetAsync(DpuSet *Set) : DpuSetOps(Set->cSet, true), set(Set) {}

  static dpu_error_t cbWrapper(struct dpu_set_t CSet, std::uint32_t Idx,
                               void *Arg) {
    DpuSet dpuSet(CSet, false, true);
    CallContext *context = static_cast<CallContext *>(Arg);
    context->callback(dpuSet, Idx);
    if (--context->count == 0) {
      delete context;
    }
    return DPU_OK;
  }
};

inline DpuSetAsync DpuSet::async() { return DpuSetAsync(this); }

} // namespace DPUInterface
} // namespace utils
} // namespace MetaPB
  //
#endif

#include <alloc.h>
#include <barrier.h>
#include <defs.h>
#include <mram.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>

#include "Operator/dpu/CONV_1D.h"

__host conv_args DPU_INPUT_ARGUMENTS;

static void CONV_1D(T *bufferB, T *bufferA, T *kernel, int kernelSize,
                    int stride, unsigned int l_size) {
  size_t padding = (kernelSize - 1) / 2;
  // Perform convolution
  for (size_t i = 0; i < l_size; ++i) {
    float sum = 0.0f;
    for (size_t j = 0; j < kernelSize; ++j) {
      // Compute the input index, considering padding
      int inputIndex = i - padding + j;
      // Check if the input index is within bounds
      if (inputIndex >= 0 && inputIndex < l_size) {
        sum += bufferA[inputIndex] * kernel[j];
      }
    }
    bufferB[i] = sum;
  }
}

// Barrier
BARRIER_INIT(my_barrier, NR_TASKLETS);

int main(void) {
  unsigned int tasklet_id = me();
#if PRINT
  printf("tasklet_id = %u\n", tasklet_id);
#endif
  if (tasklet_id == 0) { // Initialize once the cycle counter
    mem_reset();         // Reset the heap
  }
  // Barrier
  barrier_wait(&my_barrier);

  uint32_t input_size_dpu_bytes =
      DPU_INPUT_ARGUMENTS.co.inputSize; // Input size per DPU in bytes
  uint32_t input_size_dpu_bytes_transfer =
      DPU_INPUT_ARGUMENTS.co.inputSize; // Transfer input size per DPU in bytes
  float kernel[8];
  int kernelSize = DPU_INPUT_ARGUMENTS.kernelSize;
  for (int i = 0; i < 8; i++) {
    kernel[i] = DPU_INPUT_ARGUMENTS.gaussianKernel[i];
  }
  int stride = DPU_INPUT_ARGUMENTS.stride;

  // Address of the current processing block in MRAM
  uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
  uint32_t mram_base_addr_A = (uint32_t)DPU_MRAM_HEAP_POINTER;
  uint32_t mram_base_addr_B =
      (uint32_t)(DPU_MRAM_HEAP_POINTER + input_size_dpu_bytes_transfer);

  // Initialize a local cache to store the MRAM block
  T *cache_A = (T *)mem_alloc(BLOCK_SIZE);
  T *cache_B = (T *)mem_alloc(BLOCK_SIZE);

  for (unsigned int byte_index = base_tasklet;
       byte_index < input_size_dpu_bytes;
       byte_index += BLOCK_SIZE * NR_TASKLETS) {

    // Bound checking
    uint32_t l_size_bytes = (byte_index + BLOCK_SIZE >= input_size_dpu_bytes)
                                ? (input_size_dpu_bytes - byte_index)
                                : BLOCK_SIZE;

    // Load cache with current MRAM block
    mram_read((__mram_ptr void const *)(mram_base_addr_A + byte_index), cache_A,
              l_size_bytes);
    mram_read((__mram_ptr void const *)(mram_base_addr_B + byte_index), cache_B,
              l_size_bytes);

    CONV_1D(cache_B, cache_A, kernel, kernelSize, stride, l_size_bytes >> DIV);

    // Write cache to current MRAM block
    mram_write(cache_B, (__mram_ptr void *)(mram_base_addr_B + byte_index),
               l_size_bytes);
  }

  return 0;
}

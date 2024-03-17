#include <alloc.h>
#include <barrier.h>
#include <defs.h>
#include <mram.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>

#include "Operator/dpu/LOOKUP.h"

__host lookup_args DPU_INPUT_ARGUMENTS;

static void LOOKUP(T *bufferA, T target, unsigned int l_size) {
  for (unsigned int i = 0; i < l_size; i++) {
    if (bufferA[i] == target)
      bufferA[i] = i; // mimic lookup, avoid handle transport
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
  float target = DPU_INPUT_ARGUMENTS.target;

  // Address of the current processing block in MRAM
  uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
  uint32_t mram_base_addr_A = (uint32_t)DPU_MRAM_HEAP_POINTER;

  // Initialize a local cache to store the MRAM block
  T *cache_A = (T *)mem_alloc(BLOCK_SIZE);

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

    LOOKUP(cache_A, target, l_size_bytes >> DIV);

    // Write cache to current MRAM block
    mram_write(cache_A, (__mram_ptr void *)(mram_base_addr_A + byte_index),
               l_size_bytes);
  }

  return 0;
}

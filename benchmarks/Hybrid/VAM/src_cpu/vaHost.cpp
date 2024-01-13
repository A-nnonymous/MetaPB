extern "C" {
#include <assert.h>
#include <dpu.h>
#include <dpu_log.h>
#include <getopt.h>
#include <omp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../support/common.h"
#include "../support/params.h"
#include "../support/timer.h"
}

#include <string>
using std::string;
// Define the DPU Binary path as DPU_BINARY here
#ifndef DPU_BINARY
#define DPU_BINARY "../dpu_bin/dpuVA"
#endif

#if ENERGY
#include <dpu_probe.h>
#endif

#include "utils/ChronoTrigger.hpp"
#include <iostream>
using MetaPB::utils::ChronoTrigger;

// Pointer declaration
static T *A;
static T *B;
static T *C;
static T *C2;

// Create input arrays
static void read_input(T *A, T *B, size_t nr_elements) {
  srand(0);
  printf("nr_elements\t%u\t", nr_elements);
#pragma omp parallel for
  for (size_t i = 0; i < nr_elements; i++) {
    A[i] = (T)(1);
    B[i] = (T)(1);
  }
}

// Compute output in the host
static void vector_addition_host(T *C, T *A, T *B, size_t nr_elements) {
#pragma omp parallel for
  for (size_t i = 0; i < nr_elements; i++) {
    C[i] = A[i] + B[i];
  }
}
static void vector_multiplication_host(T *C, T *A, T *B, size_t nr_elements) {
#pragma omp parallel for
  for (size_t i = 0; i < nr_elements; i++) {
    C[i] = A[i] * B[i];
  }
}

// Main of the Host Application
int main(int argc, char **argv) {

  ChronoTrigger ct;
  struct Params p = input_params(argc, argv);

  struct dpu_set_t dpu_set, dpu;
  uint32_t nr_of_dpus;

#if ENERGY
  struct dpu_probe_t probe;
  DPU_ASSERT(dpu_probe_init("energy_probe", &probe));
#endif

  // Allocate DPUs and load binary
  DPU_ASSERT(dpu_alloc(2048, NULL, &dpu_set));
  DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
  printf("Allocated %d DPU(s)\n", nr_of_dpus);
  size_t i = 0;
  // const size_t total_size = 8*1024*size_t(1024*1024);//32GiB int32
  const size_t total_size = 1* size_t(1024* 1024 * 1024); // NGiB int32
  A = (T *)malloc(total_size * sizeof(T));
  B = (T *)malloc(total_size * sizeof(T));
  C = (T *)malloc(total_size * sizeof(T));
  C2 = (T *)malloc(total_size * sizeof(T));
  T *bufferA = A;
  T *bufferB = B;
  T *bufferC = C2;
  /*
  const size_t input_size =
      p.exp == 0 ? p.input_size * nr_of_dpus
                 : p.input_size; // Total input size (weak or strong scaling)
  const size_t input_size_8bytes =
      ((input_size * sizeof(T)) % 8) != 0
          ? roundup(input_size, 8)
          : input_size; // Input size per DPU (max.), 8-byte aligned
  const size_t input_size_dpu =
      divceil(input_size, nr_of_dpus); // Input size per DPU (max.)
  const size_t input_size_dpu_8bytes =
      ((input_size_dpu * sizeof(T)) % 8) != 0
          ? roundup(input_size_dpu, 8)
          : input_size_dpu; // Input size per DPU (max.), 8-byte aligned

  // Input/output allocation
  A = (T*)malloc(input_size_dpu_8bytes * nr_of_dpus * sizeof(T));
  B = (T*)malloc(input_size_dpu_8bytes * nr_of_dpus * sizeof(T));
  C = (T*)malloc(input_size_dpu_8bytes * nr_of_dpus * sizeof(T));
  C2 = (T*)malloc(input_size_dpu_8bytes * nr_of_dpus * sizeof(T));
  T *bufferA = A;
  T *bufferB = B;
  T *bufferC = C2;
  */
  auto warmup = 5;
  auto reps = 10;
  std::vector<std::string> kernels = {"VA", "VM"};

  for (std::string  kernel : kernels) {
    DPU_ASSERT(dpu_load(dpu_set, ("../dpu_bin/dpu" + kernel).c_str(), NULL));
    for (size_t input_size = total_size / 4; input_size <= total_size;
         input_size <<= 1) {
      const size_t input_size_8bytes =
          ((input_size * sizeof(T)) % 8) != 0
              ? roundup(input_size, 8)
              : input_size; // Input size per DPU (max.), 8-byte aligned
      const size_t input_size_dpu =
          divceil(input_size, nr_of_dpus); // Input size per DPU (max.)
      const size_t input_size_dpu_8bytes =
          ((input_size_dpu * sizeof(T)) % 8) != 0
              ? roundup(input_size_dpu, 8)
              : input_size_dpu; // Input size per DPU (max.), 8-byte aligned
      // Create an input file with arbitrary data
      read_input(A, B, input_size);

      // Timer declaration

      printf("NR_TASKLETS\t%d\tBL\t%d\n", NR_TASKLETS, BL);

      double GiBytes = input_size * sizeof(T) / double(1024 * 1024 * 1024);
      // Loop over main kernel
      for (int rep = 0; rep < warmup + reps; rep++) {

        // Compute output on CPU (performance comparison and verification
        // purposes)
        if (rep >= warmup)
          ct.tick(kernel + "_CPU_" + std::to_string(GiBytes) + "GiBytes");
        if (kernel == "VA")
          vector_addition_host(C, A, B, input_size);
        if (kernel == "VM")
          vector_multiplication_host(C, A, B, input_size);
        if (rep >= warmup)
          ct.tock(kernel + "_CPU_" + std::to_string(GiBytes) + "GiBytes");

        printf("Load input data\n");
        if (rep >= warmup)
          ct.tick("CPU2DPU_" + std::to_string(GiBytes) + "GiBytes");
        // Input arguments
        dpu_arguments_t input_arguments[NR_DPUS];
        for (i = 0; i < nr_of_dpus - 1; i++) {
          input_arguments[i].size = input_size_dpu_8bytes * sizeof(T);
          input_arguments[i].transfer_size = input_size_dpu_8bytes * sizeof(T);
          input_arguments[i].kernel = dpu_arguments_t::kernel1;
        }
        input_arguments[nr_of_dpus - 1].size =
            (input_size_8bytes - input_size_dpu_8bytes * (NR_DPUS - 1)) *
            sizeof(T);
        input_arguments[nr_of_dpus - 1].transfer_size =
            input_size_dpu_8bytes * sizeof(T);
        input_arguments[nr_of_dpus - 1].kernel = dpu_arguments_t::kernel1;

        // Copy input arrays
        i = 0;
        DPU_FOREACH(dpu_set, dpu, i) {
          DPU_ASSERT(dpu_prepare_xfer(dpu, &input_arguments[i]));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU,
                                 "DPU_INPUT_ARGUMENTS", 0,
                                 sizeof(input_arguments[0]), DPU_XFER_DEFAULT));

        DPU_FOREACH(dpu_set, dpu, i) {
          DPU_ASSERT(
              dpu_prepare_xfer(dpu, bufferA + input_size_dpu_8bytes * i));
        }
        DPU_ASSERT(dpu_push_xfer(
            dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0,
            input_size_dpu_8bytes * sizeof(T), DPU_XFER_DEFAULT));

        DPU_FOREACH(dpu_set, dpu, i) {
          DPU_ASSERT(
              dpu_prepare_xfer(dpu, bufferB + input_size_dpu_8bytes * i));
        }
        DPU_ASSERT(
            dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME,
                          input_size_dpu_8bytes * sizeof(T),
                          input_size_dpu_8bytes * sizeof(T), DPU_XFER_DEFAULT));
        if (rep >= warmup)
          ct.tock("CPU2DPU_" + std::to_string(GiBytes) + "GiBytes");

        printf("Run program on DPU(s) \n");
        // Run DPU kernel
        if (rep >= warmup) {
          ct.tick("DPU_KERNEL_" + kernel + "_"+std::to_string(GiBytes) + "GiBytes");
        }
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        if (rep >= warmup) {
          ct.tock("DPU_KERNEL_" + kernel + "_"+std::to_string(GiBytes) + "GiBytes");
        }

#if PRINT
        {
          size_t each_dpu = 0;
          printf("Display DPU Logs\n");
          DPU_FOREACH(dpu_set, dpu) {
            printf("DPU#%d:\n", each_dpu);
            DPU_ASSERT(dpulog_read_for_dpu(dpu.dpu, stdout));
            each_dpu++;
          }
        }
#endif

        printf("Retrieve results\n");
        if (rep >= warmup)
          ct.tick("DPU2CPU_" + std::to_string(GiBytes) + "GiBytes");
        i = 0;
        // PARALLEL RETRIEVE TRANSFER
        DPU_FOREACH(dpu_set, dpu, i) {
          DPU_ASSERT(
              dpu_prepare_xfer(dpu, bufferC + input_size_dpu_8bytes * i));
        }
        DPU_ASSERT(dpu_push_xfer(
            dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME,
            input_size_dpu_8bytes * sizeof(T),
            input_size_dpu_8bytes * sizeof(T), DPU_XFER_DEFAULT));
        if (rep >= warmup)
          ct.tock("DPU2CPU_" + std::to_string(GiBytes) + "GiBytes");
      } // rep


      // Check output
      bool status = true;
      for (i = 0; i < input_size; i++) {
        if (C[i] != bufferC[i]) {
          status = false;
          //#if PRINT
          printf("%d: %u -- %u\n", i, C[i], bufferC[i]);
          //#endif
        }
      }
      if (status) {
        printf("[" ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET
               "] Outputs are equal\n");
      } else {
        printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET
               "] Outputs differ!\n");
      }

    } // input size
  }   // kernel
      ct.dumpAllReport("./");
  // Deallocation
  free(A);
  free(B);
  free(C);
  free(C2);
  DPU_ASSERT(dpu_free(dpu_set));

  return 0;
}

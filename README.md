# MetaPB: A CPU-PIM co-processing load balancer using metaheuristics algorithm.

## 1. Introduction  
  This work aims to alleviate the performance degradation issues in PIM systems caused by suboptimal scheduling. By taking into account the computational capabilities of both the CPU and PIM units, as well as the cost of data transfer between them, task scheduling is optimized from both performance and energy efficiency perspectives. This optimization enables efficient collaboration between the CPU and PIM units.
## 2. Prerequisites
This work is build and tested in real world commercial PIM computing system **UPMEM-PIM[1]** server with HW&SW environment setup below:  
### 2.1. Hardware specifications
|Category|Model|Count|Performance Metrics|Energy Consumption Metrics|
|-------|-----|-----|-------------------|--------------------------|
|CPU|Intel(R) Xeon(R) Silver 4216 (16 cores, 32 threads each)|2|2.2-3.2GHz w 22MB LLC|100W * 2|
|DRAM|Samsung M393A8G40AB2-CWE (64GB each)|4|3200 MHz|30W|
|PIM-DPU|UPMEM RISC-V DPU|2530|343-350Mhz w 64KB WRAM|100-150mW * 2530|
|PIM-DRAM|UPMEM BC021B (8GB each)|20|2400MHz|280W(include DPU)|
### 2.2. Software environment specifications
|Category|Name|Version|
|--------|----|-------|
|System|Ubuntu|22.04(Docker)|
|System|Linux kernel|5.4.0-166-generic_amd64|
|Toolchains|UPMEM SDK|2023.2.0|
|Toolchains|UPMEM driver dkms|2023.2.0|
|Toolchains|GCC|12.3.0|
|Toolchains|G++|12.3.0|
|Toolchains|CMake|3.28.0|
|Toolchains|Python3|3.10.0|
|Libraries|boost|1.74.0.3 ubuntu7 amd64|  
|Libraries|xgboost|1.5.2-1 amd64|  


Be ware that the repository stdexec requires compiler that **support C++20 standard**, in order to configurate correctly, it is adviced that the CMake version is **not less than 3.27.0** and with network access.
## 3. Overall architecture

## 4. Usage  

### 4.1. Clone this repository recursively
  ```bash
  git clone  --recursive https://github.com/A-nnonymous/MetaPB.git
  ```
### 4.2. Compile using cmake
  ```bash
  cd MetaPB && mkdir build && cd build
  cmake ../ && make -j
```

### 4.3. Run demos

  ```bash
  ./build/benchmraks/foo
  ```

## 5. References
[1] upmem

# Common utilities of all modules
add_subdirectory(utils)

# Implementation of CPU-DPU mixed Operator
add_subdirectory(Operator)

# Implementation of multistage weighted task executor
add_subdirectory(Executor)

# Implementation of main co-processing scheduler's tuning framwork
add_subdirectory(Scheduler)

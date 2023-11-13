# Common utilities of all modules
add_subdirectory(utils)

# Implementation of CPU-DPU mixed Operator
add_subdirectory(operators)

# Implementation of Operator-based Coroutine task
add_subdirectory(task)

# Implementation of multistage weighted task scheduler
add_subdirectory(scheduler)

# Implementation of main co-processing scheduler's tuning framwork
add_subdirectory(metaScheduler)


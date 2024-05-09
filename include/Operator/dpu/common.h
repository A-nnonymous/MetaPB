#ifndef COMMON_H
#define COMMON_H

#ifndef NR_TASKLETS
#define NR_TASKLETS 12
#endif
#define BLOCK_SIZE_LOG2 10
#define BLOCK_SIZE (1 << BLOCK_SIZE_LOG2)
#define BL BLOCK_SIZE_LOG2
#define DIV 2

#ifndef __cplusplus
#define T int
#endif

typedef struct {
  unsigned long long inputSize;
  unsigned long long operandNum;
} common_args;
#endif

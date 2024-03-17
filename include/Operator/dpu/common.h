#ifndef COMMON_H
#define COMMON_H
#define BLOCK_SIZE_LOG2 10
#define BLOCK_SIZE (1 << BLOCK_SIZE_LOG2)
#define BL BLOCK_SIZE_LOG2
#define DIV 2

#ifndef __cplusplus
#define T float
#endif

typedef struct {
  int inputSize;
  int operandNum;
} common_args;
#endif
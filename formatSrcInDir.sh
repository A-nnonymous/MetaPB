#!/bin/bash

# Argument check
if [ $# -ne 1 ]; then
      echo "Usage: $0 <SRC_ROOT>"
          exit 1
        fi

SRC_ROOT=$1

find $SRC_ROOT -type f \( -name *.cpp -o -name *.hpp -o -name *.h -o -name *.c \) -print0 | xargs -0 clang-format -i -style=llvm

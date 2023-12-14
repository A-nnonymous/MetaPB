find ./ -type f \( -name *.cpp -o -name *.hpp -o -name *.h -o -name *.c \) -print0 | xargs -0 clang-format -i -style=llvm

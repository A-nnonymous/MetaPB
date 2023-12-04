find /path/to/your/source/directory -type f \( -name *.cpp -o -name *.h \) -print0 | xargs -0 clang-format -i -style=file

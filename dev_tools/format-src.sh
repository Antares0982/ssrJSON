#!/usr/bin/env bash
find ./src -prune -o -type f \( -name '*.c' -o -name '*.h' \) ! -path './src/xxhash.h' ! -path './src/khash.h' -exec clang-format -i -style=file {} +
cmake-format -i CMakeLists.txt

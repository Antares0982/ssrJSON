#!/usr/bin/env bash
find ./src -path ./src/xjb -prune -o -type f \( -name '*.c' -o -name '*.h' \) ! -path './src/xxhash.h' ! -path './src/khash.h' -exec clang-format -i -style=file {} +
cmake-format -i CMakeLists.txt
cmake-format -i cmake/*

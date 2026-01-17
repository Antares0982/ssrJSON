#!/usr/bin/env bash
find ./src -path ./src/dragonbox -prune -o -type f \( -name '*.c' -o -name '*.h' \) ! -path './src/xxhash.h' -exec clang-format -i -style=file {} +

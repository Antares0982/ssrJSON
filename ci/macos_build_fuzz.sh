#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-fuzz}"

# The caller must have CC/CXX pointing to a clang with libFuzzer support
# (e.g. Homebrew LLVM) and Python3_INCLUDE_DIR/Python3_LIBRARY in the env.

SDKROOT="${SDKROOT:-$(xcrun --sdk macosx --show-sdk-path 2>/dev/null || echo)}"
LINKER_FLAGS=""
if [ -n "$SDKROOT" ] && [ -f "$SDKROOT/usr/lib/libSystem_asan.tbd" ]; then
    # Homebrew LLVM does not auto-select the ASAN variant of libSystem
    LINKER_FLAGS="-L${SDKROOT}/usr/lib -lSystem_asan"
fi

mkdir -p "$BUILD_DIR"

cmake . -B "$BUILD_DIR" \
	-DCMAKE_BUILD_TYPE=Debug \
	-DASAN_ENABLED=ON \
	-DBUILD_FUZZER=ON \
	-DSEARCH_PYTHON3_USE_ENV=ON \
	-DCMAKE_EXE_LINKER_FLAGS="$LINKER_FLAGS"

cmake --build "$BUILD_DIR" -j

mkdir -p "$BUILD_DIR/decode_corpus"
mkdir -p "$BUILD_DIR/encode_corpus"

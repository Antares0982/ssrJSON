#!/usr/bin/env bash
set -e
source dev_tools/get_env.sh

PGO_BUILD_DIR="build-pgo-instr"

if [[ $Python3_GIL_ENABLED == 1 ]]; then
	BUILD_FREE_THREADING=OFF
else
	BUILD_FREE_THREADING=ON
fi

echo "=== PGO Phase 1: Instrumented build ==="

rm -rf "$PGO_BUILD_DIR"
mkdir -p "$PGO_BUILD_DIR"
echo $CUR_PYVER >"$PGO_BUILD_DIR/pyver"

cmake . -B "$PGO_BUILD_DIR" \
	-DCMAKE_BUILD_TYPE=Release \
	-DPython3_ROOT_DIR="$Python3_ROOT_DIR" \
	-DBUILD_FREE_THREADING="$BUILD_FREE_THREADING" \
	-DBUILD_PGO_GENERATE=ON

cmake --build "$PGO_BUILD_DIR" -- -j $(nproc)

echo ""
echo "=== Instrumented build complete ==="
echo "Next: PYTHONPATH=$PGO_BUILD_DIR python ci/pgo_train.py"

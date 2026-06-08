#!/usr/bin/env bash
set -e
source dev_tools/get_env.sh

PGO_INSTR_DIR="build-pgo-instr"
PGO_DATA_DIR="$PGO_INSTR_DIR/pgo_data"
PROF_DATA="$PGO_DATA_DIR/ssrjson.profdata"

if [[ $Python3_GIL_ENABLED == 1 ]]; then
	BUILD_FREE_THREADING=OFF
else
	BUILD_FREE_THREADING=ON
fi

# Check profraw files exist
if ! ls "$PGO_DATA_DIR"/*.profraw 1>/dev/null 2>&1; then
	echo "Error: No .profraw files found in $PGO_DATA_DIR"
	echo "Run training first: PYTHONPATH=$PGO_INSTR_DIR python ci/pgo_train.py"
	exit 1
fi

echo "=== PGO Phase 2: Merging profiles ==="
llvm-profdata merge -output="$PROF_DATA" -sparse "$PGO_DATA_DIR"/*.profraw
echo "Merged profile: $PROF_DATA"

echo "=== PGO Phase 3: Optimized build ==="

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
echo $CUR_PYVER >"$BUILD_DIR/pyver"

cmake . -B "$BUILD_DIR" \
	-DCMAKE_BUILD_TYPE=Release \
	-DPython3_ROOT_DIR="$Python3_ROOT_DIR" \
	-DBUILD_FREE_THREADING="$BUILD_FREE_THREADING" \
	-DBUILD_PGO_USE="$PROF_DATA"

cmake --build "$BUILD_DIR" -- -j $(nproc)

echo ""
echo "=== PGO build complete ==="
echo "Output: $BUILD_DIR/ssrjson.so"

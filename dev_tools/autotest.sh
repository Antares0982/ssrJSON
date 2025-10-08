#!/usr/bin/env -S bash
set -e
source ./dev_tools/get_env.sh
mkdir -p $BUILD_DIR
rm -rf ./$BUILD_DIR/*

if [ -z ${TARGET_BUILD_TYPE+x} ]; then
    TARGET_BUILD_TYPE=Release
fi

./.nix-devenv/bin/cmake . -B $BUILD_DIR -DCMAKE_BUILD_TYPE=$TARGET_BUILD_TYPE -DPython3_ROOT_DIR=$Python3_ROOT_DIR -DSEARCH_PYTHON3_USE_ENV=ON
cmake --build $BUILD_DIR -- -j $(nproc)

export PYTHONPATH=$(pwd)/$BUILD_DIR
if [ -z ${SKIP_TEST+x} ]; then
    exec $Python3_EXECUTABLE -m pytest --random-order python-test
fi

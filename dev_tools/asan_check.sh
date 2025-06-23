#!/usr/bin/env -S bash
set -e
source ./dev_tools/get_env.sh
mkdir -p ./$BUILD_DIR
rm -rf ./$BUILD_DIR/*
./.nix-pyenv/bin/cmake . -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Debug -DASAN_ENABLED=on -DPython3_ROOT_DIR=$Python3_ROOT_DIR
cmake --build $BUILD_DIR --config Debug -- -j $(nproc)
export LD_PRELOAD=$(pwd)/.nix-pyenv/lib/libasan.so
set +e
$Python3_EXECUTABLE -c "exit(0)"
return_value=$?
set -e
if [ $return_value -ne 0 ]; then
    echo "This version of CPython has memory leaks, skip ASAN test"
    exit 0
fi
if [ ! -z ${SKIP_TEST+x} ]; then
    exit 0
fi
export PYTHONPATH=$(pwd)/$BUILD_DIR
exec $Python3_EXECUTABLE -m pytest --random-order python-test

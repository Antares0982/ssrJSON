#!/usr/bin/env bash

set -e
source dev_tools/get_env.sh

if [ $# -eq 0 ]; then
    cmake --build $BUILD_DIR -- -j $(nproc)
    exit 0
fi

set +e

TARGET_BUILD_TYPE=$1

y=$(cat $BUILD_DIR/pyver 2>/dev/null)
if [[ $CUR_PYVER != $y ]]; then
    echo "py ver mismatch, removing build dir"
    rm -rf $BUILD_DIR
fi

get_cmake_build_type() {
    if [ -f "./$BUILD_DIR/CMakeCache.txt" ]; then
        build_type=$(grep "CMAKE_BUILD_TYPE" ./$BUILD_DIR/CMakeCache.txt | cut -d'=' -f2)
    else
        build_type=""
    fi

    if [[ $build_type != "" && $TARGET_BUILD_TYPE != $build_type ]]; then
        echo "CMAKE_BUILD_TYPE mismatch, removing $BUILD_DIR dir"
        rm -rf $BUILD_DIR
    fi
}

get_cmake_build_type

mkdir -p $BUILD_DIR
echo $CUR_PYVER >$BUILD_DIR/pyver

cmake . -B $BUILD_DIR -DCMAKE_BUILD_TYPE=$TARGET_BUILD_TYPE -DPython3_ROOT_DIR=$Python3_ROOT_DIR
cmake --build $BUILD_DIR -- -j $(nproc)

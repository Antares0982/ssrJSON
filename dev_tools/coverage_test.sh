#!/usr/bin/env bash

set -e
source dev_tools/get_env.sh

BUILD_DIR=$BUILD_DIR-coverage

mkdir -p $BUILD_DIR

cmake . -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release -DPython3_ROOT_DIR=$Python3_ROOT_DIR -DBUILD_COVERAGE=ON
cmake --build $BUILD_DIR -- -j $(nproc)

COV_DIR=_coverage
PROFDATA=$COV_DIR/coverage.profdata
LCOVFILE=$COV_DIR/coverage.lcov
COVREPORT_DIR=$COV_DIR/report

mkdir -p $COV_DIR
export LLVM_PROFILE_FILE="$COV_DIR/python-%p.profraw"

rm -f $COV_DIR/python-*.profraw

PYTHONPATH=$BUILD_DIR pytest python-test
PYTHONPATH=$BUILD_DIR run-sde-clx pytest python-test
PYTHONPATH=$BUILD_DIR run-sde-ivb pytest python-test
PYTHONPATH=$BUILD_DIR $BUILD_DIR/ssrjson_test
PYTHONPATH=$BUILD_DIR run-sde-clx $BUILD_DIR/ssrjson_test
PYTHONPATH=$BUILD_DIR run-sde-ivb $BUILD_DIR/ssrjson_test

llvm-profdata merge -sparse $COV_DIR/python-*.profraw -o $PROFDATA

llvm-cov export $BUILD_DIR/ssrjson.so -instr-profile=$PROFDATA -format=lcov -ignore-filename-regex="/nix/store/*|xxhash.h|dragonbox" > $LCOVFILE
llvm-cov show $BUILD_DIR/ssrjson.so -instr-profile=$PROFDATA -format=html -output-dir=$COVREPORT_DIR -ignore-filename-regex="/nix/store/*|xxhash.h|dragonbox"

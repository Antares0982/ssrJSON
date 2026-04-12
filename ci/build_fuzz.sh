#!/usr/bin/env bash

set -e

BUILD_DIR=build-fuzz

mkdir -p $BUILD_DIR

cmake . -B $BUILD_DIR \
	-DCMAKE_BUILD_TYPE=Debug \
	-DASAN_ENABLED=ON \
	-DBUILD_FUZZER=ON

cmake --build $BUILD_DIR -- -j

mkdir -p $BUILD_DIR/decode_corpus
mkdir -p $BUILD_DIR/encode_corpus

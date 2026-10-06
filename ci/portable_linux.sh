#!/usr/bin/env bash
set -euo pipefail
cd /io
git config --global --add safe.directory /io
minor=${1%t}
abi=$1
export PATH="/opt/python/cp3${minor}-cp3${abi}/bin:$PATH"
if ! command -v clang >/dev/null; then
    if command -v apt-get >/dev/null; then
        apt-get update
        apt-get install -y clang lld gfortran libopenblas-dev
    else
        dnf install -y clang lld gcc-gfortran openblas-devel
    fi
fi
unset PIP_NO_CACHE_DIR
export PIP_CONFIG_FILE=/dev/null PIP_CACHE_DIR=/io/.portable-pip-cache
mkdir -p "$PIP_CACHE_DIR"
chown -R "$(id -u):$(id -g)" "$PIP_CACHE_DIR"
export CC=clang CXX=clang++
export LDFLAGS="-fuse-ld=lld"
if [[ $(uname -m) == arm* ]]; then
    export CFLAGS='-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=hard'
    export CXXFLAGS="$CFLAGS"
fi
CC=gcc CXX=g++ python -m pip install --cache-dir "$PIP_CACHE_DIR" 'cmake>=3.30' build pytest pytest-random-order psutil numpy
chmod -R a+rX "$PIP_CACHE_DIR"
python -c 'import numpy; print(numpy.__version__)'
cmake_bin=$(python -c 'import cmake; print(cmake.CMAKE_BIN_DIR)')
export PATH="$cmake_bin:$PATH"
python ci/portable_test.py "${@:2}"

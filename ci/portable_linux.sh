#!/usr/bin/env bash
set -euo pipefail
cd /io
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
export CC=clang CXX=clang++
export LDFLAGS="-fuse-ld=lld"
if [[ $(uname -m) == arm* ]]; then
    export CFLAGS='-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=hard'
    export CXXFLAGS="$CFLAGS"
fi
python -m pip install 'cmake>=3.30' build pytest pytest-random-order psutil numpy
python ci/portable_test.py "${@:2}"

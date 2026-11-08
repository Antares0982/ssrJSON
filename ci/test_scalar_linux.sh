#!/usr/bin/env bash
set -euo pipefail

if [[ $# == 2 ]]; then
    PYTHONPATH=$(realpath "$2")
    export PYTHONPATH
else
    unset PYTHONPATH
fi
"$1" -m venv --system-site-packages build-oldcpu-venv
python_bin="$PWD/build-oldcpu-venv/bin/python"
"$python_bin" -m pip install pytest pytest-random-order
CC=clang CXX=clang++ "$python_bin" -m pip install --force-reinstall --no-cache-dir --no-binary=numpy \
    --config-settings=setup-args=-Dcpu-baseline=none \
    --config-settings=setup-args=-Dcpu-dispatch=none \
    --config-settings=setup-args=-Dallow-noblas=true numpy

module=$("$python_bin" - <<'PY'
import importlib.util
from pathlib import Path

spec = importlib.util.find_spec("ssrjson")
if spec.submodule_search_locations is not None:
    spec = importlib.util.find_spec("ssrjson.ssrjson")
print(Path(spec.origin).name)
PY
)
sde=(sde64 -p4p -chip_check_image "$module" -chip_check_image ssrjson_test -chip_check_image ssrjson_test.so --)
"${sde[@]}" "$python_bin" -c 'import numpy, ssrjson; print(ssrjson.__file__, ssrjson.get_current_features()); assert ssrjson.get_current_features()["simd"] == "SCALAR"; assert ssrjson.get_current_features()["multi_lib"] is True'
"${sde[@]}" "$python_bin" -m pytest --random-order python-test \
    --ignore python-test/test_memory.py --ignore python-test/test_freethreading.py
if [[ $# == 2 ]]; then
    if [[ -x "$2/ssrjson_test" ]]; then
        "${sde[@]}" "$2/ssrjson_test"
    else
        "${sde[@]}" "$python_bin" -c 'import ctypes, sys; assert ctypes.PyDLL(sys.argv[1]).ssrjson_run_tests() == 0' "$PYTHONPATH/ssrjson_test.so"
    fi
fi

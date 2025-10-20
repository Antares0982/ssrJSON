#!/usr/bin/env bash
ARTIFACT_NAME="$1"
python -m pip install "$ARTIFACT_NAME" --prefix build-testing
python -m pip install pytest pytest-xdist pytest-random-order --prefix build-testing
export PYTHONPATH="$2"
python -m pytest --random-order python-test

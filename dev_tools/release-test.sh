#!/usr/bin/env bash
ARTIFACT_NAME="$1"
python -m pip install "$ARTIFACT_NAME" --prefix build-testing
python -m pip install -r requirements.txt --prefix build-testing
export PYTHONPATH="$2"
python -m pytest --random-order python-test

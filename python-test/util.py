# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import os
from typing import Any, Dict

import ssrjson

dirname = os.path.join(os.path.dirname(__file__), "../")

STR_CACHE: Dict[str, str] = {}

OBJ_CACHE: Dict[str, Any] = {}


def read_fixture_bytes(filename):
    target_file = os.path.join(dirname, "bench", filename)
    if os.path.exists(target_file):
        with open(target_file, "rb") as file:
            return file.read()
    target_file = os.path.join(dirname, "test_data", filename)
    if os.path.exists(target_file):
        with open(target_file, "rb") as file:
            return file.read()


def read_fixture_str(filename):
    if filename not in STR_CACHE:
        STR_CACHE[filename] = read_fixture_bytes(filename).decode("utf-8")
    return STR_CACHE[filename]


def read_fixture_obj(filename):
    if filename not in OBJ_CACHE:
        OBJ_CACHE[filename] = ssrjson.loads(read_fixture_str(filename))
    return OBJ_CACHE[filename]

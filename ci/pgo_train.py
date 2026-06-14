#!/usr/bin/env python3
"""PGO training script. Runs representative JSON workloads against an
instrumented build of ssrjson to generate profiling data."""

import argparse
import os
import sys
import json
from typing import Any


_FILE_WEIGET = {
    "apache": 10,
    "canada": 1,
    "ctm": 1,
    "github": 10,
    "instruments": 3,
    "mesh": 1,
    "truenull": 0,
    "tweet": 10,
    "twitter": 3,
    "MotionsQuestionsAnswersQuestions2016": 1,
}


class DumpsResult:
    s: list[Any]
    b: list[Any]

    def __init__(self) -> None:
        self.s = [None, None, None]
        self.b = [None, None, None]


def generate_raw_object(file: str):
    with open(file, "rb") as fh:
        raw = fh.read()
    # `raw` maybe indented or not.
    # Use json.loads to get raw object
    return json.loads(raw)


def run_dumps(obj, weight: int):
    import ssrjson

    if weight <= 0:
        return

    result = DumpsResult()

    indent_to_index = {
        None: 0,
        2: 1,
        4: 2,
    }

    for w in range(weight):
        # dumps (9 times)
        for indent in (None, 2, 4):
            for _ in range(2):
                ssrjson.dumps(obj, indent=indent)
        for indent in (None, 2, 4):
            _r = ssrjson.dumps(obj, indent=indent)
            if w == weight - 1:
                result.s[indent_to_index[indent]] = _r
            del _r

        # dumps_to_bytes (9 times)
        # no cache
        for indent in (None, 2, 4):
            ssrjson.dumps_to_bytes(obj, indent=indent)
        # write cache
        for indent in (None, 2, 4):
            ssrjson.dumps_to_bytes(obj, indent=indent, is_write_cache=True)
        # use cache
        for indent in (None, 2, 4):
            _r = ssrjson.dumps_to_bytes(obj, indent=indent, is_write_cache=False)
            if w == weight - 1:
                result.b[indent_to_index[indent]] = _r
            del _r

    return result


def run_loads(dumps_result: DumpsResult, weight: int):
    import ssrjson

    if weight <= 0:
        return

    for _ in range(weight):
        # 2 times loads minify
        for _ in range(2):
            ssrjson.loads(dumps_result.s[0])
        # 2 times loads pretty
        ssrjson.loads(dumps_result.s[1])
        ssrjson.loads(dumps_result.s[2])
        # same for bytes input
        for _ in range(2):
            ssrjson.loads(dumps_result.b[0])
        ssrjson.loads(dumps_result.b[1])
        ssrjson.loads(dumps_result.b[2])


def main() -> int:
    default_root = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description="PGO training for ssrjson")
    parser.add_argument(
        "--build-dir",
        default=os.path.join(default_root, "..", "build-pgo-instr"),
        help="Path to instrumented build directory (default: ../build-pgo-instr)",
    )
    parser.add_argument(
        "--bench-dir",
        default=os.path.join(default_root, "..", "bench"),
        help="Path to bench JSON directory (default: ../bench)",
    )
    parser.add_argument(
        "--profile-dir",
        default=None,
        help="Output directory for .profraw files (default: <build-dir>/pgo_data)",
    )
    args = parser.parse_args()

    build_dir = os.path.abspath(args.build_dir)
    bench_dir = os.path.abspath(args.bench_dir)

    pgo_data_dir = (
        os.path.abspath(args.profile_dir)
        if args.profile_dir
        else os.path.abspath(os.path.join(build_dir, "pgo_data"))
    )
    os.makedirs(pgo_data_dir, exist_ok=True)
    os.environ["LLVM_PROFILE_FILE"] = os.path.join(
        pgo_data_dir, "ssrjson_%m_%p.profraw"
    )

    json_files = sorted(
        os.path.join(bench_dir, f) for f in os.listdir(bench_dir) if f.endswith(".json")
    )
    if not json_files:
        print(f"Error: no .json files found in {bench_dir}", file=sys.stderr)
        return 1

    sys.path.insert(0, os.path.join(build_dir, "Release"))
    sys.path.insert(0, build_dir)
    os.environ["SSRJSON_WRITE_UTF8_CACHE"] = "0"
    import ssrjson  # pylint: disable=import-error

    print(f"PGO training: {len(json_files)} files from {bench_dir}")

    for fpath in json_files:
        fname = os.path.basename(fpath)
        print(f"  {fname}")
        assert fname.endswith(".json")
        weight = _FILE_WEIGET[fname[: len(fname) - 5]]
        obj = generate_raw_object(fpath)

        # loads: bytes input
        dumps_result = run_dumps(obj, weight)

        # loads: str input
        run_loads(dumps_result, weight)

    print(f"PGO training complete. Profiles in {pgo_data_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

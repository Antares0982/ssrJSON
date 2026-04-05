"""Simple benchmark: ssrjson.dumps_to_bytes vs orjson.dumps for numpy ndarray encoding."""

from __future__ import annotations

import argparse
import statistics
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import numpy as np

try:
    import ssrjson  # type: ignore
except ImportError as e:
    build_dir = (Path(__file__).resolve().parent.parent / "build").as_posix()
    if build_dir not in sys.path:
        sys.path.insert(0, build_dir)
    try:
        import ssrjson  # type: ignore
    except ImportError:
        raise SystemExit(
            "Failed to import ssrjson. If you built the extension locally, try running with PYTHONPATH=./build. "
            f"Original error: {e}"
        ) from e

ssrjson.setup_numpy_types(np)

try:
    import orjson  # type: ignore
except ImportError:
    orjson = None  # type: ignore


@dataclass(frozen=True)
class ArrayCase:
    name: str
    arr: Any  # numpy ndarray


def _make_cases(scale: int) -> list[ArrayCase]:
    """Build benchmark cases. scale multiplies the base element count (default scale=1 → 100k elements)."""
    n = 100_000 * scale
    s = int(n**0.5)  # side length for 2D cases
    c = int(round(n ** (1 / 3)))  # side length for 3D cases
    rng = np.random.default_rng(42)
    return [
        ArrayCase(f"int32_1d[{n}]", rng.integers(-1000, 1000, size=n, dtype=np.int32)),
        ArrayCase(
            f"int64_1d[{n}]", rng.integers(-(2**31), 2**31, size=n, dtype=np.int64)
        ),
        ArrayCase(f"float32_1d[{n}]", rng.random(n, dtype=np.float32)),
        ArrayCase(f"float64_1d[{n}]", rng.random(n, dtype=np.float64)),
        ArrayCase(f"float64_2d[{s}x{s}]", rng.random((s, s), dtype=np.float64)),
        ArrayCase(f"float64_3d[{c}x{c}x{c}]", rng.random((c, c, c), dtype=np.float64)),
        ArrayCase(
            f"int32_2d[{s}x{s}]", rng.integers(-9999, 9999, size=(s, s), dtype=np.int32)
        ),
        ArrayCase(f"bool_1d[{n}]", rng.integers(0, 2, size=n, dtype=np.bool_)),
    ]


def _format_seconds(s: float) -> str:
    if s < 1e-6:
        return f"{s * 1e9:.1f} ns"
    if s < 1e-3:
        return f"{s * 1e6:.1f} µs"
    if s < 1.0:
        return f"{s * 1e3:.2f} ms"
    return f"{s:.3f} s"


def _format_rate(bps: float) -> str:
    mib = bps / (1024.0 * 1024.0)
    if mib >= 1024:
        return f"{mib / 1024.0:.2f} GiB/s"
    return f"{mib:.2f} MiB/s"


def _time_fn(
    fn: Any, cases: list[ArrayCase], number: int, repeat: int, warmup: int
) -> dict[str, Any]:
    # warmup
    for _ in range(warmup):
        for c in cases:
            fn(c.arr)

    per_case_timings: dict[str, list[float]] = {c.name: [] for c in cases}

    for _ in range(repeat):
        for c in cases:
            t0 = time.perf_counter()
            for _ in range(number):
                fn(c.arr)
            t1 = time.perf_counter()
            per_case_timings[c.name].append((t1 - t0) / number)

    return {name: timings for name, timings in per_case_timings.items()}


def _print_comparison(
    case: ArrayCase,
    ssr_timings: list[float],
    orjson_timings: list[float],
    ssr_bytes: int,
    orjson_bytes: int,
) -> None:
    ssr_med = statistics.median(ssr_timings)
    ssr_best = min(ssr_timings)
    orj_med = statistics.median(orjson_timings)
    orj_best = min(orjson_timings)

    speedup_med = orj_med / ssr_med if ssr_med > 0 else float("inf")
    speedup_best = orj_best / ssr_best if ssr_best > 0 else float("inf")

    arr = case.arr
    shape_str = "x".join(str(d) for d in arr.shape)
    nbytes = arr.nbytes

    print(
        f"  {case.name}  shape={shape_str}  dtype={arr.dtype}  {nbytes / 1024:.1f} KiB raw"
    )
    print(
        f"    ssrjson  : median {_format_seconds(ssr_med)}  best {_format_seconds(ssr_best)}"
        f"  out={ssr_bytes}B  {_format_rate(ssr_bytes / ssr_med)}"
    )
    print(
        f"    orjson   : median {_format_seconds(orj_med)}  best {_format_seconds(orj_best)}"
        f"  out={orjson_bytes}B  {_format_rate(orjson_bytes / orj_med)}"
    )
    sign = "faster" if speedup_med >= 1.0 else "slower"
    ratio = speedup_med if speedup_med >= 1.0 else 1.0 / speedup_med
    print(
        f"    ssrjson is {ratio:.2f}x {sign} than orjson (median); best {speedup_best:.2f}x"
    )
    print()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Benchmark ssrjson vs orjson for numpy ndarray encoding"
    )
    parser.add_argument(
        "--number", type=int, default=20, help="Inner loop rounds per repeat"
    )
    parser.add_argument(
        "--repeat", type=int, default=7, help="Repeats (median reported)"
    )
    parser.add_argument(
        "--warmup", type=int, default=2, help="Warmup rounds (not timed)"
    )
    parser.add_argument(
        "--scale",
        type=int,
        default=1,
        help="Scale factor for array sizes (default=1 → 100k elements per case)",
    )
    parser.add_argument(
        "--filter", default="", help="Only run cases whose name contains this substring"
    )
    args = parser.parse_args(argv)

    if orjson is None:
        raise SystemExit(
            "orjson is required for this benchmark. Install it with: pip install orjson"
        )

    cases = _make_cases(args.scale)
    if args.filter:
        key = args.filter.lower()
        cases = [c for c in cases if key in c.name.lower()]
    if not cases:
        print("No cases selected.", file=sys.stderr)
        return 2

    def ssr_fn(arr: Any) -> bytes:
        return ssrjson.dumps_to_bytes(arr)

    def orj_fn(arr: Any) -> bytes:
        return orjson.dumps(arr, option=orjson.OPT_SERIALIZE_NUMPY)

    print(
        f"numpy {np.__version__}  |  scale={args.scale}  number={args.number}  repeat={args.repeat}  warmup={args.warmup}"
    )
    print(f"Python {sys.version.split()[0]}")
    print()
    print("[dumps_to_bytes: ssrjson vs orjson — numpy ndarray]")
    print()

    ssr_all = _time_fn(ssr_fn, cases, args.number, args.repeat, args.warmup)
    orj_all = _time_fn(orj_fn, cases, args.number, args.repeat, args.warmup)

    for c in cases:
        ssr_bytes = len(ssr_fn(c.arr))
        orj_bytes = len(orj_fn(c.arr))
        _print_comparison(c, ssr_all[c.name], orj_all[c.name], ssr_bytes, orj_bytes)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

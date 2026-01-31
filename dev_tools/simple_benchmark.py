from __future__ import annotations

import argparse
import json
import os
import statistics
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable

try:
    import ssrjson  # type: ignore
except ImportError as e:
    # For this repo, the built extension typically lives under ./build.
    build_dir = (Path(__file__).resolve().parent.parent / "build").as_posix()
    if build_dir not in sys.path:
        sys.path.insert(0, build_dir)
    try:
        import ssrjson  # type: ignore
    except ImportError:
        raise SystemExit(
            "Failed to import ssrjson. If you built the extension locally, try running with PYTHONPATH=./build, "
            "or run inside the project's nix develop environment. "
            f"Original error: {e}"
        ) from e

_orjson_loads: Any = None
_orjson_dumps: Any = None
try:
    import orjson  # type: ignore

    _orjson_loads = orjson.loads
    _orjson_dumps = orjson.dumps
except ImportError:  # pragma: no cover
    orjson = None

_msgspec_decode: Any = None
_msgspec_encode: Any = None
try:
    import msgspec  # type: ignore

    _msgspec_decode = msgspec.json.decode
    _msgspec_encode = msgspec.json.encode
except ImportError:  # pragma: no cover
    msgspec = None

try:
    import ujson  # type: ignore
except ImportError:  # pragma: no cover
    ujson = None


@dataclass(frozen=True)
class BenchCase:
    name: str
    text: str
    raw: bytes
    obj: Any


def _format_seconds(seconds: float) -> str:
    if seconds < 1e-6:
        return f"{seconds * 1e9:.1f} ns"
    if seconds < 1e-3:
        return f"{seconds * 1e6:.1f} µs"
    if seconds < 1.0:
        return f"{seconds * 1e3:.2f} ms"
    return f"{seconds:.3f} s"


def _format_rate(bytes_per_sec: float) -> str:
    mib = bytes_per_sec / (1024.0 * 1024.0)
    if mib >= 1024:
        return f"{mib / 1024.0:.2f} GiB/s"
    return f"{mib:.2f} MiB/s"


def _load_cases(bench_dir: Path) -> list[BenchCase]:
    cases: list[BenchCase] = []
    files = sorted(
        p for p in bench_dir.iterdir() if p.is_file() and p.suffix.lower() == ".json"
    )
    if not files:
        raise FileNotFoundError(f"No .json files found under: {bench_dir}")

    for path in files:
        raw = path.read_bytes()
        # The benchmark corpus should be UTF-8 JSON.
        text = raw.decode("utf-8")
        # Use stdlib json to build canonical Python objects once.
        obj = json.loads(text)
        cases.append(BenchCase(name=path.name, text=text, raw=raw, obj=obj))
    return cases


def _time_many(
    label: str,
    func: Callable[[BenchCase], Any],
    cases: list[BenchCase],
    number: int,
    repeat: int,
    bytes_per_round: int | None,
    warmup_rounds: int,
) -> dict[str, float]:
    _ = label
    if warmup_rounds > 0:
        sink: Any = None
        for _ in range(warmup_rounds):
            for case in cases:
                sink = func(case)
        # keep sink alive
        if sink is None and cases:
            sink = cases[0].name

    timings: list[float] = []
    sink2: Any = None
    for _ in range(repeat):
        start = time.perf_counter()
        for _ in range(number):
            for case in cases:
                sink2 = func(case)
        end = time.perf_counter()
        timings.append(end - start)
    if sink2 is None and cases:
        sink2 = cases[-1].name

    median = statistics.median(timings)
    best = min(timings)
    worst = max(timings)

    rounds = number * len(cases)
    result: dict[str, float] = {
        "median_s": median,
        "best_s": best,
        "worst_s": worst,
        "ops": float(rounds),
    }
    if bytes_per_round is not None:
        total_bytes = float(bytes_per_round) * float(number)
        result["bytes"] = total_bytes
        result["bytes_per_sec_median"] = total_bytes / median
        result["bytes_per_sec_best"] = total_bytes / best
    return result


def _print_result(label: str, r: dict[str, float], *, show_bytes: bool) -> None:
    median = r["median_s"]
    best = r["best_s"]
    worst = r["worst_s"]
    ops = r["ops"]
    ops_per_sec = ops / median

    extra = ""
    if show_bytes and "bytes_per_sec_median" in r:
        extra = f" | {_format_rate(r['bytes_per_sec_median'])} (best {_format_rate(r['bytes_per_sec_best'])})"

    print(
        f"- {label}: median {_format_seconds(median)} | best {_format_seconds(best)} | worst {_format_seconds(worst)} | "
        f"{ops_per_sec:,.0f} ops/s{extra}"
    )


def _json_dumps(obj: Any) -> str:
    # Make stdlib closer to compact JSON encoders.
    return json.dumps(obj, separators=(",", ":"), ensure_ascii=False)


def _ujson_dumps(obj: Any) -> str:
    assert ujson is not None
    try:
        return ujson.dumps(obj, ensure_ascii=False)
    except TypeError:
        # Older ujson versions may not accept ensure_ascii.
        return ujson.dumps(obj)


def _ujson_loads_text(text: str) -> Any:
    assert ujson is not None
    return ujson.loads(text)


def _ujson_loads_bytes(raw: bytes) -> Any:
    assert ujson is not None
    return ujson.loads(raw)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Simple benchmark for ssrjson vs json/ujson on ./bench/*.json"
    )
    parser.add_argument(
        "--bench-dir",
        default="./bench",
        help="Directory containing .json benchmark corpus",
    )
    parser.add_argument(
        "--number", type=int, default=5, help="Inner loop rounds per repeat"
    )
    parser.add_argument(
        "--repeat",
        type=int,
        default=7,
        help="How many repeats to run (median reported)",
    )
    parser.add_argument(
        "--warmup", type=int, default=1, help="Warmup rounds (not timed)"
    )
    parser.add_argument(
        "--filter",
        default="",
        help="Only include files containing this substring (case-insensitive); empty = all",
    )
    parser.add_argument(
        "--no-ujson",
        action="store_true",
        help="Skip ujson even if installed",
    )
    parser.add_argument(
        "--no-orjson",
        action="store_true",
        help="Skip orjson even if installed",
    )
    parser.add_argument(
        "--no-msgspec",
        action="store_true",
        help="Skip msgspec even if installed",
    )
    parser.add_argument(
        "--write-utf8-cache",
        choices=["default", "on", "off"],
        default="default",
        help="Control ssrjson.write_utf8_cache for dumps_to_bytes (default = leave as-is)",
    )
    args = parser.parse_args(argv)

    bench_dir = Path(args.bench_dir)
    cases = _load_cases(bench_dir)
    if args.filter:
        key = args.filter.lower()
        cases = [c for c in cases if key in c.name.lower()]
    if not cases:
        print("No cases selected.", file=sys.stderr)
        return 2

    if args.write_utf8_cache == "on":
        ssrjson.write_utf8_cache = True
    elif args.write_utf8_cache == "off":
        ssrjson.write_utf8_cache = False

    total_in_bytes = sum(len(c.raw) for c in cases)
    print(
        f"Cases: {len(cases)} | Total input: {total_in_bytes / (1024 * 1024):.2f} MiB"
    )
    print(f"Python: {sys.version.split()[0]} | PID: {os.getpid()}")
    enabled = ["ssrjson", "json"]
    if ujson is not None and not args.no_ujson:
        enabled.append("ujson")
    if orjson is not None and not args.no_orjson:
        enabled.append("orjson")
    if msgspec is not None and not args.no_msgspec:
        enabled.append("msgspec")
    print("Enabled: " + ", ".join(enabled))
    print(f"number={args.number} repeat={args.repeat} warmup={args.warmup}")
    print()

    # ---- loads(str) ----
    print("[loads(str)]")
    loads_str_bytes_per_round = sum(len(c.raw) for c in cases)
    results: list[tuple[str, dict[str, float]]] = []
    results.append(
        (
            "ssrjson.loads",
            _time_many(
                "ssrjson.loads(str)",
                lambda c: ssrjson.loads(c.text),
                cases,
                number=args.number,
                repeat=args.repeat,
                bytes_per_round=loads_str_bytes_per_round,
                warmup_rounds=args.warmup,
            ),
        )
    )
    results.append(
        (
            "json.loads",
            _time_many(
                "json.loads(str)",
                lambda c: json.loads(c.text),
                cases,
                number=args.number,
                repeat=args.repeat,
                bytes_per_round=loads_str_bytes_per_round,
                warmup_rounds=args.warmup,
            ),
        )
    )
    if ujson is not None and not args.no_ujson:
        results.append(
            (
                "ujson.loads",
                _time_many(
                    "ujson.loads(str)",
                    lambda c: _ujson_loads_text(c.text),
                    cases,
                    number=args.number,
                    repeat=args.repeat,
                    bytes_per_round=loads_str_bytes_per_round,
                    warmup_rounds=args.warmup,
                ),
            )
        )

    for label, r in results:
        _print_result(label, r, show_bytes=True)
    print()

    # ---- loads(bytes) ----
    print("[loads(bytes)]")
    results = []
    results.append(
        (
            "ssrjson.loads",
            _time_many(
                "ssrjson.loads(bytes)",
                lambda c: ssrjson.loads(c.raw),
                cases,
                number=args.number,
                repeat=args.repeat,
                bytes_per_round=loads_str_bytes_per_round,
                warmup_rounds=args.warmup,
            ),
        )
    )
    results.append(
        (
            "json.loads",
            _time_many(
                "json.loads(bytes)",
                lambda c: json.loads(c.raw),
                cases,
                number=args.number,
                repeat=args.repeat,
                bytes_per_round=loads_str_bytes_per_round,
                warmup_rounds=args.warmup,
            ),
        )
    )
    if ujson is not None and not args.no_ujson:
        results.append(
            (
                "ujson.loads",
                _time_many(
                    "ujson.loads(bytes)",
                    lambda c: _ujson_loads_bytes(c.raw),
                    cases,
                    number=args.number,
                    repeat=args.repeat,
                    bytes_per_round=loads_str_bytes_per_round,
                    warmup_rounds=args.warmup,
                ),
            )
        )
    if _orjson_loads is not None and not args.no_orjson:
        results.append(
            (
                "orjson.loads",
                _time_many(
                    "orjson.loads(bytes)",
                    lambda c: _orjson_loads(c.raw),
                    cases,
                    number=args.number,
                    repeat=args.repeat,
                    bytes_per_round=loads_str_bytes_per_round,
                    warmup_rounds=args.warmup,
                ),
            )
        )
    if _msgspec_decode is not None and not args.no_msgspec:
        results.append(
            (
                "msgspec.json.decode",
                _time_many(
                    "msgspec.json.decode(bytes)",
                    lambda c: _msgspec_decode(c.raw),
                    cases,
                    number=args.number,
                    repeat=args.repeat,
                    bytes_per_round=loads_str_bytes_per_round,
                    warmup_rounds=args.warmup,
                ),
            )
        )
    for label, r in results:
        _print_result(label, r, show_bytes=True)
    print()

    # ---- dumps(str) ----
    print("[dumps(str)]")
    results = []
    results.append(
        (
            "ssrjson.dumps",
            _time_many(
                "ssrjson.dumps",
                lambda c: ssrjson.dumps(c.obj),
                cases,
                number=args.number,
                repeat=args.repeat,
                bytes_per_round=None,
                warmup_rounds=args.warmup,
            ),
        )
    )
    results.append(
        (
            "json.dumps",
            _time_many(
                "json.dumps",
                lambda c: _json_dumps(c.obj),
                cases,
                number=args.number,
                repeat=args.repeat,
                bytes_per_round=None,
                warmup_rounds=args.warmup,
            ),
        )
    )
    if ujson is not None and not args.no_ujson:
        results.append(
            (
                "ujson.dumps",
                _time_many(
                    "ujson.dumps",
                    lambda c: _ujson_dumps(c.obj),
                    cases,
                    number=args.number,
                    repeat=args.repeat,
                    bytes_per_round=None,
                    warmup_rounds=args.warmup,
                ),
            )
        )
    for label, r in results:
        _print_result(label, r, show_bytes=False)
    print()

    # ---- dumps_to_bytes vs dumps+encode ----
    print("[bytes output: dumps_to_bytes vs dumps+encode('utf-8')]")
    # Precompute output bytes sizes once for throughput display.
    ssr_bytes_per_round = 0
    json_bytes_per_round = 0
    ujson_bytes_per_round = 0
    orjson_bytes_per_round = 0
    msgspec_bytes_per_round = 0
    for c in cases:
        ssr_bytes_per_round += len(ssrjson.dumps_to_bytes(c.obj))
        json_bytes_per_round += len(_json_dumps(c.obj).encode("utf-8"))
        if ujson is not None and not args.no_ujson:
            ujson_bytes_per_round += len(_ujson_dumps(c.obj).encode("utf-8"))
            if _orjson_dumps is not None and not args.no_orjson:
                orjson_bytes_per_round += len(_orjson_dumps(c.obj))
            if _msgspec_encode is not None and not args.no_msgspec:
                msgspec_bytes_per_round += len(_msgspec_encode(c.obj))

    results = []
    results.append(
        (
            "ssrjson.dumps_to_bytes",
            _time_many(
                "ssrjson.dumps_to_bytes",
                lambda c: ssrjson.dumps_to_bytes(c.obj),
                cases,
                number=args.number,
                repeat=args.repeat,
                bytes_per_round=ssr_bytes_per_round,
                warmup_rounds=args.warmup,
            ),
        )
    )
    results.append(
        (
            "json.dumps+encode",
            _time_many(
                "json.dumps+encode",
                lambda c: _json_dumps(c.obj).encode("utf-8"),
                cases,
                number=args.number,
                repeat=args.repeat,
                bytes_per_round=json_bytes_per_round,
                warmup_rounds=args.warmup,
            ),
        )
    )
    if ujson is not None and not args.no_ujson:
        results.append(
            (
                "ujson.dumps+encode",
                _time_many(
                    "ujson.dumps+encode",
                    lambda c: _ujson_dumps(c.obj).encode("utf-8"),
                    cases,
                    number=args.number,
                    repeat=args.repeat,
                    bytes_per_round=ujson_bytes_per_round,
                    warmup_rounds=args.warmup,
                ),
            )
        )
    if _orjson_dumps is not None and not args.no_orjson:
        results.append(
            (
                "orjson.dumps",
                _time_many(
                    "orjson.dumps(bytes)",
                    lambda c: _orjson_dumps(c.obj),
                    cases,
                    number=args.number,
                    repeat=args.repeat,
                    bytes_per_round=orjson_bytes_per_round,
                    warmup_rounds=args.warmup,
                ),
            )
        )
    if _msgspec_encode is not None and not args.no_msgspec:
        results.append(
            (
                "msgspec.json.encode",
                _time_many(
                    "msgspec.json.encode(bytes)",
                    lambda c: _msgspec_encode(c.obj),
                    cases,
                    number=args.number,
                    repeat=args.repeat,
                    bytes_per_round=msgspec_bytes_per_round,
                    warmup_rounds=args.warmup,
                ),
            )
        )
    for label, r in results:
        _print_result(label, r, show_bytes=True)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

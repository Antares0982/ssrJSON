"""Generate string-heavy JSON corpora for benchmarking the decoder.

The documents produced here are deliberately dominated by string content of a
single, known UTF-8 shape, so that a run of `simple_benchmark.py` over them
isolates the string decoding kernels from number/container parsing:

    python dev_tools/gen_utf8_corpus.py /tmp/corpus
    PYTHONPATH=build python dev_tools/simple_benchmark.py --bench-dir /tmp/corpus \
        --no-ujson --no-orjson --no-msgspec

Each corpus name says what it holds; the percentage is the share of ASCII
characters mixed into the non-ASCII text, which is what decides whether a
vector block is homogeneous or mixed.

`--docs` decides how much distinct text each corpus holds. It matters for the
mixed corpora: their decoding is dominated by data-dependent branches, and a
document small enough for the branch predictor to memorise reports a cost that
no real workload will see. Measured on one 8 KiB document, decoding the same
one repeatedly costs 0.46 ns/byte against 1.83 ns/byte once the pattern no
longer fits, so the default is deliberately large. `--docs 1` reproduces the
old, flattering numbers.
"""

from __future__ import annotations

import argparse
import json
import random
from pathlib import Path

# Characters per string; 8 strings per --docs unit.
STR_LEN = 4000
STR_COUNT = 8


def make_text(rng: random.Random, ascii_pct: int, lo: int, hi: int) -> str:
    out = []
    for _ in range(STR_LEN):
        if rng.randrange(100) < ascii_pct:
            c = rng.randrange(0x20, 0x7E)
            # '"' and '\\' would end the fast scan on every other byte and turn
            # the benchmark into an escape-handling benchmark instead.
            if c in (0x22, 0x5C):
                c = 0x78
            out.append(chr(c))
        else:
            out.append(chr(rng.randrange(lo, hi)))
    return "".join(out)


# name -> (ascii percentage, non-ASCII code point range)
CORPORA = {
    "ascii": (100, 0, 1),
    "latin1_50": (50, 0xC0, 0x100),
    "greek": (0, 0x400, 0x500),
    "cyrillic": (15, 0x410, 0x450),
    "cjk": (0, 0x4E00, 0x9FFF),
    "cjk_25": (25, 0x4E00, 0x9FFF),
    "cjk_50": (50, 0x4E00, 0x9FFF),
    "emoji": (0, 0x1F300, 0x1F600),
    "emoji_50": (50, 0x1F300, 0x1F600),
}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("outdir", type=Path)
    parser.add_argument("--seed", type=int, default=7)
    parser.add_argument(
        "--docs",
        type=int,
        default=8,
        help="documents' worth of distinct text per corpus file (default 8). "
        "simple_benchmark.py times one file as one case, so the text has to be "
        "distinct within the file to keep the branch predictor from learning it. "
        "Use 1 to reproduce the old corpus sizes.",
    )
    args = parser.parse_args()
    if args.docs < 1:
        parser.error("--docs must be at least 1")

    args.outdir.mkdir(parents=True, exist_ok=True)
    rng = random.Random(args.seed)
    for name, (ascii_pct, lo, hi) in CORPORA.items():
        doc = [make_text(rng, ascii_pct, lo, hi) for _ in range(STR_COUNT * args.docs)]
        path = args.outdir / f"utf8_{name}.json"
        path.write_text(json.dumps(doc, ensure_ascii=False), encoding="utf-8")
        print(f"{path} {path.stat().st_size} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Scan source files for non-ASCII characters to prevent trojan source attacks.

Checks all .c, .h, .cpp, .inl.h files under src/ (excluding vendored code),
all files under cmake/, and the top-level CMakeLists.txt.

Dangerous Unicode categories detected:
  - Bidi control characters (U+200E..U+200F, U+202A..U+202E, U+2066..U+2069)
  - Zero-width characters (U+200B..U+200D, U+2060, U+FEFF)
  - Homoglyph-prone characters (non-ASCII that looks like ASCII)
  - Any other non-ASCII character

Exit code 0 if clean, 1 if issues found.
"""

import sys
from pathlib import Path

BIDI_CONTROLS = {
    0x200E,
    0x200F,  # LRM, RLM
    0x202A,
    0x202B,
    0x202C,
    0x202D,
    0x202E,  # LRE, RLE, PDF, LRO, RLO
    0x2066,
    0x2067,
    0x2068,
    0x2069,  # LRI, RLI, FSI, PDI
}

ZERO_WIDTH = {
    0x200B,  # Zero-width space
    0x200C,  # Zero-width non-joiner
    0x200D,  # Zero-width joiner
    0x2060,  # Word joiner
    0xFEFF,  # BOM / zero-width no-break space
}

# Vendored paths to skip (relative to repo root)
VENDORED = {"src/xjb", "src/xxhash.h", "src/khash.h"}

SOURCE_EXTENSIONS = {".c", ".h", ".cpp"}


def classify_char(ch: str) -> str | None:
    cp = ord(ch)
    if cp < 128:
        return None
    if cp in BIDI_CONTROLS:
        return "BIDI CONTROL"
    if cp in ZERO_WIDTH:
        return "ZERO-WIDTH"
    return "NON-ASCII"


def scan_file(filepath: Path) -> list[tuple[int, int, str, str]]:
    """Return list of (line, col, category, repr) for non-ASCII chars."""
    findings = []
    try:
        text = filepath.read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        print(f"  WARNING: cannot read {filepath}: {e}", file=sys.stderr)
        return findings

    for lineno, line in enumerate(text.splitlines(), start=1):
        for col, ch in enumerate(line, start=1):
            category = classify_char(ch)
            if category is not None:
                findings.append((lineno, col, category, repr(ch)))
    return findings


def collect_files(repo_root: Path) -> list[Path]:
    files = []

    # src/ (excluding vendored)
    src_dir = repo_root / "src"
    if src_dir.is_dir():
        for p in sorted(src_dir.rglob("*")):
            if not p.is_file():
                continue
            rel = p.relative_to(repo_root).as_posix()
            if any(rel == v or rel.startswith(v + "/") for v in VENDORED):
                continue
            if p.suffix in SOURCE_EXTENSIONS:
                files.append(p)

    # ci/*
    ci_dir = repo_root / "ci"
    if ci_dir.is_dir():
        for p in sorted(ci_dir.glob("*")):
            if p.is_file():
                files.append(p)

    # cmake/
    cmake_dir = repo_root / "cmake"
    if cmake_dir.is_dir():
        for p in sorted(cmake_dir.rglob("*")):
            if p.is_file():
                files.append(p)

    # CMakeLists.txt
    cmakelists = repo_root / "CMakeLists.txt"
    if cmakelists.is_file():
        files.append(cmakelists)

    return files


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent
    files = collect_files(repo_root)

    total_issues = 0
    files_with_issues = 0

    for filepath in files:
        findings = scan_file(filepath)
        if findings:
            files_with_issues += 1
            rel = filepath.relative_to(repo_root)
            for lineno, col, category, char_repr in findings:
                severity = (
                    "CRITICAL"
                    if category in ("BIDI CONTROL", "ZERO-WIDTH")
                    else "WARNING"
                )
                print(
                    f"{severity}: {rel}:{lineno}:{col}: {category} character {char_repr}"
                )
                total_issues += 1

    print()
    print(f"Scanned {len(files)} files.")
    if total_issues == 0:
        print("No non-ASCII characters found. Clean.")
        return 0
    else:
        print(
            f"Found {total_issues} non-ASCII character(s) in {files_with_issues} file(s)."
        )
        # Fail only on bidi/zero-width (actual trojan source vectors)
        return 1


if __name__ == "__main__":
    sys.exit(main())

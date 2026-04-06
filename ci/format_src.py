#!/usr/bin/env python3
"""Format all source files in the repository.

Runs:
  - clang-format on C/C++ sources under src/ (excluding vendored code)
  - cmake-format on CMakeLists.txt and cmake/*
  - ruff format on Python scripts in specified directories

Usage:
  python ci/format_src.py          # format all
  python ci/format_src.py --check  # check only (exit 1 if unformatted)
"""

import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

# --- clang-format ---

CLANG_FORMAT_VENDORED = {"src/xjb", "src/xxhash.h", "src/khash.h"}
CLANG_FORMAT_EXTENSIONS = {".c", ".h", ".cpp"}


def collect_clang_format_files() -> list[Path]:
    src_dir = REPO_ROOT / "src"
    files = []
    for p in sorted(src_dir.rglob("*")):
        if not p.is_file():
            continue
        rel = p.relative_to(REPO_ROOT).as_posix()
        if any(rel == v or rel.startswith(v + "/") for v in CLANG_FORMAT_VENDORED):
            continue
        if p.suffix in CLANG_FORMAT_EXTENSIONS:
            files.append(p)
    return files


# --- cmake-format ---


def collect_cmake_format_files() -> list[Path]:
    files = []
    cmakelists = REPO_ROOT / "CMakeLists.txt"
    if cmakelists.is_file():
        files.append(cmakelists)
    cmake_dir = REPO_ROOT / "cmake"
    if cmake_dir.is_dir():
        for p in sorted(cmake_dir.rglob("*")):
            if p.is_file():
                files.append(p)
    return files


# --- ruff ---

RUFF_PATHS = [
    "dev_tools/**/*.py",
    "ci/**/*.py",
    "./*.py",
    "test_data/**/*.py",
    "python-test/**/*.py",
    "pysrc/**/*.py",
    "pysrc/**/*.pyi",
]


def collect_ruff_files() -> list[Path]:
    files = []
    for pattern in RUFF_PATHS:
        for p in sorted(REPO_ROOT.glob(pattern)):
            if p.is_file():
                files.append(p)
    return files


# --- runner ---


def run_formatter(name: str, cmd: list[str], files: list[Path]) -> bool:
    """Run a formatter on a list of files. Returns True if successful."""
    if not files:
        return True
    rel_files = [str(f.relative_to(REPO_ROOT)) for f in files]
    print(f"[{name}] Formatting {len(rel_files)} file(s)...")
    result = subprocess.run(cmd + rel_files, cwd=REPO_ROOT)
    if result.returncode != 0:
        print(f"[{name}] FAILED (exit code {result.returncode})", file=sys.stderr)
        return False
    return True


def main() -> int:
    check_mode = "--check" in sys.argv

    ok = True

    # clang-format
    c_files = collect_clang_format_files()
    ok &= run_formatter("clang-format", ["clang-format", "-i", "-style=file"], c_files)

    # cmake-format
    cmake_files = collect_cmake_format_files()
    ok &= run_formatter("cmake-format", ["cmake-format", "-i"], cmake_files)

    # ruff format
    ruff_files = collect_ruff_files()
    ok &= run_formatter("ruff", ["ruff", "format"], ruff_files)

    if not ok:
        return 1

    if check_mode:
        result = subprocess.run(
            ["git", "diff", "--name-only"],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
        )
        changed = result.stdout.strip()
        if changed:
            print()
            print("The following files are not properly formatted:")
            print(changed)
            return 1
        print("All files are properly formatted.")

    return 0


if __name__ == "__main__":
    sys.exit(main())

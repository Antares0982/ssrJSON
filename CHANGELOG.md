# Changelog

## 0.0.26

### Python Compatibility

- Drop CPython 3.10 support. CPython 3.11 is now the minimum supported version (#47)
- Support CPython 3.16 prereleases from source, including free-threaded builds. Release wheels cover CPython 3.11 through 3.15 (#47)

## 0.0.25

### New Features

- Add automatic scalar fallback on x86-64 CPUs without SSE4.2 (#42)
- Support Windows x86, Linux i686, and little-endian Linux ARMv7 hard-float; ARMv7 does not require NEON
- Add 32-bit wheels: Linux i686 requires SSE2 and glibc 2.34+; ARMv7 requires glibc 2.35+

### API Changes

- AArch64 builds use a single NEON backend by default, ignore `BUILD_SHIPPING_SIMD`, and report `get_current_features()["multi_lib"]` as `False`; `BUILD_SCALAR` explicitly selects scalar
- `get_current_features()["simd"]` reports `"SCALAR"` when the scalar backend is active

### Bug Fixes

- Raise `MemoryError` on size overflow in encoder buffer reservations, NumPy array sizing, and decoder buffer growth
- Handle allocation failure when decoding a top-level JSON string
- Correct overlapping Unicode widening on ARMv7 (#42)
- Fix NumPy scalar alignment and CPython 3.12/3.13 immortal-object checks on 32-bit builds
- Prevent decoder buffer corruption in reentrant `loads()` calls from hooks or garbage-collection finalizers (#45)
- Safely decode non-compact `str` input, including `str` subclasses (#45)
- Avoid encoder reentrancy through NumPy ndarray subclass overrides and structured-dtype callbacks (#45)
- Keep shared module state alive across re-imports and while exported functions remain referenced (#45)
- Match UTF-8 cache allocation to CPython's allocator to prevent mismatched frees (#45)
- Initialize the hash cache of `dumps_to_bytes()` output so hashing and dict/set lookups work correctly (#45)
- Fix list/dict size races during encoding and bytearray resize races during decoding in free-threaded builds with locking enabled (#45)
- Report character positions for errors in root-level `str` strings and the end of input for unterminated strings (#45)

### Performance

- Use single-pass scalar string scans that support compiler auto-vectorization
- Reduce AArch64 allocation-check overhead with conservative string-size bounds
- Defer Unicode kind extraction to non-ASCII paths in `dumps_to_bytes`
- Convert Python integers that fit in `int64_t` or `uint64_t` inline when encoding on CPython builds with 30-bit integer digits, including CPython 3.10/3.11 (#46)
- Write integers of 9 or more digits with SSSE3 on x86-64 SIMD backends (#46)
- Write indentation one nesting level per store, so clang PGO builds no longer use slow AVX-512 scatters for UCS4 output (#46)

### Build & CI

- Add `BUILD_SCALAR` for explicit scalar builds and select the CMake platform matching Python on Windows
- Allow non-Windows extension builds without an embeddable libpython and run C tests through Python when it is unavailable
- Add 32-bit build, test, and release coverage, plus old-CPU scalar fallback checks for x86-64 Linux and Windows wheels
- Include scalar in x86-64 PGO training and test explicit scalar builds on AArch64
- Update portable CI actions for Node.js 24 and refresh dependency caches between runs
- Add free-threaded Python to the regular portable CI matrix
- Move AArch64 release jobs and ARMv7 portable tests to GitHub-hosted ARM64 runners; parallelize ARMv7 tests and avoid duplicate portable package builds
- Expand decode fuzzing to hooks, bytearray input, and `str` subclasses; fix allocator setup for free-threaded ASAN runs (#45)
- Run Linux fuzzers without Nix, disable Cachix for non-dry-run releases, and update the Nix development flake lock

### Contributors

- [@Nambers](https://github.com/Nambers) co-authored scalar fallback and portable-platform support ([#42](https://github.com/Antares0982/ssrJSON/pull/42)), contributed memory-safety and correctness fixes plus decode fuzzing improvements ([#45](https://github.com/Antares0982/ssrJSON/pull/45)), and optimized integer encoding and indentation ([#46](https://github.com/Antares0982/ssrJSON/pull/46))

## 0.0.24

No code change since 0.0.23.

### Build & CI
- Build Python 3.15(t) wheels
- Support lock version using git SHA

## 0.0.23

### New Features
- Add `array_hook` to `loads` to transform decoded arrays, including nested arrays; supports use together with `object_hook` (#34)

### Bug Fixes
- Fix invalid memory reads when encoding short `str` subclasses and short UTF-8 caches with `PYTHONMALLOC=malloc`

### Performance
- Optimize dictionary iteration for combined Unicode-key dictionaries on CPython 3.11-3.15 GIL builds
- Optimize AVX2 tail copies with branchless overlapping stores, reuse byte tail escape bitmasks, and move escape retries out of the hot path
- Move NumPy array and non-compact string bytes encoding out of the hot path
- Improve UTF-8 encoding of UCS4 strings containing characters that require three UTF-8 bytes

### Build & CI
- Allow `ENCODE_RESERVE_DEBUG` to be defined through build flags

## 0.0.22

### Performance
- Replace old scalar bytes (UTF-8) decoder with SIMD-accelerated UTF-8 decoder. Proved to be faster on both x86-64 and aarch64-apple (#24)

### Documentation
- Add Chinese README

## 0.0.21

### Bug Fixes
- Fix 1-byte heap overflow in xjb float-to-string when `XJB_NO_MEMMOVE` is 0: the buffer requirement was under-counted by one byte
- Fix 1-byte heap overflow when encoding numpy arrays containing extreme float values

### Performance
- Enable `no_memmove` in xjb for generic aarch64 NEON
- Mitigate a pow10 table load regression on Zen4 under `-fPIC` in xjb (up to ~1.85x slowdown on the double path); enabled for all x86-64 PIC builds since Zen4 cannot be detected, costing ~2% on Intel
- Remove Nix hardening flags (`fortify`, `zerocallusedregs`, `libcxxhardeningfast`) that degrade codegen on hot paths in wheel builds

### Code Quality
- Fix NEON intrinsic type punning in xjb, add debug-only buffer bounds checks, and remove dead code

### Build & CI
- Fix release workflow: suppress `-Wprofile-instr-out-of-date` in PGO builds
- Reimplement the encode fuzzer input generator in C, replacing the Python generator and fuzzer dictionary

## 0.0.20

### API Changes
- `ssrjson.get_current_features()["pgo"]` now returns whether the current build is PGO-optimized (#36)

### Bug Fixes
- Fix 1-byte heap overflow when writing numpy i8 in range -128 ~ -100 (#35)

### Performance
- Add PGO (Profile-Guided Optimization) support: wheel builds use PGO by default; installing from tarball requires `SSRJSON_ENABLE_PGO=1` environment variable and LLVM toolchain (#36)
- Remove loop4 from encode SIMD kernels to simplify code and improve runtime performance (#37)

## 0.0.19

### New Features
- Add non-compact unicode encoder to support subclasses of Python str (#33)

### Bug Fixes
- Fix aarch64 NEON encoder heap buffer overflow in `encode_trailing_copy_with_cvt` - all prior versions recommended to upgrade
- Fix stack buffer overflow related to xjb32

### Performance
- Implement `no_memmove` feature in xjb for improved float-to-string performance [xjb#7](https://github.com/xjb714/xjb/pull/7)

### Code Quality
- Refactor key/str writer, reduce binary size, and remove magic numbers

### Build & CI
- Use trusted publisher for PyPI
- Add aarch64-MacOS ASAN tests and fuzzer

## 0.0.18

### Bug Fixes
- Fix wrong key created using dirty memory when object contains UCS(X) and UCS(Y) strings (X>Y) and byte size of UCS(Y) string is exactly 64
- Fix crash when list subclass contains string that elevates UCS type

### Performance
- Update xjb to 1.5.0

## 0.0.17

### New Features
- Support encoding numpy types (#30)

### API Changes
- `multi_lib` can only be true in x86-64 now

### Bug Fixes
- Fix potential race condition in free-threaded cache writing
- Fix potential stack overflow
- Fix PyErr not set correctly when realloc/malloc fails

### Performance
- Enable LTO by default
- Improve branch prediction in dumps `str`
- Optimize decode, add compile-time related macros
- Remove branch when writing bool, pass immediate number when writing first container
- Remove two redundant branches when writing integer
- Use pymem allocator
- Update xjb
- Strip binary

### Build & CI
- MacOS wheel is also built by Nix now
- Darwin compatibility fix
- Add compiler check
- Rename `ci_tools/` to `ci/`
- Fix Node.js 20 deprecation warning in CI
- Add Python format check and non-ASCII check

### Code Quality
- Rename `COMPILE_SIMD_BITS` to `_CompileVectorBits`
- Rename platform macros to `SSRJSON_IS_X64` / `SSRJSON_IS_AARCH64`
- Remove unused functions
- Move xxhash and khash includes to `ssrjson.h`
- Adjust inline/noinline annotations
- Cleanup trailing-copy code

## 0.0.16

Initial tracked release.

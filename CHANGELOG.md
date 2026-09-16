# Changelog

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

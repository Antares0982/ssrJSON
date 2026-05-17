# Changelog

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

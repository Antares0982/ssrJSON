# Changelog

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

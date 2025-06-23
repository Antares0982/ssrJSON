# ssrJSON

ssrJSON is a Python JSON library that leverages modern hardware capabilities to achieve peak performance, implemented primarily in C with some components written in C++. It offers a fully compatible interface to Python’s standard `json` module, making it a seamless drop-in replacement, while providing exceptional performance for JSON encoding and decoding.

`ssrjson.dumps()` is about 4x-26x as fast as `json.dumps()` (Python3.13, x86-64, AVX2). `ssrjson.loads()` is about 2x-8x as fast as `json.loads()` for `str` input and is about 2x-8x as fast as `json.loads()` for `bytes` input (Python3.13, x86-64, AVX2). ssrJSON also provides `ssrjson.dumps_to_bytes()`, which encode Python objects directly to `bytes` object using SIMD instructions, similar to `orjson.dumps` but without calling slow CPython functions to do the UTF-8 encoding. ssrJSON is faster than or nearly as fast as [orjson](https://github.com/ijl/orjson) on most benchmark cases, which means ssrJSON is the world's fastest Python JSON library at now. Typically, ssrJSON is capable of processing non-ASCII strings directly without invoking any slow CPython UTF-8 encoding and decoding interfaces, eliminating the need for intermediate representations. Furthermore, the underlying implementation leverages SIMD acceleration to optimize this process. Details of benchmarking can be found in the [benchmark repository](https://github.com/Nambers/ssrJSON-benchmark). Implementation details can be found in [Implementation Details](#implementation-details) section.

The design goal of ssrJSON is to provide a straightforward and highly compatible approach to replace the inherently slower Python standard JSON encoding and decoding implementation with a significantly more efficient and high-performance alternative. If your module exclusively utilizes `dumps` and `loads`, you can replace the current JSON implementation by importing ssrJSON as `import ssrjson as json`. To facilitate this, ssrJSON maintains compatibility with the argument formats of `json.dumps` and `json.loads`; however, it does not guarantee identical results to the standard JSON module, as many features are either not yet supported or intentionally omitted. For further information, please refer to the section [Implementation Details](#implementation-details).

The development of ssrJSON is still actively ongoing, and some features have yet to be supported. Your code contributions are highly appreciated.

## How To Install

ssrJSON requires at least SSE4.2 on x86-64 ([x86-64-v2](https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels:~:text=their%20encryption%20extensions.-,Microarchitecture%20levels,-%5Bedit%5D)). ssrJSON does not work with other Python implementations other than CPython. Currently supported CPython versions are 3.9, 3.10, 3.11, 3.12, 3.13, 3.14.

Pre-built wheels will soon be available on PyPI. Then you can install it with

```
pip install ssrjson
```

### Build From Source

Since ssrJSON utilizes LLVM's vectorization extensions, it requires compilation with Clang and cannot be compiled in GCC or MSVC environments. On Windows, `clang-cl` can be used for this purpose. Build can be easily done by the following commands (make sure CMake, Clang and Python are already installed)

```bash
# On Linux:
# export CC=clang
# export CXX=clang++
mkdir build
cmake -S . -B build  # On Windows, configure with `clang -T ClangCL`
cmake --build build
```

## Usage

### Basic

```python
>>> import ssrjson
>>> ssrjson.dumps({"key": "value"})
'{"key":"value"}'
>>> ssrjson.loads('{"key":"value"}')
{'key': 'value'}
>>> ssrjson.dumps_to_bytes({"key": "value"})
b'{"key":"value"}'
>>> ssrjson.loads(b'{"key":"value"}')
{'key': 'value'}
```

### Indent

ssrJSON only supports encoding with indent = 2, 4 or no indent (indent=0). When indent is used, a space is inserted between each key and value.

```python
>>> import ssrjson
>>> ssrjson.dumps({"a": "b", "c": {"d": True}, "e": [1, 2]})
'{"a":"b","c":{"d":true},"e":[1,2]}'
>>> ssrjson.dumps({"a": "b", "c": {"d": True}, "e": [1, 2]}, indent=2)
'{\n  "a": "b",\n  "c": {\n    "d": true\n  },\n  "e": [\n    1,\n    2\n  ]\n}'
>>> ssrjson.dumps({"a": "b", "c": {"d": True}, "e": [1, 2]}, indent=4)
'{\n    "a": "b",\n    "c": {\n        "d": true\n    },\n    "e": [\n        1,\n        2\n    ]\n}'
>>> ssrjson.dumps({"a": "b", "c": {"d": True}, "e": [1, 2]}, indent=3)
Traceback (most recent call last):
  File "<python-input>", line 1, in <module>
    ssrjson.dumps({"a": "b", "c": {"d": True}, "e": [1, 2]}, indent=3)
    ~~~~~~~~~~~~~^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ValueError: indent must be 0, 2, or 4
```

### Other Arguments

Arguments like `ensure_ascii`, `parse_float` provided by `json` can be recognized but ignored by design.

The functionality of `object_hook` in `json.loads` will be supported in future.

## Implementation Details

The implementations of ssrJSON's `dumps` and `loads` functions are designed to perform in-place processing as much as possible, avoiding intermediate representations. The `dumps` function employs SIMD instructions for rapid encoding in a single step. Similarly, `dumps_to_bytes` uses SIMD to efficiently handle both UTF-8 encoding and JSON serialization at the same time. With minor modifications, the code used by `dumps_to_bytes` can also serve as a SIMD-accelerated replacement for `str.encode("utf-8")`.

The implementation of ssrJSON's `loads` draws inspiration from [yyjson](https://github.com/ibireme/yyjson), and also [orjson](https://github.com/ijl/orjson)'s caching algorithm for short dictionary keys. When the input type is `str`, `loads` avoids any UTF-8 encoding or decoding operations on non-ASCII strings. If the input is bytes, loads utilizes a modified string decoding algorithm based on yyjson. The main control flow and number decoding of `loads` are also modified from yyjson.

Generally, `ssrjson.dumps` behaves like `json.dumps` with `ensure_ascii=False`, and `ssrjson.loads` behaves like `json.loads`.

## Features

Below we explain some feature details of ssrJSON, which might be different from `json` module or other third-party JSON libraries.

### Strings

Code points within the range `[0xd800, 0xdfff]` cannot be represented in UTF-8 encoding, and the standard JSON specification typically prohibits the presence of such characters. However, since Python's `str` type is not encoded in UTF-8, ssrJSON aims to maintain compatibility with the Python json module's behavior, while other third-party Python JSON libraries may complain about this. In contrast, for the `dumps_to_bytes` function, which encodes output in UTF-8, the inclusion of these characters in the input is considered invalid.

```python
>>> s = chr(0xd800)
>>> (json.dumps(s, ensure_ascii=False) == '"' + s + '"', json.dumps(s, ensure_ascii=False))
(True, '"\ud800"')
>>> (ssrjson.dumps(s) == '"' + s + '"', ssrjson.dumps(s))
(True, '"\ud800"')
>>> ssrjson.dumps_to_bytes(s)
Traceback (most recent call last):
  File "<python-input>", line 1, in <module>
    ssrjson.dumps_to_bytes(s)
    ~~~~~~~~~~~~~~~~~~~~~~^^^
ssrjson.JSONEncodeError: Cannot encode unicode character in range [0xd800, 0xdfff] to utf-8
>>> json.loads(json.dumps(s, ensure_ascii=False)) == s
True
>>> ssrjson.loads(ssrjson.dumps(s)) == s
True
```

### Integers

`ssrjson.dumps` can only handle integers that can be expressed by either `uint64_t` or `int64_t` in C.

```python
>>> ssrjson.dumps(-(1<<63)-1)
Traceback (most recent call last):
  File "<python-input>", line 1, in <module>
    ssrjson.dumps(-(1<<63)-1)
    ~~~~~~~~~~~~~^^^^^^^^^^^^
ssrjson.JSONEncodeError: convert value to long long failed
>>> ssrjson.dumps(-(1<<63))
'-9223372036854775808'
>>> ssrjson.dumps((1<<64)-1)
'18446744073709551615'
>>> ssrjson.dumps(1<<64)
Traceback (most recent call last):
  File "<python-input>", line 1, in <module>
    ssrjson.dumps(1<<64)
    ~~~~~~~~~~~~~^^^^^^^
ssrjson.JSONEncodeError: convert value to unsigned long long failed
```

`ssrjson.loads` treats overflow integers as `float` objects.

```python
>>> ssrjson.loads('-9223372036854775809')  # -(1<<63)-1
-9.223372036854776e+18
>>> ssrjson.loads('-9223372036854775808')  # -(1<<63)
-9223372036854775808
>>> ssrjson.loads('18446744073709551615')  # (1<<64)-1
18446744073709551615
>>> ssrjson.loads('18446744073709551616')  # 1<<64
1.8446744073709552e+19
```

### Floats

For floating-point encoding, ssrJSON employs a slightly modified version of the [Dragonbox](https://github.com/jk-jeon/dragonbox) algorithm. Dragonbox is a highly efficient algorithm for converting floating-point to strings, typically producing output in scientific notation. ssrJSON has partially adapted this algorithm to enhance readability by outputting a more user-friendly format when no exponent is present.

Encoding and decoding `math.inf` are supported. `ssrjson.dumps` outputs the same result as `json.dumps`. The input of `ssrjson.loads` should be `"infinity"` with lower or upper cases (for each character), and cannot be `"inf"`.

```python
>>> json.dumps(math.inf)
'Infinity'
>>> ssrjson.dumps(math.inf)
'Infinity'
>>> ssrjson.loads("[infinity, Infinity, InFiNiTy, INFINITY]")
[inf, inf, inf, inf]
```

The case of `math.nan` is similar.

```python
>>> json.dumps(math.nan)
'NaN'
>>> ssrjson.dumps(math.nan)
'NaN'
>>> ssrjson.loads("[nan, Nan, NaN, NAN]")
[nan, nan, nan, nan]
```

## Limitations

Please note that ssrJSON is currently in the **beta stage** of development.

Several commonly used features are still under development, including the serialization of subclass objects of built-in types such as `dict`, `list`, and `str`, the `object_hook` functionality, and error location reporting during decoding. ssrJSON will not support encoding and decoding of third-party data structures.

The ARM64 architecture is not yet supported but will be supported in the near future.

## Contributing

Contributions are welcome! Please open issues or submit pull requests for bug fixes, performance improvements, or new features. There will soon be a development documentation.

## License

This project is licensed under the MIT License. Licenses of other repositories are under [licenses](licenses) directory.

## Acknowledgments

We would like to express our gratitude to the outstanding libraries and their authors:

- [CPython](https://github.com/python/cpython)
- [yyjson](https://github.com/ibireme/yyjson): ssrJSON draws extensively from yyjson’s highly optimized implementations, including the core decoding logic, the decoding of bytes objects, and the number decoding routines.
- [orjson](https://github.com/ijl/orjson): ssrJSON references parts of orjson’s SIMD-based ASCII string encoding and decoding algorithms, as well as the dictionary key caching mechanism. Additionally, ssrJSON utilizes orjson’s pytest framework for testing purposes.
- [Dragonbox](https://github.com/jk-jeon/dragonbox): ssrJSON employs Dragonbox for high-performance floating-point encoding.
- [xxHash](https://github.com/Cyan4973/xxHash): ssrJSON leverages xxHash to efficiently compute hash values for key caching.


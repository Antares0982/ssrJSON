<div align="center">

# **ssrJSON**

[![PyPI - Version](https://img.shields.io/pypi/v/ssrjson)](https://pypi.org/project/ssrjson/) [![PyPI - Wheel](https://img.shields.io/pypi/wheel/ssrjson)](https://pypi.org/project/ssrjson/) [![Supported Python versions](https://img.shields.io/pypi/pyversions/ssrjson.svg?logo=python&logoColor=FFE873)](https://pypi.org/project/ssrjson) [![codecov](https://codecov.io/gh/Antares0982/ssrJSON/graph/badge.svg?token=A1T0XTPEXO)](https://codecov.io/gh/Antares0982/ssrJSON)

A SIMD boosted high-performance and correct Python JSON parsing library that fully leverages modern processor capabilities.

</div>

## Introduction

ssrJSON is a Python JSON library that leverages modern hardware capabilities to achieve peak performance, implemented primarily in C with some components written in C++. It offers a fully compatible interface to Python’s standard `json` module, making it a seamless drop-in replacement, while providing exceptional performance for JSON encoding and decoding.

If you prefer to skip the technical details below, please proceed directly to the [How To Install](#how-to-install) section.

### How Fast is ssrJSON?

TL;DR: ssrJSON is faster than or nearly as fast as [orjson](https://github.com/ijl/orjson) (which [announces](https://github.com/ijl/orjson/blob/3.11.4/README.md#:~:text=It%20benchmarks%20as%20the%20fastest) itself as the fastest Python library for JSON) on most benchmark cases.

![](pics/ratio_distribution.v0.0.9.svg)

Below is an artificial benchmark case to demonstrate the speed of encoding non-ASCII JSON ([simple_object_zh.json](https://github.com/Nambers/ssrJSON-benchmark/blob/9207eb70c972200cec44ea3538773590b59b01ad/src/ssrjson_benchmark/_files/simple_object_zh.json)). Upon seeing the diagram below, you might wonder: why do the performance results from other libraries appear so poor? If you are interested, please refer to the section [UTF-8 Cache of str Objects](#utf-8-cache-of-str-objects).

![](pics/simple_object_zh.json_dumps_to_bytes.v0.0.9.svg)
![](pics/simple_object_zh.json_load&dump.v0.0.9.svg)

Real-world case ([twitter.json](https://github.com/Nambers/ssrJSON-benchmark/blob/9207eb70c972200cec44ea3538773590b59b01ad/src/ssrjson_benchmark/_files/twitter.json)):

![](pics/twitter.json_dumps_to_bytes.v0.0.9.svg)
![](pics/twitter.json_load&dump.v0.0.9.svg)

Real-world case II ([github.json](https://github.com/Nambers/ssrJSON-benchmark/blob/9207eb70c972200cec44ea3538773590b59b01ad/src/ssrjson_benchmark/_files/github.json)):

![](pics/github.json_dumps_to_bytes.v0.0.9.svg)
![](pics/github.json_load&dump.v0.0.9.svg)

Floats ([canada.json](https://github.com/Nambers/ssrJSON-benchmark/blob/9207eb70c972200cec44ea3538773590b59b01ad/src/ssrjson_benchmark/_files/canada.json)):

![](pics/canada.json_dumps_to_bytes.v0.0.9.svg)
![](pics/canada.json_load&dump.v0.0.9.svg)

Numbers ([mesh.json](https://github.com/Nambers/ssrJSON-benchmark/blob/9207eb70c972200cec44ea3538773590b59b01ad/src/ssrjson_benchmark/_files/mesh.json)):

![](pics/mesh.json_dumps_to_bytes.v0.0.9.svg)
![](pics/mesh.json_load&dump.v0.0.9.svg)

`ssrjson.dumps()` is about 4x-27x as fast as `json.dumps()` (Python3.14, x86-64, AVX2). `ssrjson.loads()` is about 2x-8x as fast as `json.loads()` for `str` input and is about 2x-8x as fast as `json.loads()` for `bytes` input (Python3.14, x86-64, AVX2). ssrJSON also provides `ssrjson.dumps_to_bytes()`, which encode Python objects directly to UTF-8 encoded `bytes` object using SIMD instructions.

Details of benchmarking can be found in the [ssrjson-benchmark](https://github.com/Nambers/ssrJSON-benchmark) project. If you wish to run the benchmark tests yourself, you can execute the following commands:

```bash
pip install ssrjson-benchmark
python -m ssrjson_benchmark
```

This will generate a PDF report of the results. If you choose to, you may submit this report to the benchmark repository, allowing others to view the performance metrics of ssrJSON on your device.

### SIMD Acceleration

ssrJSON is designed for modern hardware and extensively leverages SIMD instruction sets to accelerate encoding and decoding processes. This includes operations such as memory copying, integer type conversions, JSON encoding, and UTF-8 encoding. Currently, ssrJSON supports x86-64-v2 and above (requiring at least SSE4.2) as well as aarch64 devices. It does not support 32-bit systems or older x86-64 and ARM hardware with limited SIMD capabilities.

On the x86-64 platform, ssrJSON provides three distinct SIMD libraries optimized for SSE4.2, AVX2, and AVX512, respectively, automatically selecting the most appropriate library based on the device’s capabilities. For aarch64 architectures, it utilizes the NEON instruction set. Combined with Clang’s powerful vector extensions and compiler optimizations, ssrJSON can almost fully exploit CPU performance during encoding operations.

### UTF-8 Cache of `str` Objects

Non-ASCII `str` objects may store a cached representation of their UTF-8 encoding (within the corresponding C structure `PyUnicodeObject`, represented as a `const char *` and a length with type `Py_ssize_t`) to minimize the overhead of subsequent UTF-8 encoding operations. When `PyUnicode_AsUTF8AndSize` (or other similar functions) is invoked, the CPython implementation utilizes it to store the C string along with its length. This mechanism ensures that the caller does not need to manage the lifetime of the returned C string. The `str.encode("utf-8")` operation does not write to the cache; however, if the cache is already present, it utilizes the cached data to create the `bytes` object.

To the best of author's knowledge, existing third-party Python JSON libraries typically utilize certain CPython APIs to indirectly write the UTF-8 cache when performing `dumps` on non-ASCII `str` objects when the cache does not exist. This results in benchmark tests appearing more favorable than they actually are, since the same object is repeatedly dumped during performance measurements and the cache written will be utilized. But in reality, UTF-8 encoding is computationally intensive on the CPU and often becomes a major performance bottleneck in the dumping process. Also, writing cache will increase the memory usage. Also it is worth noting that during JSON encoding and decoding in Python, converting between `str` objects does not involve any UTF-8-related operations. However, some third-party JSON libraries still directly or indirectly invoke UTF-8 encoding APIs, which are resource-intensive. This explains why other third-party libraries exhibit poor performance when performing `loads` on `str` inputs, or when their `dumps` function outputs `str` types.

`ssrjson.dumps_to_bytes` addresses this by leveraging SIMD instruction sets for UTF-8 encoding, achieving significantly better performance than conventional encoding algorithms implemented in CPython. Furthermore, ssrJSON grants users explicit control over whether or not to write this cache. It is recommended that users evaluate their projects for repeated encoding of each `str` object to decide on enabling or disabling this caching mechanism accordingly. (Note that `ssrjson.dumps` produces a `str` object; there is nothing related to this topic.)

Also, the [ssrjson-benchmark](https://github.com/Nambers/ssrJSON-benchmark) project takes this aspect into account by differentiating test scenarios based on the presence or absence of this cache. The results demonstrate that ssrJSON **maintains a substantial performance advantage over other third-party Python JSON libraries regardless of whether the cache exists**.

If you decide to enable writing cache, ssrJSON will first ensure the cache. The following `dumps_to_bytes` calls on the same `str` object will be faster, but the first time may be slower and memory cost may grow.

Pros:

* The following calls after the first call to `dumps_to_bytes` on the same `str` might be faster.

Cons:

* The first call to `dumps_to_bytes` (when visiting a non-ASCII `str` without cache) might be slower.
* The memory cost will grow. Each non-ASCII `str`  visited will result in memory usage corresponding to the length of its UTF-8 representation. The memory will be released only when the `str` object is deallocated.

If you decide to disable it, ssrJSON will not write cache; but if the cache already exists, ssrJSON will still use it.

By default, writing cache is enabled globally. You can use `ssrjson.write_utf8_cache` to control this behavior globally, or pass `is_write_cache` to `ssrjson.dumps_to_bytes` in each call.

### Dragonbox

Tests and comparisons reveals that the [Dragonbox](https://github.com/jk-jeon/dragonbox) algorithm significantly outperforms other algorithms in terms of performance. Consequently, the ssrJSON project adopts a floating-point number string conversion approach based on the Dragonbox algorithm. The modified Dragonbox library constitutes the sole C++ code within ssrJSON.

### JSON Module compatibility

The design goal of ssrJSON is to provide a straightforward and highly compatible approach to replace the inherently slower Python standard JSON encoding and decoding implementation with a significantly more efficient and high-performance alternative. If your module exclusively utilizes `dumps` and `loads`, you can replace the current JSON implementation by importing ssrJSON as `import ssrjson as json`. To facilitate this, ssrJSON maintains compatibility with the argument formats of `json.dumps` and `json.loads`; however, it does not guarantee identical results to the standard JSON module, as many features are either intentionally omitted or not yet supported. For further information, please refer to the section [Features](#Features).

### Other Implementation Details

#### Overview of Encoding

The encoding performance of JSON libraries is not significantly limited by CPython, resulting in a very high potential maximum. As mentioned above, during string encoding, ssrJSON extensively utilizes SIMD instructions to accelerate copying and conversion operations. The implementation of `dumps_to_bytes` also tackles challenges related to UTF-8 encoding. ssrJSON includes a comprehensive UTF-8 encoding algorithm optimized for all supported SIMD features as well as Python’s internal string representation format (PyCompactUnicodeObject).  When encoding integers, ssrJSON adapts the integer encoding approach from [yyjson](https://github.com/ibireme/yyjson), a highly optimized C-language JSON parsing library.

#### Overview of Decoding

The main performance bottleneck in JSON decoding is the speed of creating Python objects. To address this, ssrJSON adopts the short-key caching mechanism from orjson, which greatly reduces the overhead of creating Python string objects. For string handling, when the input is of `str` type, ssrJSON applies SIMD optimizations similar to those used in encoding, speeding up the decoding process. For `bytes` inputs, ssrJSON uses a customized version of yyjson’s string decoding algorithm. Beyond string handling, ssrJSON extensively leverages yyjson’s codebase, including its numeric decoding algorithms and core decoding logic.

## Limitations

Please note that ssrJSON is currently in its beta development stage, and some common features have yet to be implemented. We welcome your contributions to help build a highly performant Python JSON library.

Once the essential features are completed and the project has undergone public testing to reach a stable phase, ssrJSON will strive to minimize the addition of new features to maintain its stability. There are two main reasons for this approach: first, ssrJSON aims to serve as a high-performance foundational library rather than one overloaded with various elaborate features; second, although leveraging C language brings significant performance advantages, it also introduces considerable potential instability. Drawing from software engineering experience, limiting new features will help reduce the incidence of critical vulnerabilities.

## How To Install

Pre-built wheels are available on PyPI, you can install it using pip.

```
pip install ssrjson
```

Note: ssrJSON requires at least SSE4.2 on x86-64 ([x86-64-v2](https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels)), or aarch64. 32-bit platforms are not supported. ssrJSON does not work with Python implementations other than CPython. Currently supported CPython versions are 3.10, 3.11, 3.12, 3.13, 3.14, 3.15. For Python >= 3.15, you need to build it from source.

### Build From Source

Since ssrJSON utilizes Clang's vector extensions, it requires compilation with Clang and cannot be compiled in GCC or pure MSVC environments. On Windows, `clang-cl` can be used for this purpose. Build can be easily done by the following commands (make sure CMake, Clang and Python are already installed)

```bash
# On Linux:
# export CC=clang
# export CXX=clang++
mkdir build
cmake -S . -B build  # On Windows, configure with `cmake -T ClangCL`
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

ssrJSON only supports encoding with indent = 2, 4 or no indent (don't pass indent, or pass indent=None). When indent is used, a space is inserted between each key and value.

```python
>>> import ssrjson
>>> ssrjson.dumps({"a": "b", "c": {"d": True}, "e": [1, 2]})
'{"a":"b","c":{"d":true},"e":[1,2]}'
>>> print(ssrjson.dumps({"a": "b", "c": {"d": True}, "e": [1, 2]}, indent=2))
{
  "a": "b",
  "c": {
    "d": true
  },
  "e": [
    1,
    2
  ]
}
>>> print(ssrjson.dumps({"a": "b", "c": {"d": True}, "e": [1, 2]}, indent=4))
{
    "a": "b",
    "c": {
        "d": true
    },
    "e": [
        1,
        2
    ]
}
>>> ssrjson.dumps({"a": "b", "c": {"d": True}, "e": [1, 2]}, indent=3)
Traceback (most recent call last):
  File "<python-input>", line 1, in <module>
    ssrjson.dumps({"a": "b", "c": {"d": True}, "e": [1, 2]}, indent=3)
    ~~~~~~~~~~~~~^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ValueError: integer indent must be 2 or 4
```

### Other Arguments Supported by Python's json

Arguments like `ensure_ascii`, `parse_float` provided by `json` module can be recognized but *ignored by design*. To treat passing these arguments as an error, call `ssrjson.strict_argparse(True)` once and it will take effect globally.

The functionality of `object_hook` in `json.loads` will be supported in future.

### Inspect Module Features and Settings

Call `get_current_features` to get current features and settings of ssrJSON.

```python
>>> ssrjson.get_current_features()
{'multi_lib': True, 'write_utf8_cache': True, 'strict_arg_parse': False, 'simd': 'AVX2'}
```

## Features

Generally, `ssrjson.dumps` behaves like `json.dumps` with `ensure_ascii=False`, and `ssrjson.loads` behaves like `json.loads`. Below we explain some feature details of ssrJSON, which might be different from `json` module or other third-party JSON libraries.

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

## License

This project is licensed under the MIT License. Licenses of other repositories are under [licenses](licenses) directory.

## Acknowledgments

We would like to express our gratitude to the outstanding libraries and their authors:

- [CPython](https://github.com/python/cpython)
- [yyjson](https://github.com/ibireme/yyjson): ssrJSON draws extensively from yyjson’s highly optimized implementations, including the core decoding logic, the decoding of bytes objects, the integer encoding and number decoding routines.
- [orjson](https://github.com/ijl/orjson): ssrJSON references parts of orjson’s SIMD-based ASCII string encoding and decoding algorithms, as well as the key caching mechanism. Additionally, ssrJSON utilizes orjson’s pytest framework for testing purposes.
- [Dragonbox](https://github.com/jk-jeon/dragonbox): ssrJSON employs Dragonbox for high-performance floating-point encoding.
- [xxHash](https://github.com/Cyan4973/xxHash): ssrJSON leverages xxHash to efficiently compute hash values for key caching.


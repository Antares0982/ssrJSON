<div align="center">

# **ssrJSON**

[![PyPI - Version](https://img.shields.io/pypi/v/ssrjson)](https://pypi.org/project/ssrjson/) [![PyPI - Wheel](https://img.shields.io/pypi/wheel/ssrjson)](https://pypi.org/project/ssrjson/) [![Supported Python versions](https://img.shields.io/pypi/pyversions/ssrjson.svg?logo=python&logoColor=FFE873)](https://pypi.org/project/ssrjson) [![codecov](https://codecov.io/gh/Antares0982/ssrJSON/graph/badge.svg?token=A1T0XTPEXO)](https://codecov.io/gh/Antares0982/ssrJSON)

由 SIMD 加速、兼具高性能与正确性的 Python JSON 解析库，比最快更快。

[English](https://github.com/Antares0982/ssrJSON/blob/main/README.md)

</div>

## 简介

ssrJSON 是一个主要用 C 实现的 Python JSON 库，旨在利用现代硬件的能力实现极致性能。其接口与 Python 标准库 `json` 模块完全兼容，可以直接作为标准库的无缝替代品。

如果想跳过下面的技术细节，可以直接跳到[安装方法](#安装方法)一节。

### ssrJSON 有多快？

在大多数 benchmark 场景中，ssrJSON 都比 [orjson](https://github.com/ijl/orjson) 更快或与之持平（orjson [宣称](https://github.com/ijl/orjson/blob/3.11.4/README.md#:~:text=It%20benchmarks%20as%20the%20fastest)自己为最快 Python JSON 库）。

![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/ratio_distribution.v0.0.17.svg)

下面是一个*人为构造*的 benchmark 场景，用于展示非 ASCII JSON 的编码速度（[simple_object_zh.json](https://github.com/Nambers/ssrJSON-benchmark/blob/9207eb70c972200cec44ea3538773590b59b01ad/src/ssrjson_benchmark/_files/simple_object_zh.json)）。看到下图你可能会疑惑：为什么其他库的表现这么差？如果你感兴趣，请参阅[`str` 对象的 UTF-8 缓存](#str-对象的-utf-8-缓存)一节。

![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/simple_object_zh.json_dumps_to_bytes.v0.0.17.svg)
![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/simple_object_zh.json_load&dump.v0.0.17.svg)

真实场景（[twitter.json](https://github.com/Nambers/ssrJSON-benchmark/blob/9207eb70c972200cec44ea3538773590b59b01ad/src/ssrjson_benchmark/_files/twitter.json)）：

![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/twitter.json_dumps_to_bytes.v0.0.17.svg)
![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/twitter.json_load&dump.v0.0.17.svg)

真实场景 II（[github.json](https://github.com/Nambers/ssrJSON-benchmark/blob/9207eb70c972200cec44ea3538773590b59b01ad/src/ssrjson_benchmark/_files/github.json)）：

![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/github.json_dumps_to_bytes.v0.0.17.svg)
![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/github.json_load&dump.v0.0.17.svg)

浮点数（[canada.json](https://github.com/Nambers/ssrJSON-benchmark/blob/9207eb70c972200cec44ea3538773590b59b01ad/src/ssrjson_benchmark/_files/canada.json)）：

![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/canada.json_dumps_to_bytes.v0.0.17.svg)
![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/canada.json_load&dump.v0.0.17.svg)

数字（[mesh.json](https://github.com/Nambers/ssrJSON-benchmark/blob/9207eb70c972200cec44ea3538773590b59b01ad/src/ssrjson_benchmark/_files/mesh.json)）：

![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/mesh.json_dumps_to_bytes.v0.0.17.svg)
![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/mesh.json_load&dump.v0.0.17.svg)

在 Python 3.14（x86-64、AVX2）环境下，`ssrjson.dumps()` 的速度约为 `json.dumps()` 的 4 ~ 31 倍；`ssrjson.loads()` 的速度约为 `json.loads()` 的 2 ~ 8 倍。ssrJSON 还提供 `ssrjson.dumps_to_bytes()`，它利用 SIMD 优化把 Python 对象直接编码为 UTF-8 的 `bytes` 对象。

benchmark 的详细信息可在 [ssrjson-benchmark](https://github.com/Nambers/ssrJSON-benchmark) 项目中找到。如果你想运行 benchmark ，可以执行以下命令：

```bash
pip install ssrjson-benchmark
python -m ssrjson_benchmark
```

运行结束后会生成一份包含测试结果的 PDF 报告。你可以把这份报告 PR 到 [ssrjson-benchmark](https://github.com/Nambers/ssrJSON-benchmark) 仓库，让其他人也看到 ssrJSON 在你设备上的性能表现。

### SIMD 加速

ssrJSON 面向现代硬件设计，在编码与解码过程中大量使用 SIMD 指令集来加速内存复制、整数类型转换、JSON 编码和 UTF-8 编码等操作。目前，ssrJSON 支持 x86-64-v2 及更高版本（至少需要 SSE4.2）以及 aarch64 设备；不支持 32 位，也不支持 SIMD 能力有限的旧 x86-64 硬件。

在 x86-64 平台上，ssrJSON 分别针对 SSE4.2、AVX2 和 AVX512 提供了三个独立的 SIMD 实现，运行时会根据设备能力自动选择最合适的一个；aarch64 架构则使用 NEON 指令集。借助 Clang 强大的向量扩展和编译器优化，ssrJSON 在编码时几乎能榨干 CPU 的性能。

### `str` 对象的 UTF-8 缓存

> 我写过一篇关于此主题的详细技术博客：[警惕第三方 Python JSON 库的性能陷阱](https://en.chr.fan/2026/01/07/python-json/)。

非 ASCII `str` 对象可能会缓存其 UTF-8 编码结果（缓存放于对应的 C 结构 `PyUnicodeObject` 中，由一个 `const char *` 和 `Py_ssize_t` 类型的长度构成），以减少后续 UTF-8 编码操作的开销。调用 `PyUnicode_AsUTF8AndSize`（或其他类似函数）时，CPython 会顺便把生成的 C 字符串及其长度存入缓存，因此调用方无需管理返回字符串的生命周期。`str.encode("utf-8")` 不会写入缓存；但如果缓存已经存在，它会直接利用缓存数据来创建 `bytes` 对象。

一些第三方 Python JSON 库在对尚未写入缓存的非 ASCII `str` 对象执行 `dumps` 时，通常会借助某些 CPython API 把 UTF-8 结果间接写入缓存。这让 benchmark 看起来比实际更好看：测量时会反复转储同一个对象，而已经写入的缓存恰好被复用。但事实上，UTF-8 编码非常消耗 CPU 性能，通常是主要的性能瓶颈；同时，写入缓存还会增加内存占用。

`ssrjson.dumps_to_bytes` 利用 SIMD 指令集完成 UTF-8 编码，解决了上述问题，性能明显优于 CPython 内置的传统编码算法。此外，ssrJSON 能让用户自行控制是否写入该缓存，开启的优势是同一个 `str` 对象在第一次 `dumps_to_bytes` 之后，后续相同调用可能会更快；缺点是内存开销增加，每个被访问过的非 ASCII `str` 都会按其 UTF-8 表示的长度占用额外内存，这些内存要等 `str` 对象被销毁后才会释放。建议先评估项目中是否会反复编码同一个 `str` 对象，再据此决定开启还是关闭缓存。

为此，[ssrjson-benchmark](https://github.com/Nambers/ssrJSON-benchmark) 项目在测试中区分了缓存存在与不存在两种场景。结果显示，**无论缓存是否存在，ssrJSON 相比其他第三方 Python JSON 库都保持显著的性能优势**。

缓存写入默认全局开启。你可以通过 `ssrjson.write_utf8_cache` 全局控制这一行为，也可以每次调用 `ssrjson.dumps_to_bytes` 时通过 `is_write_cache` 参数单独指定。

> `ssrjson.dumps` 生成的是 `str` 对象，与本节内容无关。

### xjb64

ssrJSON 使用 xjb64 作为浮点数转字符串的算法。测试与对比表明，[xjb64](https://github.com/xjb714/xjb) 算法在性能上明显优于同类算法，兼容性也更好。为符合 Python `json` 模块的标准行为，ssrJSON 采用了经过小幅修改的版本。

Apple M1 上的随机 double ：

![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/xjb_random_double_m1.svg)

AMD R7-7840H 上的随机 double ：

![](https://raw.githubusercontent.com/Antares0982/ssrJSON/main/pics/xjb_random_double_7840h.svg)

### JSON 模块兼容性

ssrJSON 的设计目标，是以简单直接、高度兼容的方式，为性能偏慢的 Python 标准 JSON 编解码实现提供高效得多、性能强劲的替代方案。如果代码里只用到 `dumps` 和 `loads`，通常直接用 `import ssrjson as json` 即可替换当前的 JSON 实现。为此，ssrJSON 保持了与 `json.dumps`、`json.loads` 一致的参数格式；但它不保证结果与标准 `json` 模块完全相同，因为许多 feature 被刻意省略或尚未支持。更多信息请参阅[行为](#行为)一节。

### 其他实现细节

#### 编码概览

CPython 基本不会限制 JSON 库的编码性能，因此其理论性能上限非常高。如上所述，ssrJSON 在编码字符串时广泛使用 SIMD 指令来加速复制和转换；`dumps_to_bytes` 则专门解决了 UTF-8 编码的难题——ssrJSON 内置了一套完整的 UTF-8 编码算法，针对所有受支持的 SIMD 特性以及 Python 的内部字符串表示格式（PyCompactUnicodeObject）都做了优化。编码整数时，ssrJSON 借鉴了高度优化的 C 语言 JSON 解析库 [yyjson](https://github.com/ibireme/yyjson) 的整数编码方法。

#### 解码概览

JSON 解码的主要瓶颈在于创建 Python 对象的速度。为此，ssrJSON 采用了 orjson 的短键缓存机制，大幅降低了创建 Python 字符串对象的开销。处理字符串时，如果输入是 `str`，ssrJSON 会应用 SIMD 优化来加速解码；如果输入是 `bytes`，字符串解码同样经过向量化：先用 SIMD 校验源字节块并转码到目标的 UCS 宽度，遇到更宽的码点时再原地加宽。这部分借鉴了 simdutf 的 UTF-8 转码模式和 Lemire 的块校验方法；当块内序列长度差异过大、不适合作向量化处理时，则回退到 yyjson 的逐序列解码。除字符串处理外，ssrJSON 还大量使用了 yyjson 的代码，包括其数字解码算法和核心解码逻辑。

## 局限性

请注意，ssrJSON 目前仍处于 beta 阶段，一些常用 feature 尚未实现。欢迎你贡献力量，一起打造一个高性能的 Python JSON 库。

为了保持稳定性，ssrJSON 会尽量少添加很少被使用的 feature ，原因有二：

* ssrJSON 的目标是成为高性能的基础库，而不是堆满各种花哨 feature 的库。
* 使用 C 语言虽然带来了显著的性能优势，但也伴随相当大的不稳定性风险。软件工程的经验表明，减少不常用 feature 有助于减少严重漏洞的发生。

## 安装方法

### 从 PyPI 安装

PyPI 上提供了预构建的 wheel，你可以使用 pip 安装。

```
pip install ssrjson
```

注意：在 x86-64 上，ssrJSON 至少需要 SSE4.2（[x86-64-v2](https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels)）或 aarch64 架构，不支持 32 位平台，也无法在 CPython 以外的 Python 实现上运行。目前支持的 CPython 版本为 3.10、3.11、3.12、3.13、3.14、3.15，其中 Python ≥ 3.15 需要从源码构建。

### 从源码构建

ssrJSON 使用了 Clang 的向量扩展，因此必须用 Clang 编译，无法在 GCC 或纯 MSVC 环境中编译；Windows 上可以使用 `clang-cl`。确保已经安装 CMake、Clang 和 Python 后，通过下面的命令即可轻松构建：

```bash
# 在 Linux 上：
# export CC=clang
# export CXX=clang++
mkdir build
cmake -S . -B build  # 在 Windows 上，使用 `cmake -T ClangCL` 配置
cmake --build build
```

或者，如果你更习惯用 `pip`：

```
mv pysrc ssrjson  # 重命名 Python 源码目录，使其可以安装
pip install .
```

## 用法

### 基本用法

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

### NumPy 支持

ssrJSON 可以直接序列化 NumPy 标量类型和 `ndarray` 对象，无需先把它们转换成 Python 类型。为避免把 NumPy 变成硬依赖，你需要先调用一次 `setup_numpy_types` 来显式启用该 feature ：

```python
>>> import numpy as np
>>> import ssrjson
>>> ssrjson.setup_numpy_types(np)
```

完成设置后，`dumps` 和 `dumps_to_bytes` 就能识别 NumPy 标量和数组：

```python
>>> ssrjson.dumps(np.int64(42))
'42'
>>> ssrjson.dumps(np.array([1, 2, 3]))
'[1,2,3]'
>>> ssrjson.dumps({"data": np.array([[1, 2], [3, 4]]), "score": np.float32(0.95)})
'{"data":[[1,2],[3,4]],"score":0.95}'
>>> ssrjson.dumps_to_bytes(np.arange(5))
b'[0,1,2,3,4]'
```

支持的 NumPy 类型：

| 类别 | 类型 |
|---|---|
| 整数 | `int8`、`int16`、`int32`、`int64`、`uint8`、`uint16`、`uint32`、`uint64` |
| 浮点数 | `float16`、`float32`、`float64` |
| 布尔值 | `bool_` |
| 数组 | `ndarray`（C 连续、任意受支持的元素 dtype、最多 32 维） |

`np.float64` 是 Python `float` 的子类，无论是否调用过 `setup_numpy_types`，都会走标准的浮点数编码路径。

编码 ndarray 时，ssrJSON 直接从数组的内存缓冲区读出元素数据。再结合 xjb64/xjb32 浮点编码算法和源自 yyjson 的整数编码算法，ssrJSON 拥有显著的性能优势。

简单的 benchmark 表明，ssrJSON 在编码 NumPy 数组时有显著优势：

```
$ python dev_tools/numpy_benchmark.py --scale 10
numpy 2.4.2  |  scale=10  number=20  repeat=7  warmup=2
Python 3.14.3

[dumps_to_bytes: ssrjson vs orjson — numpy ndarray]

  int32_1d[1000000]  shape=1000000  dtype=int32  3906.2 KiB raw
    ssrjson  : median 1.83 ms  best 1.78 ms  out=4391695B  2.24 GiB/s
    orjson   : median 4.96 ms  best 4.88 ms  out=4391695B  843.99 MiB/s
    ssrjson is 2.72x faster than orjson (median); best 2.74x

  int64_1d[1000000]  shape=1000000  dtype=int64  7812.5 KiB raw
    ssrjson  : median 4.30 ms  best 4.13 ms  out=10982023B  2.38 GiB/s
    orjson   : median 8.79 ms  best 8.66 ms  out=10982023B  1.16 GiB/s
    ssrjson is 2.05x faster than orjson (median); best 2.10x

  float32_1d[1000000]  shape=1000000  dtype=float32  3906.2 KiB raw
    ssrjson  : median 9.15 ms  best 9.04 ms  out=10627011B  1.08 GiB/s
    orjson   : median 17.25 ms  best 17.18 ms  out=10627112B  587.55 MiB/s
    ssrjson is 1.88x faster than orjson (median); best 1.90x

  float64_1d[1000000]  shape=1000000  dtype=float64  7812.5 KiB raw
    ssrjson  : median 13.27 ms  best 12.94 ms  out=19269164B  1.35 GiB/s
    orjson   : median 19.12 ms  best 18.99 ms  out=19269255B  961.00 MiB/s
    ssrjson is 1.44x faster than orjson (median); best 1.47x

  float64_2d[1000x1000]  shape=1000x1000  dtype=float64  7812.5 KiB raw
    ssrjson  : median 13.50 ms  best 13.14 ms  out=19272764B  1.33 GiB/s
    orjson   : median 19.09 ms  best 19.04 ms  out=19272837B  962.88 MiB/s
    ssrjson is 1.41x faster than orjson (median); best 1.45x

  float64_3d[100x100x100]  shape=100x100x100  dtype=float64  7812.5 KiB raw
    ssrjson  : median 13.46 ms  best 13.13 ms  out=19291108B  1.33 GiB/s
    orjson   : median 19.48 ms  best 19.42 ms  out=19291190B  944.22 MiB/s
    ssrjson is 1.45x faster than orjson (median); best 1.48x

  int32_2d[1000x1000]  shape=1000x1000  dtype=int32  3906.2 KiB raw
    ssrjson  : median 1.17 ms  best 1.17 ms  out=5391602B  4.31 GiB/s
    orjson   : median 5.04 ms  best 5.00 ms  out=5391602B  1020.42 MiB/s
    ssrjson is 4.32x faster than orjson (median); best 4.29x

  bool_1d[1000000]  shape=1000000  dtype=bool  976.6 KiB raw
    ssrjson  : median 323.7 µs  best 323.1 µs  out=5500290B  15.83 GiB/s
    orjson   : median 3.24 ms  best 3.21 ms  out=5500290B  1.58 GiB/s
    ssrjson is 10.02x faster than orjson (median); best 9.92x
```

### 缩进

ssrJSON 编码时仅支持 `indent=2`、`indent=4` 或不缩进（不传 `indent`，或传入 `indent=None`）。使用缩进时，键与值之间会插入一个空格。

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

### Python `json` 支持的其他参数

`object_hook` 可在 `ssrjson.loads` 中使用，其行为与 `json.loads` 相同。

`json` 模块里的 `ensure_ascii`、`parse_float` 等参数能被识别，但会*被刻意忽略*。如果想改成在传入这些参数时报错，调用一次 `ssrjson.strict_argparse(True)` 即可，该设置全局生效。

### 查看 features

调用 `get_current_features` 可获取 ssrJSON 当前的构建配置和设置。

```python
>>> ssrjson.get_current_features()
{'multi_lib': True, 'write_utf8_cache': True, 'strict_arg_parse': False, 'free_threading': False, 'lockfree': False, 'simd': 'AVX2'}
```

## 行为

总体而言，`ssrjson.dumps` 的行为与使用 `ensure_ascii=False` 的 `json.dumps` 一致，`ssrjson.loads` 的行为与 `json.loads` 一致。下面介绍 ssrJSON 的一些行为细节，它们可能与标准 `json` 模块或其他第三方 JSON 库不同。

### 字符串

`[0xd800, 0xdfff]` 范围内的码点无法用 UTF-8 表示，标准 JSON 规范通常也禁止出现这类字符。不过，由于 Python 的 `str` 并不是按 UTF-8 存储，为与 Python `json` 模块的行为保持一致，`ssrjson.dumps` 允许这些字符，而其他第三方 Python JSON 库可能会对这种情况报错。相比之下，`ssrjson.dumps_to_bytes` 输出的是 UTF-8 编码，输入中包含这类字符会被视为错误。

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
ssrjson.JSONEncodeError: Cannot encode unicode character in range [0xd800, 0xdfff] to UTF-8
>>> json.loads(json.dumps(s, ensure_ascii=False)) == s
True
>>> ssrjson.loads(ssrjson.dumps(s)) == s
True
```

### 整数

`ssrjson.dumps` 只能处理能够用 C 中的 `uint64_t` 或 `int64_t` 表示的整数。

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

对于超出 `int64_t`/`uint64_t` 表示范围的整数，`ssrjson.loads` 会将其解析为 `float` 对象。

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

### 浮点数

浮点数编码采用 [xjb64](https://github.com/xjb714/xjb) 算法——一个把浮点数高效转换为字符串的算法。

ssrJSON 支持编码和解码 `math.inf`：`ssrjson.dumps` 的输出与 `json.dumps` 相同；`ssrjson.loads` 接受 `"infinity"` 的任意大小写拼写（每个字母可分别大写或小写），但不接受 `"inf"`。

```python
>>> json.dumps(math.inf)
'Infinity'
>>> ssrjson.dumps(math.inf)
'Infinity'
>>> json.dumps(-math.inf)
'-Infinity'
>>> ssrjson.dumps(-math.inf)
'-Infinity'
>>> ssrjson.loads("[infinity, Infinity, InFiNiTy, INFINITY]")  # 允许，但不建议在 JSON 中写作 `InFiNiTy`
[inf, inf, inf, inf]
```

`math.nan` 的情况也类似，不过 NaN 没有符号：无论正负，编码结果都是 `NaN`。

```python
>>> json.dumps(math.nan)
'NaN'
>>> ssrjson.dumps(math.nan)
'NaN'
>>> json.dumps(-math.nan)
'NaN'
>>> ssrjson.dumps(-math.nan)
'NaN'
>>> ssrjson.loads("[nan, Nan, NaN, NAN]")  # 允许，但不建议在 JSON 中写 `Nan`
[nan, nan, nan, nan]
```

### Free-threading

ssrJSON 实验性地支持 free-threading（Python ≥ 3.14），PyPI 上有相应的稳定 wheel 。从源码构建时，可通过 `-DBUILD_FREE_THREADING=ON` 启用该 feature 。在此类构建中，编码时 ssrJSON 会从外到内获取 dict 和 list 对象上的锁；若其他线程以不同的顺序锁定这些对象，则可能发生死锁——这是预期行为。如果遇到意外崩溃，请提交 issue。解码过程则完全无锁。

如果需要无锁的编码版本，请使用 `-DFREE_THREADING_LOCKFREE=ON` 从源码构建。与基于锁的版本相比，无锁版本的单线程编码性能提升约 13%。需要注意：在该配置下，多个线程同时修改同一个 dict/list 是竞态，用户须用其他办法自行确保不存在竞态。PyPI 不提供无锁版本的 wheel 。

## 许可证

本项目采用 MIT 许可证。其他仓库的许可证位于 [licenses](licenses) 目录中。

## 致谢

我们谨向以下优秀的库及其作者表示感谢：

- [CPython](https://github.com/python/cpython)
- [yyjson](https://github.com/ibireme/yyjson)：ssrJSON 广泛借鉴了 yyjson 的高优化实现，包括核心解码逻辑、bytes 对象解码、整数编码和数字解码例程。
- [orjson](https://github.com/ijl/orjson)：ssrJSON 参考了 orjson 基于 SIMD 的部分 ASCII 字符串编解码算法，以及短键缓存机制。此外，ssrJSON 使用 orjson 的 pytest 代码作为初版代码进行测试。
- [xjb64](https://github.com/xjb714/xjb)：ssrJSON 使用 xjb32/64 算法进行高性能浮点数编码。
- [xxHash](https://github.com/Cyan4973/xxHash)：ssrJSON 利用 xxHash 高效计算用于键缓存的哈希值。
- [klib](https://github.com/attractivechaos/klib)：ssrJSON 使用 khash 在 free-threading 构建中实现循环引用检测。
- [simdutf](https://github.com/simdutf/simdutf)：用于 `bytes` 输入的向量化 UTF-8 解码器，借鉴了 simdutf 的 UTF-8 到 UTF-16 转码模式及其查找表，并结合了 Lemire 的 `utf8_lookup4` 块校验算法。

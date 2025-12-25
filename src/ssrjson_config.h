/*==============================================================================
 Copyright (c) 2025 Antares <antares0982@gmail.com>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *============================================================================*/

#ifndef SSRJSON_CONFIG_H
#define SSRJSON_CONFIG_H

#ifdef _DEBUG
#    undef _DEBUG
#    include <Python.h>
#    define _DEBUG
#else
#    include <Python.h>
#endif
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


// feature checks
#if INTPTR_MAX == INT64_MAX
#    define SSRJSON_64BIT
#elif INTPTR_MAX == INT32_MAX
#    define SSRJSON_32BIT
#else
#    error "Unsupported platform"
#endif

/* String buffer size for decoding. Default cost: 512 * 1024 = 512kb (per thread if GIL disabled). */
#ifndef SSRJSON_STRING_BUFFER_SIZE
#    define SSRJSON_STRING_BUFFER_SIZE (512 * 1024)
#endif

/* Buffer for key associative cache. Default cost: 512 * sizeof(decode_cache_t) = 8kb (per thread if GIL disabled). */
#ifndef SSRJSON_KEY_CACHE_SIZE
#    define SSRJSON_KEY_CACHE_SIZE (1 << 9)
#endif

/*
 Buffer size for decoding object buffer.
 Default cost: 1024 * sizeof(void*) = 8kb (per thread if GIL disabled).
 */
#ifndef SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE
#    define SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE (1024)
#endif

/*
 Buffer size for decode container buffer.
 Cost: 1024 * sizeof(Py_ssize_t) = 8kb (per thread if GIL disabled).
 */
#ifndef SSRJSON_DECODE_MAX_RECURSION
#    define SSRJSON_DECODE_MAX_RECURSION (1024)
#endif

/*
 Init buffer size for encode buffer. Must be multiple of 64.
 Cost: 1kb (per thread if GIL disabled).
 */
#ifndef SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE
#    define SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE (1024)
#endif

/*
 Max nested structures for encoding.
 Cost: 1024 * sizeof(EncodeCtnWithIndex) = 16kb (per thread if GIL disabled).
 */
#ifndef SSRJSON_ENCODE_MAX_RECURSION
#    define SSRJSON_ENCODE_MAX_RECURSION (1024)
#endif

/* Whether implementation of encoding ASCII/UCS1 string to bytes is inlined. */
#ifndef SSRJSON_ENCODE_UCS1_TO_BYTES_IMPL_INLINE
#    define SSRJSON_ENCODE_UCS1_TO_BYTES_IMPL_INLINE 0
#endif

/* Whether implementation of encoding UCS2 string to bytes is inlined. */
#ifndef SSRJSON_ENCODE_UCS2_TO_BYTES_IMPL_INLINE
#    define SSRJSON_ENCODE_UCS2_TO_BYTES_IMPL_INLINE 0
#endif

/* Whether implementation of encoding UCS4 string to bytes is inlined. */
#ifndef SSRJSON_ENCODE_UCS4_TO_BYTES_IMPL_INLINE
#    define SSRJSON_ENCODE_UCS4_TO_BYTES_IMPL_INLINE 0
#endif

/* 
 * Whether to write UTF-8 cache when calling dumps_to_bytes().
 * This behavior can also be controlled by the `is_write_cache` argument,
 * or disabled/enabled globally by calling `ssrjson.write_utf8_cache()`.
 */
#ifndef SSRJSON_WRITE_UTF8_CACHE
#    define SSRJSON_WRITE_UTF8_CACHE 1
#endif

/* 
 * Whether to force strict argument parsing.
 * This behavior can also be controlled
 * globally by calling `ssrjson.strict_argparse()`.
 */
#ifndef SSRJSON_NONSTRICT_ARGPARSE
#    define SSRJSON_NONSTRICT_ARGPARSE 1
#endif

/** Type definition for primitive types. */
typedef float f32;
typedef double f64;
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef size_t usize;

typedef PyObject *pyobj_ptr_t;

/* Some feature checks. */

static_assert(SIZEOF_VOID_P == 8, "SIZEOF_VOID_P != 8");
static_assert(sizeof(PyASCIIObject) >= 16, "sizeof(PyASCIIObject) < 16");
static_assert(offsetof(PyBytesObject, ob_sval) >= 16, "offsetof(PyBytesObject, ob_sval) < 16");


#endif

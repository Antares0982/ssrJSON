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

#ifndef SSRJSON_MEMCPY_H
#define SSRJSON_MEMCPY_H

#include "ssrjson.h"

// provide inline version of memcpy.
#if __AVX512F__
#    define ssrjson_memcpy(_d, _s, _size) ssrjson_memcpy_simd((_d), (_s), (_size), 512, false)
#    define ssrjson_memcpy_prealigned(_d, _s, _size) ssrjson_memcpy_simd((_d), (_s), (_size), 512, true)
#    define SSRJSON_MEMCPY_SIMD_SIZE 64
#elif __AVX__
#    define ssrjson_memcpy(_d, _s, _size) ssrjson_memcpy_simd((_d), (_s), (_size), 256, false)
#    define ssrjson_memcpy_prealigned(_d, _s, _size) ssrjson_memcpy_simd((_d), (_s), (_size), 256, true)
#    define SSRJSON_MEMCPY_SIMD_SIZE 32
#else
#    define ssrjson_memcpy(_d, _s, _size) ssrjson_memcpy_simd((_d), (_s), (_size), 128, false)
#    define ssrjson_memcpy_prealigned(_d, _s, _size) ssrjson_memcpy_simd((_d), (_s), (_size), 128, true)
#    define SSRJSON_MEMCPY_SIMD_SIZE 16
#endif
// use libc memcpy if need noinline version.
#define ssrjson_memcpy_noinline memcpy

#if defined(_MSC_VER) && !defined(__clang__)
typedef __declspec(align(1)) struct {
    u8 v[1];
} aligned1;

typedef __declspec(align(2)) struct {
    u16 v[1];
} aligned2;

typedef __declspec(align(4)) struct {
    u32 v[1];
} aligned4;

typedef __declspec(align(8)) struct {
    u64 v[1];
} aligned8;

typedef __declspec(align(16)) struct {
    u64 v[2];
} aligned16;

typedef __declspec(align(32)) struct {
    u64 v[4];
} aligned32;

typedef __declspec(align(64)) struct {
    u64 v[8];
} aligned64;

typedef __declspec(align(128)) struct {
    u64 v[16];
} aligned128;

typedef __declspec(align(256)) struct {
    u64 v[32];
} aligned256;

typedef __declspec(align(1)) struct {
    u8 v[1];
} unaligned1;

typedef __declspec(align(1)) struct {
    u8 v[2];
} unaligned2;

typedef __declspec(align(1)) struct {
    u6 v[4];
} unaligned4;

typedef __declspec(align(1)) struct {
    u8 v[8];
} unaligned8;

typedef __declspec(align(1)) struct {
    u8 v[16];
} unaligned16;

typedef __declspec(align(1)) struct {
    u8 v[32];
} unaligned32;

typedef __declspec(align(1)) struct {
    u8 v[64];
} unaligned64;

typedef __declspec(align(1)) struct {
    u8 v[128];
} unaligned128;

typedef __declspec(align(1)) struct {
    u8 v[256];
} unaligned256;
#else
typedef u8 aligned1 __attribute__((__vector_size__(1), __aligned__(1)));
typedef u16 aligned2 __attribute__((__vector_size__(2), __aligned__(2)));
typedef u32 aligned4 __attribute__((__vector_size__(4), __aligned__(4)));
typedef u64 aligned8 __attribute__((__vector_size__(8), __aligned__(8)));
typedef u64 aligned16 __attribute__((__vector_size__(16), __aligned__(16)));
typedef u64 aligned32 __attribute__((__vector_size__(32), __aligned__(32)));
typedef u64 aligned64 __attribute__((__vector_size__(64), __aligned__(64)));
typedef u64 aligned128 __attribute__((__vector_size__(128), __aligned__(128)));
typedef u64 aligned256 __attribute__((__vector_size__(256), __aligned__(256)));

typedef u8 unaligned1 __attribute__((__vector_size__(1), __aligned__(1)));
typedef u8 unaligned2 __attribute__((__vector_size__(2), __aligned__(1)));
typedef u8 unaligned4 __attribute__((__vector_size__(4), __aligned__(1)));
typedef u8 unaligned8 __attribute__((__vector_size__(8), __aligned__(1)));
typedef u8 unaligned16 __attribute__((__vector_size__(16), __aligned__(1)));
typedef u8 unaligned32 __attribute__((__vector_size__(32), __aligned__(1)));
typedef u8 unaligned64 __attribute__((__vector_size__(64), __aligned__(1)));
typedef u8 unaligned128 __attribute__((__vector_size__(128), __aligned__(1)));
typedef u8 unaligned256 __attribute__((__vector_size__(256), __aligned__(1)));
#endif

force_inline void __ssrjson_memcpy(u8 **restrict dest_addr, const u8 **restrict src_addr, size_t n_bytes) {
    memcpy((void *)*dest_addr, (const void *)*src_addr, n_bytes);
    *dest_addr += n_bytes;
    *src_addr += n_bytes;
}

force_inline void ssrjson_memcpy_aligned_all_power2(void *restrict dest, const void *restrict src, size_t n_bytes) {
#define COPY_ALIGNED(_size)                   \
    {                                         \
        register aligned##_size __tmp;        \
        __tmp = *(const aligned##_size *)src; \
        *(aligned##_size *)dest = __tmp;      \
        break;                                \
    }

    switch (n_bytes) {
        case 1:
            COPY_ALIGNED(1);
        case 2:
            COPY_ALIGNED(2);
        case 4:
            COPY_ALIGNED(4);
        case 8:
            COPY_ALIGNED(8);
        case 16:
            COPY_ALIGNED(16);
        case 32:
            COPY_ALIGNED(32);
        case 64:
            COPY_ALIGNED(64);
        case 128:
            COPY_ALIGNED(128);
        case 256:
            COPY_ALIGNED(256);
        default: {
            SSRJSON_UNREACHABLE();
        }
    }
#undef COPY_ALIGNED
}

force_inline void ssrjson_memcpy_aligned_store_power2(void *restrict dest, const void *restrict src, size_t n_bytes) {
#define COPY_TO_ALIGNED_DST(_size)                          \
    {                                                       \
        register aligned##_size __tmp;                      \
        __tmp = (aligned##_size)(*(unaligned##_size *)src); \
        *(aligned##_size *)dest = __tmp;                    \
        break;                                              \
    }

    switch (n_bytes) {
        case 1:
            COPY_TO_ALIGNED_DST(1);
        case 2:
            COPY_TO_ALIGNED_DST(2);
        case 4:
            COPY_TO_ALIGNED_DST(4);
        case 8:
            COPY_TO_ALIGNED_DST(8);
        case 16:
            COPY_TO_ALIGNED_DST(16);
        case 32:
            COPY_TO_ALIGNED_DST(32);
        case 64:
            COPY_TO_ALIGNED_DST(64);
        case 128:
            COPY_TO_ALIGNED_DST(128);
        case 256:
            COPY_TO_ALIGNED_DST(256);
        default: {
            SSRJSON_UNREACHABLE();
        }
    }
#undef COPY_TO_ALIGNED_DST
}

force_inline void __ssrjson_memcpy_aligned_store_power2(u8 **restrict dest_addr, const u8 **restrict src_addr, size_t n_bytes) {
    ssrjson_memcpy_aligned_store_power2(*dest_addr, *src_addr, n_bytes);
    *dest_addr += n_bytes;
    *src_addr += n_bytes;
}

force_inline void __ssrjson_short_memcpy_small_first(u8 **restrict dest_addr, const u8 **restrict src_addr, size_t n_bytes, size_t size_less_than) {
    if (size_less_than >= 2 && (n_bytes & 1)) __ssrjson_memcpy(dest_addr, src_addr, 1);
    if (size_less_than >= 4 && (n_bytes & 2)) __ssrjson_memcpy(dest_addr, src_addr, 2);
    if (size_less_than >= 8 && (n_bytes & 4)) __ssrjson_memcpy(dest_addr, src_addr, 4);
    if (size_less_than >= 16 && (n_bytes & 8)) __ssrjson_memcpy(dest_addr, src_addr, 8);
    if (size_less_than >= 32 && (n_bytes & 16)) __ssrjson_memcpy(dest_addr, src_addr, 16);
    if (size_less_than >= 64 && (n_bytes & 32)) __ssrjson_memcpy(dest_addr, src_addr, 32);
}

force_inline void __ssrjson_short_memcpy_small_first_aligned_store(u8 **restrict dest_addr, const u8 **restrict src_addr, size_t n_bytes, size_t size_less_than) {
    if (size_less_than >= 2 && (n_bytes & 1)) __ssrjson_memcpy_aligned_store_power2(dest_addr, src_addr, 1);
    if (size_less_than >= 4 && (n_bytes & 2)) __ssrjson_memcpy_aligned_store_power2(dest_addr, src_addr, 2);
    if (size_less_than >= 8 && (n_bytes & 4)) __ssrjson_memcpy_aligned_store_power2(dest_addr, src_addr, 4);
    if (size_less_than >= 16 && (n_bytes & 8)) __ssrjson_memcpy_aligned_store_power2(dest_addr, src_addr, 8);
    if (size_less_than >= 32 && (n_bytes & 16)) __ssrjson_memcpy_aligned_store_power2(dest_addr, src_addr, 16);
    if (size_less_than >= 64 && (n_bytes & 32)) __ssrjson_memcpy_aligned_store_power2(dest_addr, src_addr, 32);
}

force_inline void __ssrjson_short_memcpy_large_first(u8 **restrict dest_addr, const u8 **restrict src_addr, size_t n_bytes, size_t size_less_than) {
    if (size_less_than >= 64 && (n_bytes & 32)) __ssrjson_memcpy_aligned_store_power2(dest_addr, src_addr, 32);
    if (size_less_than >= 32 && (n_bytes & 16)) __ssrjson_memcpy_aligned_store_power2(dest_addr, src_addr, 16);
    if (size_less_than >= 16 && (n_bytes & 8)) __ssrjson_memcpy_aligned_store_power2(dest_addr, src_addr, 8);
    if (size_less_than >= 8 && (n_bytes & 4)) __ssrjson_memcpy_aligned_store_power2(dest_addr, src_addr, 4);
    if (size_less_than >= 4 && (n_bytes & 2)) __ssrjson_memcpy_aligned_store_power2(dest_addr, src_addr, 2);
    if (size_less_than >= 2 && (n_bytes & 1)) __ssrjson_memcpy_aligned_store_power2(dest_addr, src_addr, 1);
}

force_inline void ssrjson_memcpy_simd(void *restrict dest, const void *restrict src, size_t n_bytes, size_t per_cpy_bitsize, bool prealigned) {
    const size_t per_cpy_bytesize = per_cpy_bitsize / 8;
    assert(per_cpy_bytesize && ((per_cpy_bytesize - 1) & per_cpy_bytesize) == 0);
    u8 *d = (u8 *)dest;
    uintptr_t d_int = (uintptr_t)d;
    const u8 *s = (const u8 *)src;
    uintptr_t s_int = (uintptr_t)s;
    size_t n = n_bytes;

    // align dest to 256-bits
    if (d_int & (per_cpy_bytesize - 1)) {
        size_t tmp = per_cpy_bytesize - (d_int & (per_cpy_bytesize - 1));
        if (tmp >= n) {
            __ssrjson_short_memcpy_small_first(&d, &s, n, per_cpy_bytesize);
            return;
        }
        size_t nh = tmp;
        __ssrjson_short_memcpy_small_first_aligned_store(&d, &s, nh, per_cpy_bytesize);
        s_int += nh;
        n -= nh;
    }

    if (!prealigned && (s_int & (per_cpy_bytesize - 1))) { // src is not aligned to 256-bits
        // unroll 4
        while (n >= 4 * per_cpy_bytesize) {
            ssrjson_memcpy_aligned_store_power2(d, s, 4 * per_cpy_bytesize);
            s += 4 * per_cpy_bytesize;
            d += 4 * per_cpy_bytesize;
            n -= 4 * per_cpy_bytesize;
        }
        while (n >= per_cpy_bytesize) {
            ssrjson_memcpy_aligned_store_power2(d, s, per_cpy_bytesize);
            s += per_cpy_bytesize;
            d += per_cpy_bytesize;
            n -= per_cpy_bytesize;
        }
    } else { // or it IS aligned
        // unroll 4
        while (n >= 4 * per_cpy_bytesize) {
            ssrjson_memcpy_aligned_all_power2(d, s, 4 * per_cpy_bytesize);
            s += 4 * per_cpy_bytesize;
            d += 4 * per_cpy_bytesize;
            n -= 4 * per_cpy_bytesize;
        }
        while (n >= per_cpy_bytesize) {
            ssrjson_memcpy_aligned_all_power2(d, s, per_cpy_bytesize);
            s += per_cpy_bytesize;
            d += per_cpy_bytesize;
            n -= per_cpy_bytesize;
        }
    }
    if (n) __ssrjson_short_memcpy_large_first(&d, &s, n, per_cpy_bytesize);
}

#endif // SSRJSON_MEMCPY_H

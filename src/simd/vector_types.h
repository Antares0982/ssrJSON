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

#ifndef SSRJSON_SIMD_VECTOR_TYPES_H
#define SSRJSON_SIMD_VECTOR_TYPES_H
#include "ssrjson.h"

/* Common SIMD vector types. */
#if defined(_MSC_VER) && !defined(__clang__)
typedef u32 vector_a_u8_32;

typedef __declspec(align(1)) struct {
    u8 v[4];
} vector_u_u8_32;

typedef u32 vector_a_u16_32;

typedef __declspec(align(2)) struct {
    u16 v[2];
} vector_u_u16_32;

typedef u32 vector_a_u32_32;

typedef __declspec(align(4)) struct {
    u32 v[1];
} vector_u_u32_32;

typedef u64 vector_a_u8_64;

typedef __declspec(align(1)) struct {
    u8 v[8];
} vector_u_u8_64;

typedef u64 vector_a_u16_64;

typedef __declspec(align(2)) struct {
    u16 v[4];
} vector_u_u16_64;

typedef u64 vector_a_u32_64;

typedef __declspec(align(4)) struct {
    u32 v[2];
} vector_u_u32_64;

typedef __m128i vector_a_u8_128;

typedef __declspec(align(1)) struct {
    u8 v[16];
} vector_u_u8_128;

typedef __m128i vector_a_u16_128;

typedef __declspec(align(2)) struct {
    u16 v[8];
} vector_u_u16_128;

typedef __m128i vector_a_u32_128;

typedef __declspec(align(4)) struct {
    u32 v[4];
} vector_u_u32_128;

typedef __m256i vector_a_u8_256;

typedef __declspec(align(1)) struct {
    u8 v[32];
} vector_u_u8_256;

typedef __m256i vector_a_u16_256;

typedef __declspec(align(2)) struct {
    u16 v[16];
} vector_u_u16_256;

typedef __m256i vector_a_u32_256;

typedef __declspec(align(4)) struct {
    u32 v[8];
} vector_u_u32_256;

typedef __m512i vector_a_u8_512;

typedef __declspec(align(1)) struct {
    u8 v[64];
} vector_u_u8_512;

typedef __m512i vector_a_u16_512;

typedef __declspec(align(2)) struct {
    u16 v[32];
} vector_u_u16_512;

typedef __m512i vector_a_u32_512;

typedef __declspec(align(4)) struct {
    u32 v[16];
} vector_u_u32_512;

#elif SSRJSON_AARCH
#    include <arm_neon.h>
// smaller than 128
typedef u8 vector_a_u8_32 __attribute__((__vector_size__(4), __aligned__(4)));
typedef u8 vector_u_u8_32 __attribute__((__vector_size__(4), __aligned__(1)));
typedef u16 vector_a_u16_32 __attribute__((__vector_size__(4), __aligned__(4)));
typedef u16 vector_u_u16_32 __attribute__((__vector_size__(4), __aligned__(2)));
typedef u32 vector_a_u32_32 __attribute__((__vector_size__(4), __aligned__(4)));
typedef u32 vector_u_u32_32 __attribute__((__vector_size__(4), __aligned__(4)));
// 64~128
typedef uint8x8_t vector_a_u8_64;
typedef u8 vector_u_u8_64 __attribute__((__vector_size__(8), __aligned__(1)));
typedef uint16x4_t vector_a_u16_64;
typedef u16 vector_u_u16_64 __attribute__((__vector_size__(8), __aligned__(2)));
typedef uint32x2_t vector_a_u32_64;
typedef u32 vector_u_u32_64 __attribute__((__vector_size__(8), __aligned__(4)));
typedef uint8x16_t vector_a_u8_128;
typedef u8 vector_u_u8_128 __attribute__((__vector_size__(16), __aligned__(1)));
typedef uint16x8_t vector_a_u16_128;
typedef u16 vector_u_u16_128 __attribute__((__vector_size__(16), __aligned__(2)));
typedef uint32x4_t vector_a_u32_128;
typedef u32 vector_u_u32_128 __attribute__((__vector_size__(16), __aligned__(4)));
// larger than 128
typedef u8 vector_a_u8_256 __attribute__((__vector_size__(32), __aligned__(32)));
typedef u8 vector_u_u8_256 __attribute__((__vector_size__(32), __aligned__(1)));
typedef u16 vector_a_u16_256 __attribute__((__vector_size__(32), __aligned__(32)));
typedef u16 vector_u_u16_256 __attribute__((__vector_size__(32), __aligned__(2)));
typedef u32 vector_a_u32_256 __attribute__((__vector_size__(32), __aligned__(32)));
typedef u32 vector_u_u32_256 __attribute__((__vector_size__(32), __aligned__(4)));
typedef u8 vector_a_u8_512 __attribute__((__vector_size__(64), __aligned__(64)));
typedef u8 vector_u_u8_512 __attribute__((__vector_size__(64), __aligned__(1)));
typedef u16 vector_a_u16_512 __attribute__((__vector_size__(64), __aligned__(64)));
typedef u16 vector_u_u16_512 __attribute__((__vector_size__(64), __aligned__(2)));
typedef u32 vector_a_u32_512 __attribute__((__vector_size__(64), __aligned__(64)));
typedef u32 vector_u_u32_512 __attribute__((__vector_size__(64), __aligned__(4)));

typedef u8 vector_a_u8_1024 __attribute__((__vector_size__(128), __aligned__(128)));
typedef u8 vector_u_u8_1024 __attribute__((__vector_size__(128), __aligned__(1)));
typedef u16 vector_a_u16_1024 __attribute__((__vector_size__(128), __aligned__(128)));
typedef u16 vector_u_u16_1024 __attribute__((__vector_size__(128), __aligned__(2)));
typedef u32 vector_a_u32_1024 __attribute__((__vector_size__(128), __aligned__(128)));
typedef u32 vector_u_u32_1024 __attribute__((__vector_size__(128), __aligned__(4)));

typedef u8 vector_a_u8_2048 __attribute__((__vector_size__(256), __aligned__(256)));
typedef u8 vector_u_u8_2048 __attribute__((__vector_size__(256), __aligned__(1)));
typedef u16 vector_a_u16_2048 __attribute__((__vector_size__(256), __aligned__(256)));
typedef u16 vector_u_u16_2048 __attribute__((__vector_size__(256), __aligned__(2)));
typedef u32 vector_a_u32_2048 __attribute__((__vector_size__(256), __aligned__(256)));
typedef u32 vector_u_u32_2048 __attribute__((__vector_size__(256), __aligned__(4)));
#else
typedef u8 vector_a_u8_32 __attribute__((__vector_size__(4), __aligned__(4)));
typedef u8 vector_u_u8_32 __attribute__((__vector_size__(4), __aligned__(1)));
typedef u16 vector_a_u16_32 __attribute__((__vector_size__(4), __aligned__(4)));
typedef u16 vector_u_u16_32 __attribute__((__vector_size__(4), __aligned__(2)));
typedef u32 vector_a_u32_32 __attribute__((__vector_size__(4), __aligned__(4)));
typedef u32 vector_u_u32_32 __attribute__((__vector_size__(4), __aligned__(4)));
typedef u8 vector_a_u8_64 __attribute__((__vector_size__(8), __aligned__(8)));
typedef u8 vector_u_u8_64 __attribute__((__vector_size__(8), __aligned__(1)));
typedef u16 vector_a_u16_64 __attribute__((__vector_size__(8), __aligned__(8)));
typedef u16 vector_u_u16_64 __attribute__((__vector_size__(8), __aligned__(2)));
typedef u32 vector_a_u32_64 __attribute__((__vector_size__(8), __aligned__(8)));
typedef u32 vector_u_u32_64 __attribute__((__vector_size__(8), __aligned__(4)));
typedef u8 vector_a_u8_128 __attribute__((__vector_size__(16), __aligned__(16)));
typedef u8 vector_u_u8_128 __attribute__((__vector_size__(16), __aligned__(1)));
typedef u16 vector_a_u16_128 __attribute__((__vector_size__(16), __aligned__(16)));
typedef u16 vector_u_u16_128 __attribute__((__vector_size__(16), __aligned__(2)));
typedef u32 vector_a_u32_128 __attribute__((__vector_size__(16), __aligned__(16)));
typedef u32 vector_u_u32_128 __attribute__((__vector_size__(16), __aligned__(4)));
typedef u8 vector_a_u8_256 __attribute__((__vector_size__(32), __aligned__(32)));
typedef u8 vector_u_u8_256 __attribute__((__vector_size__(32), __aligned__(1)));
typedef u16 vector_a_u16_256 __attribute__((__vector_size__(32), __aligned__(32)));
typedef u16 vector_u_u16_256 __attribute__((__vector_size__(32), __aligned__(2)));
typedef u32 vector_a_u32_256 __attribute__((__vector_size__(32), __aligned__(32)));
typedef u32 vector_u_u32_256 __attribute__((__vector_size__(32), __aligned__(4)));
typedef u8 vector_a_u8_512 __attribute__((__vector_size__(64), __aligned__(64)));
typedef u8 vector_u_u8_512 __attribute__((__vector_size__(64), __aligned__(1)));
typedef u16 vector_a_u16_512 __attribute__((__vector_size__(64), __aligned__(64)));
typedef u16 vector_u_u16_512 __attribute__((__vector_size__(64), __aligned__(2)));
typedef u32 vector_a_u32_512 __attribute__((__vector_size__(64), __aligned__(64)));
typedef u32 vector_u_u32_512 __attribute__((__vector_size__(64), __aligned__(4)));

typedef u8 vector_a_u8_1024 __attribute__((__vector_size__(128), __aligned__(128)));
typedef u8 vector_u_u8_1024 __attribute__((__vector_size__(128), __aligned__(1)));
typedef u16 vector_a_u16_1024 __attribute__((__vector_size__(128), __aligned__(128)));
typedef u16 vector_u_u16_1024 __attribute__((__vector_size__(128), __aligned__(2)));
typedef u32 vector_a_u32_1024 __attribute__((__vector_size__(128), __aligned__(128)));
typedef u32 vector_u_u32_1024 __attribute__((__vector_size__(128), __aligned__(4)));

typedef u8 vector_a_u8_2048 __attribute__((__vector_size__(256), __aligned__(256)));
typedef u8 vector_u_u8_2048 __attribute__((__vector_size__(256), __aligned__(1)));
typedef u16 vector_a_u16_2048 __attribute__((__vector_size__(256), __aligned__(256)));
typedef u16 vector_u_u16_2048 __attribute__((__vector_size__(256), __aligned__(2)));
typedef u32 vector_a_u32_2048 __attribute__((__vector_size__(256), __aligned__(256)));
typedef u32 vector_u_u32_2048 __attribute__((__vector_size__(256), __aligned__(4)));
#endif

#endif // SSRJSON_SIMD_VECTOR_TYPES_H

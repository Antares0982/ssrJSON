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

#ifndef SSRJSON_UNION_VECTOR_H
#define SSRJSON_UNION_VECTOR_H
#include "vector_types.h"

typedef union {
    vector_a_u8_128 x[2];
    vector_a_u8_256 y;
} unionvector_a_u8_128_x2;

typedef union {
    vector_a_u16_128 x[2];
    vector_a_u16_256 y;
} unionvector_a_u16_128_x2;

typedef union {
    vector_a_u32_128 x[2];
    vector_a_u32_256 y;
} unionvector_a_u32_128_x2;

typedef union {
    vector_a_u8_128 x[4];
    vector_a_u8_256 y[2];
    vector_a_u8_512 z;
} unionvector_a_u8_128_x4;

typedef union {
    vector_a_u16_128 x[4];
    vector_a_u16_256 y[2];
    vector_a_u16_512 z;
} unionvector_a_u16_128_x4;

typedef union {
    vector_a_u32_128 x[4];
    vector_a_u32_256 y[2];
    vector_a_u32_512 z;
} unionvector_a_u32_128_x4;

typedef union {
    vector_a_u8_256 x[2];
    vector_a_u8_512 y;
} unionvector_a_u8_256_x2;

typedef union {
    vector_a_u16_256 x[2];
    vector_a_u16_512 y;
} unionvector_a_u16_256_x2;

typedef union {
    vector_a_u32_256 x[2];
    vector_a_u32_512 y;
} unionvector_a_u32_256_x2;

typedef union {
    vector_a_u8_256 x[4];
    vector_a_u8_512 y[2];
    vector_a_u8_1024 z;
} unionvector_a_u8_256_x4;

typedef union {
    vector_a_u16_256 x[4];
    vector_a_u16_512 y[2];
    vector_a_u16_1024 z;
} unionvector_a_u16_256_x4;

typedef union {
    vector_a_u32_256 x[4];
    vector_a_u32_512 y[2];
    vector_a_u32_1024 z;
} unionvector_a_u32_256_x4;

typedef union {
    vector_a_u8_512 x[2];
    vector_a_u8_1024 y;
} unionvector_a_u8_512_x2;

typedef union {
    vector_a_u16_512 x[2];
    vector_a_u16_1024 y;
} unionvector_a_u16_512_x2;

typedef union {
    vector_a_u32_512 x[2];
    vector_a_u32_1024 y;
} unionvector_a_u32_512_x2;

typedef union {
    vector_a_u8_512 x[4];
    vector_a_u8_1024 y[2];
    vector_a_u8_2048 z;
} unionvector_a_u8_512_x4;

typedef union {
    vector_a_u16_512 x[4];
    vector_a_u16_1024 y[2];
    vector_a_u16_2048 z;
} unionvector_a_u16_512_x4;

typedef union {
    vector_a_u32_512 x[4];
    vector_a_u32_1024 y[2];
    vector_a_u32_2048 z;
} unionvector_a_u32_512_x4;

#endif // SSRJSON_UNION_VECTOR_H

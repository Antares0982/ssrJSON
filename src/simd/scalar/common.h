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

#ifndef SSRJSON_SCALAR_COMMON_H
#define SSRJSON_SCALAR_COMMON_H
#include "simd/union_vector.h"
#include "simd/vector_types.h"

force_inline vector_a_u8_128 setzero_128(void) { return (vector_a_u8_128){0}; }

force_inline bool scalar_testz(vector_a_u8_128 x) {
    for (usize i = 0; i < 16; ++i)
        if (x[i]) return false;
    return true;
}

#define testz_128(x) scalar_testz((vector_a_u8_128)(x))
#define testz2_128(a, b) testz_128((a) & (b))

force_inline vector_a_u8_128 scalar_shuffle(vector_a_u8_128 x, vector_a_u8_128 mask) {
    vector_a_u8_128 out = {0};
    for (usize i = 0; i < 16; ++i) out[i] = mask[i] < 16 ? x[mask[i]] : 0;
    return out;
}

#define shuffle_128(x, m) scalar_shuffle((vector_a_u8_128)(x), (vector_a_u8_128)(m))

force_inline vector_a_u8_128 scalar_shift(vector_a_u8_128 x, int count) {
    vector_a_u8_128 out = {0};
    for (int i = 0; i < 16; ++i)
        if (i + count >= 0 && i + count < 16) out[i] = x[i + count];
    return out;
}

#define runtime_byte_rshift_128(x, n) scalar_shift((vector_a_u8_128)(x), (n))
#define byte_rshift_128(x, n) runtime_byte_rshift_128(x, n)
#define byte_lshift_128(x, n) runtime_byte_rshift_128(x, -(n))

force_inline vector_a_u8_128 scalar_align(vector_a_u8_128 a, vector_a_u8_128 b, int n) {
    vector_a_u8_128 out = {0};
    for (int i = 0; i < 16; ++i) out[i] = i + n < 16 ? a[i + n] : b[i + n - 16];
    return out;
}

#define alignr_128(a, b, n) scalar_align((vector_a_u8_128)(a), (vector_a_u8_128)(b), n)

force_inline u16 get_bitmask_from_u8_128(vector_a_u8_128 x) {
    u16 mask = 0;
    for (usize i = 0; i < 16; ++i) mask |= (u16)(x[i] >> 7) << i;
    return mask;
}

force_inline vector_a_u8_128 broadcast_u8_128(u8 x) {
    vector_a_u8_128 out;
    for (usize i = 0; i < 16; ++i) out[i] = x;
    return out;
}

force_inline vector_a_u8_128 unsigned_max_u8_128(vector_a_u8_128 a, vector_a_u8_128 b) {
    vector_a_u8_128 out;
    for (usize i = 0; i < 16; ++i) out[i] = a[i] > b[i] ? a[i] : b[i];
    return out;
}

force_inline vector_a_u8_128 unsigned_saturate_minus_u8_128(vector_a_u8_128 a, vector_a_u8_128 b) {
    vector_a_u8_128 out;
    for (usize i = 0; i < 16; ++i) out[i] = a[i] > b[i] ? a[i] - b[i] : 0;
    return out;
}

#define cmpeq_u8_128(a, b) ((a) == (b))

force_inline vector_a_u8_128 signed_cmpgt_u8_128(vector_a_u8_128 a, vector_a_u8_128 b) {
    vector_a_u8_128 out;
    for (usize i = 0; i < 16; ++i) out[i] = (i8)a[i] > (i8)b[i] ? (u8)-1 : 0;
    return out;
}

#define signed_cmplt_u8_128(a, b) signed_cmpgt_u8_128(b, a)
#define rshift_u8_128(x, n) ((x) >> (n))
#define lshift_u8_128(x, n) ((x) << (n))

force_inline bool checkmax_u8_128(vector_a_u8_128 x, u8 limit) {
    for (usize i = 0; i < 16; ++i)
        if (x[i] > limit) return false;
    return true;
}

force_inline vector_a_u16_128 broadcast_u16_128(u16 x) {
    vector_a_u16_128 out;
    for (usize i = 0; i < 8; ++i) out[i] = x;
    return out;
}

force_inline vector_a_u16_128 unsigned_max_u16_128(vector_a_u16_128 a, vector_a_u16_128 b) {
    vector_a_u16_128 out;
    for (usize i = 0; i < 8; ++i) out[i] = a[i] > b[i] ? a[i] : b[i];
    return out;
}

force_inline vector_a_u16_128 unsigned_saturate_minus_u16_128(vector_a_u16_128 a, vector_a_u16_128 b) {
    vector_a_u16_128 out;
    for (usize i = 0; i < 8; ++i) out[i] = a[i] > b[i] ? a[i] - b[i] : 0;
    return out;
}

#define cmpeq_u16_128(a, b) ((a) == (b))

force_inline vector_a_u16_128 signed_cmpgt_u16_128(vector_a_u16_128 a, vector_a_u16_128 b) {
    vector_a_u16_128 out;
    for (usize i = 0; i < 8; ++i) out[i] = (i16)a[i] > (i16)b[i] ? (u16)-1 : 0;
    return out;
}

#define signed_cmplt_u16_128(a, b) signed_cmpgt_u16_128(b, a)
#define rshift_u16_128(x, n) ((x) >> (n))
#define lshift_u16_128(x, n) ((x) << (n))

force_inline bool checkmax_u16_128(vector_a_u16_128 x, u16 limit) {
    for (usize i = 0; i < 8; ++i)
        if (x[i] > limit) return false;
    return true;
}

force_inline vector_a_u32_128 broadcast_u32_128(u32 x) {
    vector_a_u32_128 out;
    for (usize i = 0; i < 4; ++i) out[i] = x;
    return out;
}

force_inline vector_a_u32_128 unsigned_max_u32_128(vector_a_u32_128 a, vector_a_u32_128 b) {
    vector_a_u32_128 out;
    for (usize i = 0; i < 4; ++i) out[i] = a[i] > b[i] ? a[i] : b[i];
    return out;
}

force_inline vector_a_u32_128 unsigned_saturate_minus_u32_128(vector_a_u32_128 a, vector_a_u32_128 b) {
    vector_a_u32_128 out;
    for (usize i = 0; i < 4; ++i) out[i] = a[i] > b[i] ? a[i] - b[i] : 0;
    return out;
}

#define cmpeq_u32_128(a, b) ((a) == (b))

force_inline vector_a_u32_128 signed_cmpgt_u32_128(vector_a_u32_128 a, vector_a_u32_128 b) {
    vector_a_u32_128 out;
    for (usize i = 0; i < 4; ++i) out[i] = (i32)a[i] > (i32)b[i] ? (u32)-1 : 0;
    return out;
}

#define signed_cmplt_u32_128(a, b) signed_cmpgt_u32_128(b, a)
#define rshift_u32_128(x, n) ((x) >> (n))
#define lshift_u32_128(x, n) ((x) << (n))

force_inline bool checkmax_u32_128(vector_a_u32_128 x, u32 limit) {
    for (usize i = 0; i < 4; ++i)
        if (x[i] > limit) return false;
    return true;
}

force_inline void cvt_to_dst_u8_u8_128(u8 *dst, vector_a_u8_128 x) {
    for (usize i = 0; i < 16; ++i) dst[i] = (u8)x[i];
}

force_inline void cvt_to_dst_u8_u16_128(u16 *dst, vector_a_u8_128 x) {
    for (usize i = 0; i < 16; ++i) dst[i] = (u16)x[i];
}

force_inline vector_a_u16_128 cvt_u8_to_u16_128(vector_a_u8_128 x) {
    vector_a_u16_128 out;
    for (usize i = 0; i < 8; ++i) out[i] = (u16)x[i];
    return out;
}

force_inline void cvt_to_dst_u8_u32_128(u32 *dst, vector_a_u8_128 x) {
    for (usize i = 0; i < 16; ++i) dst[i] = (u32)x[i];
}

force_inline vector_a_u32_128 cvt_u8_to_u32_128(vector_a_u8_128 x) {
    vector_a_u32_128 out;
    for (usize i = 0; i < 4; ++i) out[i] = (u32)x[i];
    return out;
}

force_inline void cvt_to_dst_u16_u8_128(u8 *dst, vector_a_u16_128 x) {
    for (usize i = 0; i < 8; ++i) dst[i] = (u8)x[i];
}

force_inline vector_a_u8_64 cvt_u16_to_u8_128(vector_a_u16_128 x) {
    vector_a_u8_64 out;
    for (usize i = 0; i < 8; ++i) out[i] = (u8)x[i];
    return out;
}

force_inline void cvt_to_dst_u16_u16_128(u16 *dst, vector_a_u16_128 x) {
    for (usize i = 0; i < 8; ++i) dst[i] = (u16)x[i];
}

force_inline void cvt_to_dst_u16_u32_128(u32 *dst, vector_a_u16_128 x) {
    for (usize i = 0; i < 8; ++i) dst[i] = (u32)x[i];
}

force_inline vector_a_u32_128 cvt_u16_to_u32_128(vector_a_u16_128 x) {
    vector_a_u32_128 out;
    for (usize i = 0; i < 4; ++i) out[i] = (u32)x[i];
    return out;
}

force_inline void cvt_to_dst_u32_u8_128(u8 *dst, vector_a_u32_128 x) {
    for (usize i = 0; i < 4; ++i) dst[i] = (u8)x[i];
}

force_inline vector_a_u8_32 cvt_u32_to_u8_128(vector_a_u32_128 x) {
    vector_a_u8_32 out;
    for (usize i = 0; i < 4; ++i) out[i] = (u8)x[i];
    return out;
}

force_inline void cvt_to_dst_u32_u16_128(u16 *dst, vector_a_u32_128 x) {
    for (usize i = 0; i < 4; ++i) dst[i] = (u16)x[i];
}

force_inline vector_a_u16_64 cvt_u32_to_u16_128(vector_a_u32_128 x) {
    vector_a_u16_64 out;
    for (usize i = 0; i < 4; ++i) out[i] = (u16)x[i];
    return out;
}

force_inline void cvt_to_dst_u32_u32_128(u32 *dst, vector_a_u32_128 x) {
    for (usize i = 0; i < 4; ++i) dst[i] = (u32)x[i];
}

#endif

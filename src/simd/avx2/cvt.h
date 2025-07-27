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

#ifndef SSRJSON_SIMD_AVX2_CVT_H
#define SSRJSON_SIMD_AVX2_CVT_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "common.h"
#include "simd/avx/cvt.h"
#include "simd/sse2/checker.h"
#include "simd/sse4.1/common.h"

force_inline const void *read_tail_mask_table_8(Py_ssize_t);
#if __AVX512F__ && __AVX512CD__
force_inline vector_a_u32_512 cvt_u8_to_u32_512(vector_a_u8_128 x);
force_inline vector_a_u32_512 cvt_u16_to_u32_512(vector_a_u16_256 y);
force_inline u64 get_high_bitmask_512(usize len);
#endif
#if __AVX512VL__ && __AVX512DQ__ && __AVX512BW__
force_inline vector_a_u16_512 cvt_u8_to_u16_512(vector_a_u8_256 y);
#endif

force_inline vector_a_u16_256 cvt_u8_to_u16_256(vector_a_u8_128 x) {
    return _mm256_cvtepu8_epi16(x);
}

force_inline vector_a_u32_256 cvt_u8_to_u32_256(vector_a_u8_128 x) {
    return _mm256_cvtepu8_epi32(x);
}

force_inline vector_a_u32_256 cvt_u16_to_u32_256(vector_a_u16_128 x) {
    return _mm256_cvtepu16_epi32(x);
}

// cvt up

force_inline void cvt_to_dst_u8_u16_256(u16 *dst, vector_a_u8_256 y) {
#if __AVX512VL__ && __AVX512DQ__ && __AVX512BW__
    *(vector_u_u16_512 *)dst = cvt_u8_to_u16_512(y);
#else
    *(vector_u_u16_256 *)(dst + 0) = cvt_u8_to_u16_256(extract_128_from_256(y, 0));
    *(vector_u_u16_256 *)(dst + 16) = cvt_u8_to_u16_256(extract_128_from_256(y, 1));
#endif
}

force_inline void cvt_to_dst_u8_u32_256(u32 *dst, vector_a_u8_256 y) {
    vector_a_u8_128 x1, x2;
    x1 = extract_128_from_256(y, 0);
    x2 = extract_128_from_256(y, 1);
#if __AVX512F__ && __AVX512CD__
    *(vector_u_u32_512 *)(dst + 0) = cvt_u8_to_u32_512(x1);
    *(vector_u_u32_512 *)(dst + 16) = cvt_u8_to_u32_512(x2);
#else
    *(vector_u_u32_256 *)(dst + 0) = cvt_u8_to_u32_256(x1);
    *(vector_u_u32_256 *)(dst + 8) = cvt_u8_to_u32_256(byte_rshift_128(x1, 8));
    *(vector_u_u32_256 *)(dst + 16) = cvt_u8_to_u32_256(x2);
    *(vector_u_u32_256 *)(dst + 24) = cvt_u8_to_u32_256(byte_rshift_128(x2, 8));
#endif
}

force_inline void cvt_to_dst_u16_u32_256(u32 *dst, vector_a_u16_256 y) {
#if __AVX512F__ && __AVX512CD__
    *(vector_u_u32_512 *)dst = cvt_u16_to_u32_512(y);
#else
    *(vector_u_u32_256 *)(dst + 0) = cvt_u16_to_u32_256(extract_128_from_256(y, 0));
    *(vector_u_u32_256 *)(dst + 8) = cvt_u16_to_u32_256(extract_128_from_256(y, 1));
#endif
}

// cvt down

force_inline void cvt_to_dst_u16_u8_256(u8 *dst, vector_a_u16_256 y) {
    *(vector_u_u8_128 *)dst = cvt_u16_to_u8_256(y);
}

force_inline void cvt_to_dst_u32_u8_256(u8 *dst, vector_a_u32_256 y) {
    *(vector_u_u8_64 *)dst = cvt_u32_to_u8_256(y);
}

force_inline void cvt_to_dst_u32_u16_256(u16 *dst, vector_a_u32_256 y) {
    *(vector_u_u16_128 *)dst = cvt_u32_to_u16_256(y);
}

// cvt same size (blend high)
force_inline void cvt_to_dst_blendhigh_u8_u8_256(u8 *dst, vector_a_u8_256 y, usize len) {
#if __AVX512VL__ && __AVX512DQ__ && __AVX512BW__
    _mm256_mask_storeu_epi8(dst, get_high_bitmask_256(len), y);
#else
    vector_u_u8_256 *uvec = (vector_u_u8_256 *)dst;
    *uvec = blendv_256(*uvec, y, get_high_mask_u8_256(len));
#endif
}

force_inline void cvt_to_dst_blendhigh_u16_u16_256(u16 *dst, vector_a_u16_256 y, usize len) {
#if __AVX512VL__ && __AVX512DQ__ && __AVX512BW__
    _mm256_mask_storeu_epi16(dst, (u16)get_high_bitmask_256(len), y);
#else
    vector_u_u16_256 *uvec = (vector_u_u16_256 *)dst;
    *uvec = blendv_256(*uvec, y, get_high_mask_u16_256(len));
#endif
}

force_inline void cvt_to_dst_blendhigh_u32_u32_256(u32 *dst, vector_a_u32_256 y, usize len) {
#if __AVX512VL__ && __AVX512DQ__ && __AVX512BW__
    _mm256_mask_storeu_epi32(dst, (u8)get_high_bitmask_256(len), y);
#else
    vector_u_u32_256 *uvec = (vector_u_u32_256 *)dst;
    *uvec = blendv_256(*uvec, y, get_high_mask_u32_256(len));
#endif
}

// cvt up (blend high)

force_inline void cvt_to_dst_blendhigh_u8_u16_256(u16 *dst, vector_a_u8_256 y, usize len) {
#if __AVX512VL__ && __AVX512DQ__ && __AVX512BW__
    vector_a_u16_512 w = cvt_u8_to_u16_512(y);
    _mm512_mask_storeu_epi16(dst, get_high_bitmask_512(len), w);
#else
    vector_a_u8_128 x1, x2;
    x1 = extract_128_from_256(y, 0);
    x2 = extract_128_from_256(y, 1);
    BLEND_HIGH_WRITER_2PARTS(dst, vector_u_u16_256, 256 / 8 / sizeof(u8), len, blendv_256, get_high_mask_u16_256, cvt_u8_to_u16_256(x1), cvt_u8_to_u16_256(x2));
#endif
}

force_inline void cvt_to_dst_blendhigh_u8_u32_256(u32 *dst, vector_a_u8_256 y, usize len) {
    vector_a_u8_128 x1, x2;
    x1 = extract_128_from_256(y, 0);
    x2 = extract_128_from_256(y, 1);
#if __AVX512F__ && __AVX512CD__
    usize part1, part2;
    split_tail_len_two_parts(len, 256 / 8 / sizeof(u8), &part1, &part2);
    _mm512_mask_storeu_epi32(dst + 0, get_high_bitmask_512(part1), cvt_u8_to_u32_512(x1));
    _mm512_mask_storeu_epi32(dst + 16, get_high_bitmask_512(part2), cvt_u8_to_u32_512(x2));
#else
#    define _EXPR0_ cvt_u8_to_u32_256(x1)
#    define _EXPR1_ cvt_u8_to_u32_256(byte_rshift_128(x1, 8))
#    define _EXPR2_ cvt_u8_to_u32_256(x2)
#    define _EXPR3_ cvt_u8_to_u32_256(byte_rshift_128(x2, 8))
    BLEND_HIGH_WRITER_4PARTS(dst, vector_u_u32_256, 256 / 8 / sizeof(u8), len, blendv_256, get_high_mask_u32_256, _EXPR0_, _EXPR1_, _EXPR2_, _EXPR3_);
#    undef _EXPR0_
#    undef _EXPR1_
#    undef _EXPR2_
#    undef _EXPR3_
#endif
}

force_inline void cvt_to_dst_blendhigh_u16_u32_256(u32 *dst, vector_a_u16_256 y, usize len) {
#if __AVX512VL__ && __AVX512DQ__ && __AVX512BW__
    vector_a_u32_512 w = cvt_u16_to_u32_512(y);
    _mm512_mask_storeu_epi32(dst, (u16)get_high_bitmask_512(len), w);
#else
    vector_a_u16_128 x1, x2;
    x1 = extract_128_from_256(y, 0);
    x2 = extract_128_from_256(y, 1);
    BLEND_HIGH_WRITER_2PARTS(dst, vector_u_u32_256, 256 / 8 / sizeof(u16), len, blendv_256, get_high_mask_u32_256, cvt_u16_to_u32_256(x1), cvt_u16_to_u32_256(x2));
#endif
}

// cvt down (blend high)
force_inline void cvt_to_dst_blendhigh_u16_u8_256(u8 *dst, vector_a_u16_256 y, usize len) {
    vector_u_u8_128 *uvec = (vector_u_u8_128 *)dst;
    *uvec = blendv_128(*uvec, cvt_u16_to_u8_256(y), get_high_mask_u8_128(len));
}

force_inline void cvt_to_dst_blendhigh_u32_u8_256(u8 *dst, vector_a_u32_256 y, usize len) {
    vector_a_u8_64 x = cvt_u32_to_u8_256(y);
    u64 w, w0;
    memcpy(&w, &x, sizeof(w));
    memcpy(&w0, dst, sizeof(w0));
    u64 mask = (1ULL << ((256 / 8 / sizeof(u32) - len) * 8)) - 1;
    w0 = w0 & mask;
    w = w & ~mask;
    w = w | w0;
    memcpy(dst, &w, sizeof(w));
}
#endif // SSRJSON_SIMD_AVX2_CVT_H

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

#ifndef SSRJSON_SIMD_AVX512FCD_COMMON_H
#define SSRJSON_SIMD_AVX512FCD_COMMON_H
#if !defined(__AVX512F__) || !__AVX512F__ || !defined(__AVX512CD__) || !__AVX512CD__
#    error "AVX512F and AVX512CD is required for this file"
#endif

#include "simd/mask_table.h"
#include "simd/simd_detect.h"
#include "simd/vector_types.h"

#define cmpeq_bitmask_u32_512(_a_, _b_) ((u16)_mm512_cmpeq_epi32_mask((_a_), (_b_)))
#define cmpneq_bitmask_u32_512(_a_, _b_) ((u16)_mm512_cmpneq_epi32_mask((_a_), (_b_)))

#define unsigned_cmple_bitmask_u32_512(_a_, _b_) ((u16)_mm512_cmple_epu32_mask((_a_), (_b_)))

#define unsigned_cmplt_bitmask_u32_512(_a_, _b_) ((u16)_mm512_cmplt_epu32_mask((_a_), (_b_)))

#define unsigned_cmpge_bitmask_u32_512(_a_, _b_) ((u16)_mm512_cmpge_epu32_mask((_a_), (_b_)))

#define unsigned_cmpgt_bitmask_u32_512(_a_, _b_) ((u16)_mm512_cmpgt_epu32_mask((_a_), (_b_)))

#define signed_cmpgt_bitmask_u32_512(_a_, _b_) ((u16)_mm512_cmpgt_epi32_mask((_a_), (_b_)))

#define maskz_loadu_u32_512 _mm512_maskz_loadu_epi32
#define maskz_loadu_u64_512 _mm512_maskz_loadu_epi64

#define extract_256_from_512 _mm512_extracti64x4_epi64
#define extract_128_from_512 _mm512_extracti32x4_epi32

#define broadcast_u8_512(_x_) (_mm512_set1_epi8((u8)(_x_)))
#define broadcast_u16_512(_x_) (_mm512_set1_epi16((u16)(_x_)))
#define broadcast_u32_512(_x_) (_mm512_set1_epi32((u32)(_x_)))

#define rshift_u32_512 _mm512_srli_epi32

#define unsigned_max_u32_512 _mm512_max_epu32
#define unsigned_max_u64_512 _mm512_max_epu64

#define setzero_512 _mm512_setzero_si512

force_inline u64 len_to_maskz(usize len) {
    return _LenToMaskZTable[len];
}

force_inline vector_a_u32_512 cvt_u16_to_u32_512(vector_a_u16_256 y) {
    return _mm512_cvtepu16_epi32(y);
}

force_inline vector_a_u32_512 cvt_u8_to_u32_512(vector_a_u8_128 x) {
    return _mm512_cvtepu8_epi32(x);
}

force_inline vector_a_u16_256 cvt_u32_to_u16_512(vector_a_u32_512 z) {
    vector_a_u32_128 x1 = extract_128_from_512(z, 0);
    vector_a_u32_128 x2 = extract_128_from_512(z, 1);
    vector_a_u32_128 x3 = extract_128_from_512(z, 2);
    vector_a_u32_128 x4 = extract_128_from_512(z, 3);
    vector_a_u32_256 y1 = _mm256_set_m128i(x3, x1);
    vector_a_u32_256 y2 = _mm256_set_m128i(x4, x2);
    return _mm256_packus_epi32(y1, y2);
}

force_inline vector_a_u8_256 cvt_u16_to_u8_512(vector_a_u16_512 z) {
    vector_a_u32_128 x1 = extract_128_from_512(z, 0);
    vector_a_u32_128 x2 = extract_128_from_512(z, 1);
    vector_a_u32_128 x3 = extract_128_from_512(z, 2);
    vector_a_u32_128 x4 = extract_128_from_512(z, 3);
    /* y1 = A|C */
    vector_a_u32_256 y1 = _mm256_set_m128i(x3, x1);
    /* y2 = B|D */
    vector_a_u32_256 y2 = _mm256_set_m128i(x4, x2);
    return _mm256_packus_epi16(y1, y2);
}

force_inline u64 get_low_bitmask_512(usize len) {
    return (1ULL << len) - 1;
}

force_inline u64 get_high_bitmask_512(usize len) {
    return ~get_low_bitmask_512(64 - len);
}

#endif // SSRJSON_SIMD_AVX512FCD_COMMON_H

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

#ifndef SSRJSON_SIMD_AVX512VLDQBW_COMMON_H
#define SSRJSON_SIMD_AVX512VLDQBW_COMMON_H
#if !defined(__AVX512VL__) || !__AVX512VL__ || !defined(__AVX512DQ__) || !__AVX512DQ__ || !defined(__AVX512BW__) || !__AVX512BW__
#    error "AVX512VL, AVX512DQ and AVX512BW is required for this file"
#endif

#include "simd/simd_detect.h"
#include "simd/vector_types.h"

#include "simd/avx512f_cd/common.h"

#define maskz_loadu_u8_512 _mm512_maskz_loadu_epi8
#define maskz_loadu_u16_512 _mm512_maskz_loadu_epi16

#define cmpeq_bitmask_u8_512(_a_, _b_) ((u64)_mm512_cmpeq_epu8_mask((_a_), (_b_)))
#define cmpeq_bitmask_u16_512(_a_, _b_) ((u32)_mm512_cmpeq_epu16_mask((_a_), (_b_)))
#define cmpneq_bitmask_u8_512(_a_, _b_) ((u64)_mm512_cmpneq_epu8_mask((_a_), (_b_)))
#define cmpneq_bitmask_u16_512(_a_, _b_) ((u32)_mm512_cmpneq_epu16_mask((_a_), (_b_)))

#define unsigned_cmple_bitmask_u8_512(_a_, _b_) ((u64)_mm512_cmple_epu8_mask((_a_), (_b_)))
#define unsigned_cmple_bitmask_u16_512(_a_, _b_) ((u32)_mm512_cmple_epu16_mask((_a_), (_b_)))

#define unsigned_cmplt_bitmask_u8_512(_a_, _b_) ((u64)_mm512_cmplt_epu8_mask((_a_), (_b_)))
#define unsigned_cmplt_bitmask_u16_512(_a_, _b_) ((u32)_mm512_cmplt_epu16_mask((_a_), (_b_)))

#define unsigned_cmpge_bitmask_u8_512(_a_, _b_) ((u64)_mm512_cmpge_epu8_mask((_a_), (_b_)))
#define unsigned_cmpge_bitmask_u16_512(_a_, _b_) ((u32)_mm512_cmpge_epu16_mask((_a_), (_b_)))

#define unsigned_cmpgt_bitmask_u8_512(_a_, _b_) ((u64)_mm512_cmpgt_epu8_mask((_a_), (_b_)))
#define unsigned_cmpgt_bitmask_u16_512(_a_, _b_) ((u32)_mm512_cmpgt_epu16_mask((_a_), (_b_)))

#define signed_cmpgt_bitmask_u8_512(_a_, _b_) ((u64)_mm512_cmpgt_epi8_mask((_a_), (_b_)))
#define signed_cmpgt_bitmask_u16_512(_a_, _b_) ((u32)_mm512_cmpgt_epi16_mask((_a_), (_b_)))

#define unsigned_max_u8_512 _mm512_max_epu8
#define unsigned_max_u16_512 _mm512_max_epu16

#define shuffle_512 _mm512_shuffle_epi8

#define rshift_u16_512 _mm512_srli_epi16

/* get_bitmask_from functions for AVX512 are not used. */

// force_inline u64 get_bitmask_from_u8_512(vector_a_u8_512 z) {
//     return (u64)_mm512_movepi8_mask(z);
// }

// force_inline u32 get_bitmask_from_u16_512(vector_a_u16_512 z) {
//     return (u32)_mm512_movepi16_mask(z);
// }

// force_inline u16 get_bitmask_from_u32_512(vector_a_u32_512 z) {
//     return (u16)_mm512_movepi32_mask(z);
// }

force_inline vector_a_u16_512 cvt_u8_to_u16_512(vector_a_u8_256 y) {
    return _mm512_cvtepu8_epi16(y);
}

force_inline vector_a_u8_128 cvt_u32_to_u8_512(vector_a_u32_512 z) {
    vector_a_u8_512 shuffler = {
            0, 4, 8, 12,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            //
            0x80, 0x80,
            0x80, 0x80,
            0, 4, 8, 12,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            //
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0, 4, 8, 12,
            0x80, 0x80,
            0x80, 0x80,
            //
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0, 4, 8, 12};
    vector_a_u8_512 shuffled = shuffle_512(z, shuffler);
    vector_a_u8_128 x1 = extract_128_from_512(shuffled, 0);
    vector_a_u8_128 x2 = extract_128_from_512(shuffled, 1);
    vector_a_u8_128 x3 = extract_128_from_512(shuffled, 2);
    vector_a_u8_128 x4 = extract_128_from_512(shuffled, 3);
    return (x1 | x2) | (x3 | x4);
}

#endif // SSRJSON_SIMD_AVX512VLDQBW_COMMON_H

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

#ifndef SSRJSON_SIMD_AVX2_COMMON_H
#define SSRJSON_SIMD_AVX2_COMMON_H

#if !defined(__AVX2__) || !__AVX2__
#    error "AVX2 is required for this file"
#endif
#include "simd/avx/common.h"
#include "simd/simd_detect.h"
#include "simd/vector_types.h"

#define extract_128_from_256 _mm256_extracti128_si256
#define shuffle_256 _mm256_shuffle_epi8

#define cmpeq_u8_256 _mm256_cmpeq_epi8
#define cmpeq_u16_256 _mm256_cmpeq_epi16
#define cmpeq_u32_256 _mm256_cmpeq_epi32

#define signed_cmpgt_u8_256 _mm256_cmpgt_epi8
#define signed_cmpgt_u16_256 _mm256_cmpgt_epi16
#define signed_cmpgt_u32_256 _mm256_cmpgt_epi32

#define unsigned_saturate_minus_u8_256 _mm256_subs_epu8
#define unsigned_saturate_minus_u16_256 _mm256_subs_epu16

#define rshift_u16_256 _mm256_srli_epi16
#define rshift_u32_256 _mm256_srli_epi32

#define unsigned_max_u8_256 _mm256_max_epu8
#define unsigned_max_u16_256 _mm256_max_epu16
#define unsigned_max_u32_256 _mm256_max_epu32

/* Create mask from the highest bit in each 8-bit element. */
force_inline u32 get_bitmask_from_u8_256(SIMD_256 a) {
    return (u32)_mm256_movemask_epi8(a);
}

force_inline vector_a_u8_64 cvt_u32_to_u8_256(vector_a_u32_256 y) {
    vector_a_u8_256 shuffler = {0, 4, 8, 12,
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
                                0x80, 0x80};
    vector_a_u8_256 shuffled = shuffle_256(y, shuffler);
    u64 ret = extract_64_from_256(shuffled, 0) | extract_64_from_256(shuffled, 2);
    return *(vector_a_u8_64 *)&ret;
}

force_inline vector_a_u16_128 cvt_u32_to_u16_256(vector_a_u32_256 y) {
    vector_a_u32_128 x_low = extract_128_from_256(y, 0);
    vector_a_u32_128 x_high = extract_128_from_256(y, 1);
    return _mm_packus_epi32(x_low, x_high);
}

force_inline vector_a_u8_128 cvt_u16_to_u8_256(vector_a_u16_256 y) {
    vector_a_u16_128 x_low = extract_128_from_256(y, 0);
    vector_a_u16_128 x_high = extract_128_from_256(y, 1);
    return _mm_packus_epi16(x_low, x_high);
}

// force_inline u32 get_low_bitmask_256(usize len) {
//     return (SSRJSON_CAST(u32, 1) << len) - 1;
// }

// force_inline u64 get_high_bitmask_256(usize len) {
//     return ~get_low_bitmask_256(32 - len);
// }

#endif // SSRJSON_SIMD_AVX2_COMMON_H

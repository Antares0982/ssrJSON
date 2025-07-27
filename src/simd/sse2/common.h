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

#ifndef SSRJSON_SIMD_SSE2_COMMON_H
#define SSRJSON_SIMD_SSE2_COMMON_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "simd/mask_table.h"

#if defined(_MSC_VER) && !defined(_M_IX86) && !defined(__clang__)
#    define extract_low_u64_from_128(_x_) ((u64)_mm_cvtsi128_si64x(_x_))
#else
#    define extract_low_u64_from_128(_x_) ((u64)_mm_cvtsi128_si64(_x_))
#endif
#define extract_low_u32_from_128(_x_) ((u32)_mm_cvtsi128_si32(_x_))

#define byte_rshift_128 _mm_bsrli_si128
#define byte_lshift_128 _mm_bslli_si128

#define rshift_u16_128 _mm_srli_epi16
#define rshift_u32_128 _mm_srli_epi32

#define unsigned_max_u8_128 _mm_max_epu8

#define setzero_128 _mm_setzero_si128

#define get_bitmask_from_u8_128(_x_) ((u16)_mm_movemask_epi8(_x_))

force_inline vector_a_u8_128 broadcast_u8_128(u8 v) {
    return _mm_set1_epi8((i8)v);
}

force_inline vector_a_u16_128 broadcast_u16_128(u16 v) {
    return _mm_set1_epi16((i16)v);
}

force_inline vector_a_u32_128 broadcast_u32_128(u32 v) {
    return _mm_set1_epi32((i32)v);
}

force_inline SIMD_128 broadcast_u64_128(i64 v) {
#if defined(_MSC_VER) && !defined(_M_IX86)
    return _mm_set1_epi64x(v);
#else
    return _mm_set1_epi64((__m64)v);
#endif
}

force_inline vector_a_u16_128 cvt_u8_to_u16_128(vector_a_u8_128 a) {
#if __SSE4_1__
    return _mm_cvtepu8_epi16(a);
#elif __SSSE3__
    vector_a_u8_128 m = {0, 0x80, 1, 0x80,
                         2, 0x80, 3, 0x80,
                         4, 0x80, 5, 0x80,
                         6, 0x80, 7, 0x80};
    return _mm_shuffle_epi8(a, m);
#else
    return _mm_unpacklo_epi8(a, setzero_128()); // ~2 cycles ..
#endif
}

force_inline vector_a_u32_128 cvt_u8_to_u32_128(vector_a_u8_128 a) {
#if __SSE4_1__
    return _mm_cvtepu8_epi32(a);
#elif __SSSE3__
    vector_a_u8_128 m = {0, 0x80, 0x80, 0x80,
                         1, 0x80, 0x80, 0x80,
                         2, 0x80, 0x80, 0x80,
                         3, 0x80, 0x80, 0x80};
    return _mm_shuffle_epi8(a, m);
#else
    a = _mm_unpacklo_epi8(a, a);                         // a0, a0, a1, a1, a2, a2, a3, a3, ....
    return rshift_u32_128(_mm_unpacklo_epi16(a, a), 24); // ~ 3 cycles ...
#endif
}

force_inline vector_a_u32_128 cvt_u16_to_u32_128(vector_a_u16_128 a) {
#if defined(__SSE4_1__)
    return _mm_cvtepu16_epi32(a);
#elif defined(__SSSE3__)
    vector_a_u8_128 shuffler = {0, 1, 0x80, 0x80,
                                2, 3, 0x80, 0x80,
                                4, 5, 0x80, 0x80,
                                6, 7, 0x80, 0x80};
    return _mm_shuffle_epi8(a, shuffler);
#else
    return _mm_unpacklo_epi16(a, setzero_128());
#endif
}

force_inline vector_a_u16_64 cvt_u32_to_u16_128(vector_a_u32_128 x) {
#if __SSE4_1__
    return (vector_a_u16_64)extract_low_u64_from_128(_mm_packus_epi32(x, x));
#else
    // in this case we don't have the convenient `_mm_packus_epi32`
    /* x =  aa00bb00|cc00dd00 */
    /* x1 = 00cc00dd|00000000 */
    vector_a_u8_128 x1 = byte_rshift_128(x, 6);
    /* x2 = bb00cc00|dd000000 */
    vector_a_u8_128 x2 = byte_rshift_128(x, 4);
    /* x3 = 00dd0000|00000000 */
    vector_a_u8_128 x3 = byte_rshift_128(x, 10);
    /* x4 = aaccbbdd|cc00dd00 */
    vector_a_u8_128 x4 = x | x1;
    /* x5 = bbddcc00|dd000000 */
    vector_a_u8_128 x5 = x2 | x3;
    /* x6 = aabbccdd|???????? */
    vector_a_u8_128 x6 = _mm_unpacklo_epi16(x4, x5);
    return (vector_a_u16_64)extract_low_u64_from_128(x6);
#endif
}

force_inline vector_a_u8_64 cvt_u16_to_u8_128(vector_a_u16_128 x) {
    return (vector_a_u8_64)extract_low_u64_from_128(_mm_packus_epi16(x, x));
}

force_inline vector_a_u8_32 cvt_u32_to_u8_128(vector_a_u32_128 x) {
#if __SSSE3__
    vector_a_u8_128 t1 = {0, 4, 8, 12,
                          0, 4, 8, 12,
                          0, 4, 8, 12,
                          0, 4, 8, 12};
    return (vector_a_u8_32)extract_low_u32_from_128(_mm_shuffle_epi8(x, t1));
#else
    // first using signed pack to u16. The values in `x` are below 256, so signed pack is equivalent to unsigned pack.
    SIMD_128 x1 = _mm_packs_epi32(x, x);
    // then use unsigned pack to u8
    return (vector_a_u8_32)extract_low_u32_from_128(_mm_packus_epi16(x1, x1));
#endif
}

#define cmpeq_u8_128 _mm_cmpeq_epi8
#define cmpeq_u16_128 _mm_cmpeq_epi16
#define cmpeq_u32_128 _mm_cmpeq_epi32
#define signed_cmplt_u8_128 _mm_cmplt_epi8
#define signed_cmplt_u16_128 _mm_cmplt_epi16
#define signed_cmplt_u32_128 _mm_cmplt_epi32
#define signed_cmpgt_u8_128 _mm_cmpgt_epi8
#define signed_cmpgt_u16_128 _mm_cmpgt_epi16
#define signed_cmpgt_u32_128 _mm_cmpgt_epi32
#define unsigned_saturate_minus_u8_128 _mm_subs_epu8
#define unsigned_saturate_minus_u16_128 _mm_subs_epu16

force_inline bool testz_128(SIMD_128 a) {
#if defined(__SSE4_1__)
    return (bool)_mm_testz_si128(a, a);
#else
    return _mm_movemask_epi8(cmpeq_u8_128(_mm_and_si128(a, a), setzero_128())) == 0xFFFF;
#endif
}

force_inline bool testz2_128(SIMD_128 a, SIMD_128 b) {
#if defined(__SSE4_1__)
    return (bool)_mm_testz_si128(a, b);
#else
    return _mm_movemask_epi8(cmpeq_u8_128(_mm_and_si128(a, b), setzero_128())) == 0xFFFF;
#endif
}

/*
 * Right shift 128 bits for the case imm8 cannot be determined at compile time.
 Shifted bits should be multiple of 8; imm8 is the number of "bytes" to shift.
 */
force_inline SIMD_128 runtime_byte_rshift_128(SIMD_128 x, int imm8) {
#if __SSSE3__
    return _mm_shuffle_epi8(x, *(SIMD_128 *)byte_rshift_mask_table(imm8));
#else
    switch (imm8) {
        case 1: {
            return byte_rshift_128(x, 1);
            break;
        }
        case 2: {
            return byte_rshift_128(x, 2);
            break;
        }
        case 3: {
            return byte_rshift_128(x, 3);
            break;
        }
        case 4: {
            return byte_rshift_128(x, 4);
            break;
        }
        case 5: {
            return byte_rshift_128(x, 5);
            break;
        }
        case 6: {
            return byte_rshift_128(x, 6);
            break;
        }
        case 7: {
            return byte_rshift_128(x, 7);
            break;
        }
        case 8: {
            return byte_rshift_128(x, 8);
            break;
        }
        case 9: {
            return byte_rshift_128(x, 9);
            break;
        }
        case 10: {
            return byte_rshift_128(x, 10);
            break;
        }
        case 11: {
            return byte_rshift_128(x, 11);
            break;
        }
        case 12: {
            return byte_rshift_128(x, 12);
            break;
        }
        case 13: {
            return byte_rshift_128(x, 13);
            break;
        }
        case 14: {
            return byte_rshift_128(x, 14);
            break;
        }
        case 15: {
            return byte_rshift_128(x, 15);
            break;
        }
        default: {
            SSRJSON_UNREACHABLE();
        }
    }
    SSRJSON_UNREACHABLE();
    return x;
#endif
}

#endif // SSRJSON_SIMD_SSE2_COMMON_H

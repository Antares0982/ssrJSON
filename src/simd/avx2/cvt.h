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

/*==============================================================================
 * `avx2_trailing_cvt` Series.
 * Copy `[src, src_end)` to `dst` with conversion. 
 * Assuming that 16 bytes before `src_end` are readable,
 * and `32 * sizeof(_dst_t) / sizeof(_src_t)`
 * bytes after `dst` are writable.
 *============================================================================*/

force_inline void __avx2_trailing_cvt_same_size(const void *__src, const void *__src_end, void *__dst) {
    const size_t half = 32 / 2;
    const u8 *const src = SSRJSON_CAST(u8 *, __src);
    // 16 bytes before src_end
    const u8 *t1 = SSRJSON_CAST(u8 *, __src_end) - half;
    const bool t1_before_src = t1 < src;
    //
    vector_a_u8_128 s1, s2;
    s1 = *(vector_u_u8_128 *)(t1_before_src ? t1 : src);
    s2 = *(vector_u_u8_128 *)t1;
    const int shl1 = (src - t1);
    const int shl2 = (half - (t1 - src));
    s1 = runtime_byte_rshift_128(s1, (t1_before_src ? shl1 : 0));
    s2 = runtime_byte_rshift_128(s2, (t1_before_src ? 0 : shl2));
    //
    *(SSRJSON_CAST(vector_u_u8_128 *, __dst) + 0) = s1;
    *(SSRJSON_CAST(vector_u_u8_128 *, __dst) + 1) = s2;
}

force_inline void avx2_trailing_cvt_u8_u8(const u8 *src, const u8 *src_end, u8 *dst) {
    __avx2_trailing_cvt_same_size(src, src_end, dst);
}

force_inline void avx2_trailing_cvt_u16_u16(const u16 *src, const u16 *src_end, u16 *dst) {
    __avx2_trailing_cvt_same_size(src, src_end, dst);
}

force_inline void avx2_trailing_cvt_u32_u32(const u32 *src, const u32 *src_end, u32 *dst) {
    __avx2_trailing_cvt_same_size(src, src_end, dst);
}

// trailing cvt (up)

force_inline void avx2_trailing_cvt_u8_u16(const u8 *src, const u8 *src_end, u16 *dst) {
    const size_t half = 32 / 2 / sizeof(u8);
    // 16 bytes before src_end
    const u8 *t1 = src_end - half;
    const bool t1_before_src = t1 < src;
    //
    vector_a_u8_128 s1, s2;
    s1 = *(vector_u_u8_128 *)(t1_before_src ? t1 : src);
    s2 = *(vector_u_u8_128 *)t1;
    //
    int __shl1 = (src - t1);
    int __shl2 = (half - (t1 - src));
    int shl1 = t1_before_src ? __shl1 : 0;
    int shl2 = t1_before_src ? 0 : __shl2;
    s1 = runtime_byte_rshift_128(s1, shl1);
    s2 = runtime_byte_rshift_128(s2, shl2);
    *(SSRJSON_CAST(vector_u_u16_256 *, dst) + 0) = cvt_u8_to_u16_256(s1);
    *(SSRJSON_CAST(vector_u_u16_256 *, dst) + 1) = cvt_u8_to_u16_256(s2);
}

force_inline void avx2_trailing_cvt_u8_u32(const u8 *src, const u8 *src_end, u32 *dst) {
    const size_t half = 32 / 2 / sizeof(u8);
    // 16 bytes before src_end
    const u8 *t1 = src_end - half;
    const bool t1_before_src = t1 < src;
    //
    vector_a_u8_128 s1, s2;
    s1 = *(vector_u_u8_128 *)(t1_before_src ? t1 : src);
    s2 = *(vector_u_u8_128 *)t1;
    int __shl1 = (src - t1);
    int __shl2 = (half - (t1 - src));
    int shl1 = t1_before_src ? __shl1 : 0;
    int shl2 = t1_before_src ? 0 : __shl2;
    vector_a_u8_128 x1, x2, x3, x4;
    x1 = runtime_byte_rshift_128(s1, shl1);
    x3 = runtime_byte_rshift_128(s2, shl2);
    x2 = byte_rshift_128(x1, 8);
    x4 = byte_rshift_128(x3, 8);
    *(vector_u_u32_256 *)(dst + 0) = cvt_u8_to_u32_256(x1);
    *(vector_u_u32_256 *)(dst + 16) = cvt_u8_to_u32_256(x3);
    *(vector_u_u32_256 *)(dst + 8) = cvt_u8_to_u32_256(x2);
    *(vector_u_u32_256 *)(dst + 24) = cvt_u8_to_u32_256(x4);
}

force_inline void avx2_trailing_cvt_u16_u32(const u16 *src, const u16 *src_end, u32 *dst) {
    const size_t half = 32 / 2 / sizeof(u16);
    // 16 bytes before src_end
    const u16 *t1 = src_end - half;
    const bool t1_before_src = t1 < src;
    //
    vector_a_u8_128 s1, s2;
    s1 = *(vector_u_u8_128 *)(t1_before_src ? t1 : src);
    s2 = *(vector_u_u8_128 *)t1;
    //
    int __shl1 = (src - t1) * 2;
    int __shl2 = (half - (t1 - src)) * 2;
    int shl1 = t1_before_src ? __shl1 : 0;
    int shl2 = t1_before_src ? 0 : __shl2;
    s1 = runtime_byte_rshift_128(s1, shl1);
    s2 = runtime_byte_rshift_128(s2, shl2);
    *(SSRJSON_CAST(vector_u_u32_256 *, dst) + 0) = cvt_u16_to_u32_256(s1);
    *(SSRJSON_CAST(vector_u_u32_256 *, dst) + 1) = cvt_u16_to_u32_256(s2);
}

// trailing cvt (down)
force_inline void avx2_trailing_cvt_u16_u8(const u16 *src, const u16 *src_end, u8 *dst) {
    const size_t half = 32 / 2 / sizeof(u16);
    // 16 bytes before src_end
    const u16 *t1 = src_end - half;
    const bool t1_before_src = t1 < src;
    //
    vector_a_u8_128 s1, s2;
    s1 = *(vector_u_u8_128 *)(t1_before_src ? t1 : src);
    s2 = *(vector_u_u8_128 *)t1;
    //
    int __shl1 = (src - t1) * 2;
    int __shl2 = (half - (t1 - src)) * 2;
    int shl1 = t1_before_src ? __shl1 : 0;
    int shl2 = t1_before_src ? 0 : __shl2;
    s1 = runtime_byte_rshift_128(s1, shl1);
    s2 = runtime_byte_rshift_128(s2, shl2);
    *SSRJSON_CAST(vector_u_u8_128 *, dst) = _mm_packus_epi16(s1, s2);
}

force_inline void avx2_trailing_cvt_u32_u8(const u32 *src, const u32 *src_end, u8 *dst) {
    const size_t half = 32 / 2 / sizeof(u32);
    // 16 bytes before src_end
    const u32 *t1 = src_end - half;
    const bool t1_before_src = t1 < src;
    //
    vector_a_u8_128 s1, s2;
    s1 = *(vector_u_u8_128 *)(t1_before_src ? t1 : src);
    s2 = *(vector_u_u8_128 *)t1;
    int __shl1 = (src - t1);
    int __shl2 = (half - (t1 - src));
    int shl1 = t1_before_src ? __shl1 : 0;
    int shl2 = t1_before_src ? 0 : __shl2;
    assert(shl1 >= 0 && shl1 < 4);
    assert(shl2 >= 0 && shl2 <= 4);
    s1 = _mm_shuffle_epi8(s1, *SSRJSON_CAST(vector_a_u8_128 *, &_AVX2TrailingCvtRShiftMaskTable32to8[shl1][0]));
    s2 = _mm_shuffle_epi8(s2, *SSRJSON_CAST(vector_a_u8_128 *, &_AVX2TrailingCvtRShiftMaskTable32to8[shl2][0]));
    // vmovd
    memcpy(dst, &s1, 4);
    // vmovd
    memcpy(dst + 4, &s2, 4);
}

#endif // SSRJSON_SIMD_AVX2_CVT_H

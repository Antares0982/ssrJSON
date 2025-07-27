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

#ifndef SSRJSON_SIMD_AVX512FCD_CVT_H
#define SSRJSON_SIMD_AVX512FCD_CVT_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "common.h"
#include "simd/sse2/common.h"

force_inline void cvt_to_dst_u8_u8_512(u8 *dst, vector_a_u8_512 x) {
    *(vector_u_u8_512 *)dst = x;
}

force_inline void cvt_to_dst_u16_u16_512(u16 *dst, vector_a_u16_512 x) {
    *(vector_u_u16_512 *)dst = x;
}

force_inline void cvt_to_dst_u32_u32_512(u32 *dst, vector_a_u32_512 x) {
    *(vector_u_u32_512 *)dst = x;
}

// cvt_to_dst_u8_u16_512: AVX512VL+DQ+BW

force_inline void cvt_to_dst_u8_u32_512(u32 *dst, vector_a_u8_512 z) {
    vector_a_u8_128 x1, x2, x3, x4;
    x1 = extract_128_from_512(z, 0);
    x2 = extract_128_from_512(z, 1);
    x3 = extract_128_from_512(z, 2);
    x4 = extract_128_from_512(z, 3);
    *(vector_u_u32_512 *)(dst + 0) = cvt_u8_to_u32_512(x1);
    *(vector_u_u32_512 *)(dst + 16) = cvt_u8_to_u32_512(x2);
    *(vector_u_u32_512 *)(dst + 32) = cvt_u8_to_u32_512(x3);
    *(vector_u_u32_512 *)(dst + 48) = cvt_u8_to_u32_512(x4);
}

force_inline void cvt_to_dst_u16_u32_512(u32 *dst, vector_a_u16_512 z) {
    *(vector_u_u32_512 *)(dst + 0) = cvt_u16_to_u32_512(extract_256_from_512(z, 0));
    *(vector_u_u32_512 *)(dst + 16) = cvt_u16_to_u32_512(extract_256_from_512(z, 1));
}

// vec down

force_inline void cvt_to_dst_u32_u16_512(u16 *dst, vector_a_u32_512 z) {
    *(vector_u_u16_256 *)dst = cvt_u32_to_u16_512(z);
}

force_inline void cvt_to_dst_u16_u8_512(u8 *dst, vector_a_u16_512 z) {
    *(vector_u_u8_256 *)dst = cvt_u16_to_u8_512(z);
}

// cvt_to_dst_u32_u8_512: AVX512VL+DQ+BW

#endif // SSRJSON_SIMD_AVX512FCD_CVT_H

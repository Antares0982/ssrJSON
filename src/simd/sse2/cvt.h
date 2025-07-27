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

#ifndef SSRJSON_SIMD_SSE2_CVT_H
#define SSRJSON_SIMD_SSE2_CVT_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "common.h"

#if __AVX512F__ && __AVX512CD__
force_inline vector_a_u32_512 cvt_u8_to_u32_512(vector_a_u8_128 x);
#endif

#if __AVX2__
force_inline vector_a_u16_256 cvt_u8_to_u16_256(vector_a_u8_128 x);
force_inline vector_a_u32_256 cvt_u16_to_u32_256(vector_a_u16_128 x);
#endif

force_inline void cvt_to_dst_u8_u8_128(u8 *dst, vector_a_u8_128 x) {
    *(vector_u_u8_128 *)dst = x;
}

force_inline void cvt_to_dst_u16_u16_128(u16 *dst, vector_a_u16_128 x) {
    *(vector_u_u16_128 *)dst = x;
}

force_inline void cvt_to_dst_u32_u32_128(u32 *dst, vector_a_u32_128 x) {
    *(vector_u_u32_128 *)dst = x;
}

// cvt up

force_inline void cvt_to_dst_u8_u16_128(u16 *dst, vector_a_u8_128 x) {
#if __AVX2__
    *(vector_u_u16_256 *)dst = cvt_u8_to_u16_256(x);
#else
    *(vector_u_u16_128 *)(dst + 0) = cvt_u8_to_u16_128(x);
    *(vector_u_u16_128 *)(dst + 8) = cvt_u8_to_u16_128(byte_rshift_128(x, 8));
#endif
}

force_inline void cvt_to_dst_u8_u32_128(u32 *dst, vector_a_u8_128 x) {
#if __AVX512F__ && __AVX512CD__
    *(vector_u_u32_512 *)dst = cvt_u8_to_u32_512(x);
#elif __AVX2__
    *(vector_u_u32_256 *)(dst + 0) = cvt_u8_to_u32_256(x);
    *(vector_u_u32_256 *)(dst + 8) = cvt_u8_to_u32_256(byte_rshift_128(x, 8));
#else
    *(vector_u_u32_128 *)(dst + 0) = cvt_u8_to_u32_128(x);
    *(vector_u_u32_128 *)(dst + 4) = cvt_u8_to_u32_128(byte_rshift_128(x, 4));
    *(vector_u_u32_128 *)(dst + 8) = cvt_u8_to_u32_128(byte_rshift_128(x, 8));
    *(vector_u_u32_128 *)(dst + 12) = cvt_u8_to_u32_128(byte_rshift_128(x, 12));
#endif
}

force_inline void cvt_to_dst_u16_u32_128(u32 *dst, vector_a_u16_128 x) {
#if __AVX2__
    *(vector_u_u32_256 *)dst = cvt_u16_to_u32_256(x);
#else
    *(vector_u_u32_128 *)(dst + 0) = cvt_u16_to_u32_128(x);
    *(vector_u_u32_128 *)(dst + 4) = cvt_u16_to_u32_128(byte_rshift_128(x, 8));
#endif
}

// cvt down

force_inline void cvt_to_dst_u16_u8_128(u8 *dst, vector_a_u16_128 x) {
    *(vector_u_u8_64 *)dst = cvt_u16_to_u8_128(x);
}

force_inline void cvt_to_dst_u32_u8_128(u8 *dst, vector_a_u32_128 x) {
    *(vector_u_u8_32 *)dst = cvt_u32_to_u8_128(x);
}

force_inline void cvt_to_dst_u32_u16_128(u16 *dst, vector_a_u32_128 x) {
    *(vector_u_u16_64 *)dst = cvt_u32_to_u16_128(x);
}


#endif // SSRJSON_SIMD_SSE2_CVT_H

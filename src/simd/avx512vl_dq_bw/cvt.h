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

#ifndef SSRJSON_SIMD_AVX512VLDQBW_CVT_H
#define SSRJSON_SIMD_AVX512VLDQBW_CVT_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "common.h"
#include "simd/avx512f_cd/cvt.h"

force_inline void cvt_to_dst_u8_u16_512(u16 *dst, vector_a_u8_512 y) {
    *(vector_u_u16_512 *)(dst + 0) = cvt_u8_to_u16_512(extract_256_from_512(y, 0));
    *(vector_u_u16_512 *)(dst + 32) = cvt_u8_to_u16_512(extract_256_from_512(y, 1));
}

force_inline void cvt_to_dst_u32_u8_512(u8 *dst, vector_a_u32_512 z) {
    *(vector_u_u8_128 *)dst = cvt_u32_to_u8_512(z);
}

// other: AVX512F+CD

#endif // SSRJSON_SIMD_AVX512VLDQBW_CVT_H

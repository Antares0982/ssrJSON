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

#ifndef SSRJSON_SIMD_AVX512VLDQBW_CHECKMAX_H
#define SSRJSON_SIMD_AVX512VLDQBW_CHECKMAX_H
#if !defined(__AVX512VL__) || !__AVX512VL__ || !defined(__AVX512DQ__) || !__AVX512DQ__ || !defined(__AVX512BW__) || !__AVX512BW__
#    error "AVX512VL, AVX512DQ and AVX512BW is required for this file"
#endif

#include "common.h"
#include "simd/simd_detect.h"
#include "simd/vector_types.h"

// checkmax_u32_512: AVX512F+CD

force_inline bool checkmax_u16_512(vector_a_u16_512 z, u16 lower_bound_minus_1) {
    const vector_a_u16_512 t = broadcast_u16_512(lower_bound_minus_1);
    return 0 == unsigned_cmpgt_bitmask_u16_512(z, t);
}

force_inline bool checkmax_u8_512(vector_a_u8_512 z, u8 lower_bound_minus_1) {
    const vector_a_u8_512 t = broadcast_u8_512(lower_bound_minus_1);
    return 0 == unsigned_cmpgt_bitmask_u8_512(z, t);
}

#endif // SSRJSON_SIMD_AVX512VLDQBW_CHECKMAX_H

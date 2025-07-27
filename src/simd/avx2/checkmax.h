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

#ifndef SSRJSON_SIMD_AVX2_CHECKMAX_H
#define SSRJSON_SIMD_AVX2_CHECKMAX_H

#if !defined(__AVX2__) || !__AVX2__
#    error "AVX2 is required for this file"
#endif
#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "simd/avx2/common.h"

// return true if all elements in the vector are less than or equal to lower_bound_minus_1
force_inline bool checkmax_u32_256(vector_a_u32_256 y, u32 lower_bound_minus_1) {
    // NOTE: ucs4 range is 0-0x10FFFF. Signed compare is equal to unsigned compare
    const vector_a_u32_256 t = broadcast_u32_256(lower_bound_minus_1);
    vector_a_u32_256 mask = signed_cmpgt_u32_256(y, t);
    return testz_256(mask);
}

force_inline bool checkmax_u16_256(vector_a_u16_256 y, u16 lower_bound_minus_1) {
    const vector_a_u16_256 t = broadcast_u16_256(lower_bound_minus_1);
    vector_a_u16_256 mask = unsigned_saturate_minus_u16_256(y, t);
    return testz_256(mask);
}

force_inline bool checkmax_u8_256(vector_a_u8_256 y, u8 lower_bound_minus_1) {
    const vector_a_u8_256 t = broadcast_u8_256(lower_bound_minus_1);
    vector_a_u8_256 mask = unsigned_saturate_minus_u8_256(y, t);
    return testz_256(mask);
}

#endif // SSRJSON_SIMD_AVX2_CHECKMAX_H

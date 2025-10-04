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

#ifndef SSRJSON_SIMD_NEON_CHECKMAX_H
#define SSRJSON_SIMD_NEON_CHECKMAX_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "common.h"

force_inline bool checkmax_u32_128(vector_a_u32_128 y, u32 lower_bound_minus_1) {
    return testz_128(y > broadcast_u32_128(lower_bound_minus_1));
}

force_inline bool checkmax_u16_128(vector_a_u16_128 y, u16 lower_bound_minus_1) {
    return testz_128(y > broadcast_u16_128(lower_bound_minus_1));
}

force_inline bool checkmax_u8_128(vector_a_u8_128 y, u8 lower_bound_minus_1) {
    return testz_128(y > broadcast_u8_128(lower_bound_minus_1));
}

#endif // SSRJSON_SIMD_NEON_CHECKMAX_H

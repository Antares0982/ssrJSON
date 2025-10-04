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

#ifndef SSRJSON_SIMD_NEON_COMMON_H
#define SSRJSON_SIMD_NEON_COMMON_H

#include "simd/simd_detect.h"
#include "simd/union_vector.h"
#include "simd/vector_types.h"
//
#include "simd/mask_table.h"
#include "ssrjson.h"

#define rshift_u16_128 vshrq_n_u16
#define rshift_u32_128 vshrq_n_u32
#define lshift_u16_128 vshlq_n_u16
#define lshift_u32_128 vshlq_n_u32

#define unsigned_max_u8_128 vmaxq_u8
#define unsigned_max_u16_128 vmaxq_u16
#define unsigned_max_u32_128 vmaxq_u32

force_inline vector_a_u8_128 setzero_128(void) {
    return vdupq_n_u8(0);
}

#define broadcast_u8_128 vdupq_n_u8
#define broadcast_u16_128 vdupq_n_u16
#define broadcast_u32_128 vdupq_n_u32
#define broadcast_u64_128 vdupq_n_u64

force_inline vector_a_u16_128 cvt_u8_to_u16_128(vector_a_u8_128 a) {
    return vmovl_u8(*SSRJSON_CAST(vector_a_u8_64 *, &a));
}

force_inline vector_a_u32_128 cvt_u16_to_u32_128(vector_a_u16_128 a) {
    return vmovl_u16(*SSRJSON_CAST(vector_a_u16_64 *, &a));
}

force_inline vector_a_u32_128 cvt_u8_to_u32_128(vector_a_u8_128 a) {
    vector_a_u16_128 b = cvt_u8_to_u16_128(a);
    return cvt_u16_to_u32_128(b);
}

#define cvt_u32_to_u16_128 vmovn_u32
#define cvt_u16_to_u8_128 vmovn_u16

force_inline vector_a_u8_32 cvt_u32_to_u8_128(vector_a_u32_128 x) {
    uint16x8_t y;
    uint8x8_t z;
    *(uint16x4_t *)&y = cvt_u32_to_u16_128(x);
    *(uint8x8_t *)&z = cvt_u16_to_u8_128(y);
    return *(vector_a_u8_32 *)&z;
}

#define cmpeq_u8_128 vceqq_u8
#define cmpeq_u16_128 vceqq_u16
#define cmpeq_u32_128 vceqq_u32
#define signed_cmplt_u8_128 vcltq_s8
#define signed_cmplt_u16_128 vcltq_s16
#define signed_cmplt_u32_128 vcltq_s32
#define signed_cmpgt_u8_128 vcgtq_s8
#define signed_cmpgt_u16_128 vcgtq_s16
#define signed_cmpgt_u32_128 vcgtq_s32

#define testz_128(_x_) ((u128)(_x_) == 0)

#define shuffle_128 vqtbl1q_u8

#define alignr_128 vextq_u8

#define testz2_128(_a_, _b_) testz_128((_a_) & (_b_))

/*
 * Right shift 128 bits for the case imm8 cannot be determined at compile time.
 Shifted bits should be multiple of 8; imm8 is the number of "bytes" to shift.
 */
force_inline vector_a_u8_128 runtime_byte_rshift_128(vector_a_u8_128 x, int imm8) {
    return shuffle_128(x, *(const vector_a_u8_128 *)byte_rshift_mask_table(imm8));
}

#endif // SSRJSON_SIMD_NEON_COMMON_H

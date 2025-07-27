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

#ifdef SSRJSON_CLANGD_DUMMY
#    ifndef COMPILE_READ_UCS_LEVEL
#        include "simd/union_vector.h"
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif
//
#include "simd/sse2/common.h"
//
#define COMPILE_SIMD_BITS 128
#include "compile_context/sr_in.inl.h"
#include "simd/mask_table.h"

force_inline vector_a get_high_mask(usize count) {
    const vector_a *mask_ptr = read_tail_mask_table_8(16 - count * sizeof(_src_t));
    return *mask_ptr;
}

force_inline vector_a high_mask(vector_a x, usize count) {
    return x & get_high_mask(count);
}

force_inline vector_a get_low_mask(usize count) {
    const vector_a *mask_ptr = read_head_mask_table_8(count * sizeof(_src_t));
    return *mask_ptr;
}

force_inline vector_a low_mask(vector_a x, usize count) {
    return x & get_low_mask(count);
}

force_inline vector_a get_escape_mask(vector_a x) {
    vector_a t1 = broadcast(_Slash);
    vector_a t2 = broadcast(_Quote);
    vector_a t3 = broadcast(ControlMax);
    vector_a x1 = x == t1;
    vector_a x2 = x == t2;
#if CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
    vector_a x3 = unsigned_saturate_minus(t3, x);
#else
    vector_a x3_1 = signed_cmpgt(x, broadcast(-1));
    vector_a x3_2 = signed_cmplt(x, t3);
    vector_a x3 = x3_1 & x3_2;
#endif
    return x1 | x2 | x3;
}

force_inline u16 escape_mask_to_bitmask(vector_a mask) {
#if CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
    mask = cmpeq(mask, setzero());
    u16 bitmask = get_bitmask_from_u8(mask);
    bitmask = ~bitmask;
#else
    u32 bitmask = get_bitmask_from_u8(mask);
#endif
    return bitmask;
}

force_inline usize escape_mask_to_done_count(vector_a mask) {
    return u32_tz_bits(escape_mask_to_bitmask(mask)) / COMPILE_READ_UCS_LEVEL;
}

force_inline usize escape_mask_to_done_count_no_eq0(vector_a mask) {
    return u32_tz_bits(get_bitmask_from_u8(mask)) / COMPILE_READ_UCS_LEVEL;
}

force_inline usize joined4_escape_mask_to_done_count(vector_a mask1,
                                                     vector_a mask2,
                                                     vector_a mask3,
                                                     vector_a mask4) {
    u64 bitmask1, bitmask2, bitmask3, bitmask4;
    bitmask1 = 0xffff & escape_mask_to_bitmask(mask1);
    bitmask2 = 0xffff & escape_mask_to_bitmask(mask2);
    bitmask3 = 0xffff & escape_mask_to_bitmask(mask3);
    bitmask4 = escape_mask_to_bitmask(mask4);
    u64 bitmask;
    bitmask = bitmask1 | (bitmask2 << 16) |
              (bitmask3 << 32) | (bitmask4 << 48);
    assert(bitmask);
    return u64_tz_bits(bitmask) / COMPILE_READ_UCS_LEVEL;
}

#include "compile_context/sr_out.inl.h"
#undef COMPILE_SIMD_BITS

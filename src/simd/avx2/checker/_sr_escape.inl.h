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
#include "simd/avx2/common.h"
//
force_inline const void *read_tail_mask_table_8(Py_ssize_t row);
force_inline const void *read_head_mask_table_8(Py_ssize_t row);

#define COMPILE_SIMD_BITS 256
#include "compile_context/sr_in.inl.h"

force_inline vector_a get_high_mask(usize count) {
    const vector_a *mask_ptr = read_tail_mask_table_8(32 - count * sizeof(_src_t));
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
    vector_a x3_2 = signed_cmpgt(t3, x);
    vector_a x3 = x3_1 & x3_2;
#endif
    return x1 | x2 | x3;
}

force_inline u32 escape_mask_to_bitmask(vector_a mask) {
#if CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
    mask = cmpeq(mask, setzero());
    u32 bitmask = get_bitmask_from_u8(mask);
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

force_inline usize escape_mask_to_done_count_track_max(vector_a mask, vector_a *max_vec, vector_a src_vec) {
    usize ret = escape_mask_to_done_count(mask);
    vector_a part = low_mask(src_vec, ret);
    *max_vec = unsigned_max(part, *max_vec);
    return ret;
}

force_inline usize joined4_escape_mask_to_done_count(vector_a mask1,
                                                     vector_a mask2,
                                                     vector_a mask3,
                                                     vector_a mask4) {
    u64 bitmask1, bitmask2, bitmask3, bitmask4;
    u64 bitmask[2];
    bitmask1 = 0xffffffff & escape_mask_to_bitmask(mask1);
    bitmask2 = escape_mask_to_bitmask(mask2);
    bitmask3 = 0xffffffff & escape_mask_to_bitmask(mask3);
    bitmask4 = escape_mask_to_bitmask(mask4);
    bitmask[0] = bitmask1 | (bitmask2 << 32);
    bitmask[1] = bitmask3 | (bitmask4 << 32);
    assert(bitmask[0] | bitmask[1]);
    if (bitmask[0]) return u64_tz_bits(bitmask[0]) / COMPILE_READ_UCS_LEVEL;
    return 64 / COMPILE_READ_UCS_LEVEL + u64_tz_bits(bitmask[1]) / COMPILE_READ_UCS_LEVEL;
}

force_inline usize joined4_escape_mask_to_done_count_track_max(vector_a mask1,
                                                               vector_a mask2,
                                                               vector_a mask3,
                                                               vector_a mask4,
                                                               vector_a *max_vec,
                                                               unionvector_a_x4 src_vecs) {
    const usize bitsize = 32;
    usize cnt;
    u32 bitmask1, bitmask2, bitmask3, bitmask4;
    bitmask1 = escape_mask_to_bitmask(mask1);
    bitmask2 = escape_mask_to_bitmask(mask2);
    bitmask3 = escape_mask_to_bitmask(mask3);
    bitmask4 = escape_mask_to_bitmask(mask4);
    if (bitmask1) {
        cnt = u32_tz_bits(bitmask1) / COMPILE_READ_UCS_LEVEL;
        vector_a part1 = low_mask(src_vecs.x[0], cnt);
        *max_vec = unsigned_max(part1, *max_vec);
        return cnt + 0 * bitsize / COMPILE_READ_UCS_LEVEL;
    }
    *max_vec = unsigned_max(src_vecs.x[0], *max_vec);
    if (bitmask2) {
        cnt = u32_tz_bits(bitmask2) / COMPILE_READ_UCS_LEVEL;
        vector_a part2 = low_mask(src_vecs.x[1], cnt);
        *max_vec = unsigned_max(part2, *max_vec);
        return cnt + 1 * bitsize / COMPILE_READ_UCS_LEVEL;
    }
    *max_vec = unsigned_max(src_vecs.x[1], *max_vec);
    if (bitmask3) {
        cnt = u32_tz_bits(bitmask3) / COMPILE_READ_UCS_LEVEL;
        vector_a part3 = low_mask(src_vecs.x[2], cnt);
        *max_vec = unsigned_max(part3, *max_vec);
        return cnt + 2 * bitsize / COMPILE_READ_UCS_LEVEL;
    }
    *max_vec = unsigned_max(src_vecs.x[2], *max_vec);
    {
        assert(bitmask4);
        cnt = u32_tz_bits(bitmask4) / COMPILE_READ_UCS_LEVEL;
        vector_a part4 = low_mask(src_vecs.x[3], cnt);
        *max_vec = unsigned_max(part4, *max_vec);
        return cnt + 3 * bitsize / COMPILE_READ_UCS_LEVEL;
    }
}

#include "compile_context/sr_out.inl.h"
#undef COMPILE_SIMD_BITS

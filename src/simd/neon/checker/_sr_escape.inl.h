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
#include "simd/neon/common.h"
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
    vector_a x3 = x < t3;
    return x1 | x2 | x3;
}

force_inline usize escape_mask_to_done_count(vector_a mask) {
    union {
        vector_a mask;
        u64 parts[2];
    } u;

    u.mask = mask;
    const usize dividesize = (COMPILE_READ_UCS_LEVEL << 3);
    // clz + lsr + csel
    usize a = u64_tz_bits(u.parts[0]) / dividesize;
    usize b = 64 / dividesize + u64_tz_bits(u.parts[1]) / dividesize;
    return u.parts[0] ? a : b;
}

force_inline usize joined4_escape_mask_to_done_count(vector_a mask1,
                                                     vector_a mask2,
                                                     vector_a mask3,
                                                     vector_a mask4) {
    union {
        vector_a mask[4];
        u64 parts[8];
    } u;

    u.mask[0] = mask1;
    u.mask[1] = mask2;
    u.mask[2] = mask3;
    u.mask[3] = mask4;

    const usize dividesize = (COMPILE_READ_UCS_LEVEL << 3);

    for (usize i = 0; i < 8; ++i) {
        if (u.parts[i]) {
            return i * (64 / dividesize) + u64_tz_bits(u.parts[i]) / dividesize;
        }
    }
    SSRJSON_UNREACHABLE();
    return (usize)-1;
}

force_inline usize escape_mask_to_done_count_track_max(vector_a mask, vector_a *max_vec, vector_a src_vec) {
    usize ret = escape_mask_to_done_count(mask);
    vector_a part = low_mask(src_vec, ret);
    *max_vec = unsigned_max(part, *max_vec);
    return ret;
}

force_inline usize joined4_escape_mask_to_done_count_track_max(vector_a mask1,
                                                               vector_a mask2,
                                                               vector_a mask3,
                                                               vector_a mask4,
                                                               vector_a *max_vec,
                                                               unionvector_a_x4 src_vecs) {
    const usize bitsize = 16;
    usize cnt;
    if (!testz(mask1)) {
        cnt = escape_mask_to_done_count(mask1);
        vector_a part1 = low_mask(src_vecs.x[0], cnt);
        *max_vec = unsigned_max(part1, *max_vec);
        return cnt + 0 * bitsize / COMPILE_READ_UCS_LEVEL;
    }
    *max_vec = unsigned_max(src_vecs.x[0], *max_vec);
    if (!testz(mask2)) {
        cnt = escape_mask_to_done_count(mask2);
        vector_a part2 = low_mask(src_vecs.x[1], cnt);
        *max_vec = unsigned_max(part2, *max_vec);
        return cnt + 1 * bitsize / COMPILE_READ_UCS_LEVEL;
    }
    *max_vec = unsigned_max(src_vecs.x[1], *max_vec);
    if (!testz(mask3)) {
        cnt = escape_mask_to_done_count(mask3);
        vector_a part3 = low_mask(src_vecs.x[2], cnt);
        *max_vec = unsigned_max(part3, *max_vec);
        return cnt + 2 * bitsize / COMPILE_READ_UCS_LEVEL;
    }
    *max_vec = unsigned_max(src_vecs.x[2], *max_vec);
    {
        assert(!testz(mask4));
        cnt = escape_mask_to_done_count(mask4);
        vector_a part4 = low_mask(src_vecs.x[3], cnt);
        *max_vec = unsigned_max(part4, *max_vec);
        return cnt + 3 * bitsize / COMPILE_READ_UCS_LEVEL;
    }
}

#include "compile_context/sr_out.inl.h"
#undef COMPILE_SIMD_BITS

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
#        include "simd/simd_detect.h"
#        include "simd/sse2/checker.h"
#        include "simd/sse2/common.h"
#        include "simd/sse4.1/common.h"
#        include "simd/union_vector.h"
#        include "simd/vector_types.h"
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif
//
#include "simd/sse4.1/common.h"
//
#define COMPILE_SIMD_BITS 128
#include "compile_context/sr_in.inl.h"
#include "simd/mask_table.h"

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
    u16 bitmask1, bitmask2, bitmask3, bitmask4;
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

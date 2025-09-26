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
#        include "simd/avx512vl_dq_bw/common.h"
#        include "simd/mask_table.h"
#        include "simd/simd_detect.h"
#        include "simd/union_vector.h"
#        include "simd/vector_types.h"
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif

#define COMPILE_SIMD_BITS 512
#include "compile_context/sr_in.inl.h"

/* High mask functions for AVX512 are not used. */

// force_inline vector_a get_high_mask(usize count) {
//     const vector_a *mask_ptr = read_tail_mask_table_8(64 - count * sizeof(_src_t));
//     return *mask_ptr;
// }

// force_inline vector_a high_mask(vector_a x, usize count) {
//     return x & get_high_mask(count);
// }

force_inline vector_a get_low_mask(usize count) {
    const vector_a *mask_ptr = read_head_mask_table_8(count * sizeof(_src_t));
    return *mask_ptr;
}

force_inline vector_a low_mask(vector_a x, usize count) {
    return x & get_low_mask(count);
}

force_inline avx512_bitmask_t get_escape_bitmask(vector_a x) {
    avx512_bitmask_t bitmask_1 = cmpeq_bitmask(x, broadcast(_Slash));
    avx512_bitmask_t bitmask_2 = cmpeq_bitmask(x, broadcast(_Quote));
    avx512_bitmask_t bitmask_3 = unsigned_cmplt_bitmask(x, broadcast(ControlMax));
    return bitmask_1 | bitmask_2 | bitmask_3;
}

force_inline usize escape_bitmask_to_done_count(avx512_bitmask_t bitmask) {
    if (sizeof(avx512_bitmask_t) == 8) {
        return u64_tz_bits((u64)bitmask);
    }
    assert(sizeof(avx512_bitmask_t) == 2 || sizeof(avx512_bitmask_t) == 4);
    return u32_tz_bits((u32)bitmask);
}

force_inline usize escape_bitmask_to_done_count_track_max(avx512_bitmask_t bitmask, vector_a *max_vec, vector_a src_vec) {
    usize ret = escape_bitmask_to_done_count(bitmask);
    vector_a part = low_mask(src_vec, ret);
    *max_vec = unsigned_max(part, *max_vec);
    return ret;
}

force_inline usize joined4_escape_bitmask_to_done_count(avx512_bitmask_t bitmask1,
                                                        avx512_bitmask_t bitmask2,
                                                        avx512_bitmask_t bitmask3,
                                                        avx512_bitmask_t bitmask4) {
#if COMPILE_READ_UCS_LEVEL == 1
#    define TZBITS u64_tz_bits
#else
#    define TZBITS u32_tz_bits
#endif
#define TOTALBITCOUNT (64 / COMPILE_READ_UCS_LEVEL)
    assert(bitmask1 | bitmask2 | bitmask3 | bitmask4);
    if (bitmask1) return TOTALBITCOUNT * 0 + TZBITS(bitmask1);
    if (bitmask2) return TOTALBITCOUNT * 1 + TZBITS(bitmask2);
    if (bitmask3) return TOTALBITCOUNT * 2 + TZBITS(bitmask3);
    return TOTALBITCOUNT * 3 + TZBITS(bitmask4);
#undef TOTALBITCOUNT
#undef TZBITS
}

force_inline usize joined4_escape_bitmask_to_done_count_track_max(avx512_bitmask_t bitmask1,
                                                                  avx512_bitmask_t bitmask2,
                                                                  avx512_bitmask_t bitmask3,
                                                                  avx512_bitmask_t bitmask4,
                                                                  vector_a *max_vec,
                                                                  unionvector_a_x4 src_vecs) {
#if COMPILE_READ_UCS_LEVEL == 1
#    define TZBITS u64_tz_bits
#else
#    define TZBITS u32_tz_bits
#endif
#define TOTALBITCOUNT (64 / COMPILE_READ_UCS_LEVEL)
    assert(bitmask1 | bitmask2 | bitmask3 | bitmask4);
    if (bitmask1) {
        usize cnt = TZBITS(bitmask1);
        *max_vec = unsigned_max(*max_vec, low_mask(src_vecs.x[0], cnt));
        return TOTALBITCOUNT * 0 + cnt;
    }
    *max_vec = unsigned_max(*max_vec, src_vecs.x[0]);
    if (bitmask2) {
        usize cnt = TZBITS(bitmask2);
        *max_vec = unsigned_max(*max_vec, low_mask(src_vecs.x[1], cnt));
        return TOTALBITCOUNT * 1 + cnt;
    }
    *max_vec = unsigned_max(*max_vec, src_vecs.x[1]);
    if (bitmask3) {
        usize cnt = TZBITS(bitmask3);
        *max_vec = unsigned_max(*max_vec, low_mask(src_vecs.x[2], cnt));
        return TOTALBITCOUNT * 2 + cnt;
    }
    *max_vec = unsigned_max(*max_vec, src_vecs.x[2]);
    {
        assert(bitmask4);
        usize cnt = TZBITS(bitmask4);
        *max_vec = unsigned_max(*max_vec, low_mask(src_vecs.x[3], cnt));
        return TOTALBITCOUNT * 3 + cnt;
    }
#undef TOTALBITCOUNT
#undef TZBITS
}

#include "compile_context/sr_out.inl.h"
#undef COMPILE_SIMD_BITS

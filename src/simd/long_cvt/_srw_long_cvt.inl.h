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
#    include "ssrjson.h"
#    ifndef SSRJSON_SIMD_LONG_CVT_H
#        include "part_back_cvt.h"
#        include "part_cvt.h"
#    endif
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 2
#    endif
#    ifndef COMPILE_SIMD_BITS
#        define COMPILE_SIMD_BITS 256
#    endif
#endif
#include "compile_context/srw_in.inl.h"

force_inline void MAKE_SRW_NAME(__small_back_cvt)(_dst_t **dst_addr, const _src_t **src_addr, usize count, usize max_power2) {
    assert((max_power2 & (max_power2 - 1)) == 0);
    assert(max_power2 > 0 && max_power2 <= 64 && count < max_power2);
    if ((count & 1) && 1 < max_power2) MAKE_RW_NAME(__partial_back_cvt_1)(dst_addr, src_addr);
    if ((count & 2) && 2 < max_power2) MAKE_RW_NAME(__partial_back_cvt_2)(dst_addr, src_addr);
    if ((count & 4) && 4 < max_power2) MAKE_RW_NAME(__partial_back_cvt_4)(dst_addr, src_addr);
    if ((count & 8) && 8 < max_power2) MAKE_RW_NAME(__partial_back_cvt_8)(dst_addr, src_addr);
    if ((count & 16) && 16 < max_power2) MAKE_RW_NAME(__partial_back_cvt_16)(dst_addr, src_addr);
    if ((count & 32) && 32 < max_power2) MAKE_RW_NAME(__partial_back_cvt_32)(dst_addr, src_addr);
}

force_inline void MAKE_SRW_NAME(__small_cvt)(_dst_t **dst_addr, const _src_t **src_addr, usize count, usize max_power2) {
    assert((max_power2 & (max_power2 - 1)) == 0);
    assert(max_power2 > 0 && max_power2 <= 64 && count < max_power2);
    if ((count & 1) && 1 < max_power2) MAKE_RW_NAME(__partial_cvt_1)(dst_addr, src_addr);
    if ((count & 2) && 2 < max_power2) MAKE_RW_NAME(__partial_cvt_2)(dst_addr, src_addr);
    if ((count & 4) && 4 < max_power2) MAKE_RW_NAME(__partial_cvt_4)(dst_addr, src_addr);
    if ((count & 8) && 8 < max_power2) MAKE_RW_NAME(__partial_cvt_8)(dst_addr, src_addr);
    if ((count & 16) && 16 < max_power2) MAKE_RW_NAME(__partial_cvt_16)(dst_addr, src_addr);
    if ((count & 32) && 32 < max_power2) MAKE_RW_NAME(__partial_cvt_32)(dst_addr, src_addr);
}

force_inline void long_back_cvt(_dst_t *dst, const _src_t *src, usize count) {
    // 16, 32 or 64
    static const usize batch_bytes = COMPILE_SIMD_BITS / 8;
    static const usize batch_count = COMPILE_SIMD_BITS / 8 / sizeof(_src_t);
    static const usize batch4_count = COMPILE_SIMD_BITS / 8 / sizeof(_src_t) * 4;
    if (sizeof(_dst_t) > sizeof(_src_t)) {
        usize align_offset = SSRJSON_CAST(uintptr_t, dst) & (batch_bytes - 1);
        assert((align_offset % sizeof(_dst_t)) == 0);
        usize not_aligned_count = align_offset / sizeof(_dst_t);
        not_aligned_count = not_aligned_count > count ? count : not_aligned_count;
        if (not_aligned_count) {
            MAKE_SRW_NAME(__small_back_cvt)(&dst, &src, not_aligned_count, batch_bytes / sizeof(_dst_t));
            count -= not_aligned_count;
        }
    }
    while (count >= batch4_count) {
        src -= batch4_count;
        dst -= batch4_count;
        count -= batch4_count;
        cvt_to_dst(dst + batch_count * 3, *(vector_u *)(src + batch_count * 3));
        cvt_to_dst(dst + batch_count * 2, *(vector_u *)(src + batch_count * 2));
        cvt_to_dst(dst + batch_count * 1, *(vector_u *)(src + batch_count * 1));
        cvt_to_dst(dst + batch_count * 0, *(vector_u *)(src + batch_count * 0));
    }
    while (count >= batch_count) {
        src -= batch_count;
        dst -= batch_count;
        count -= batch_count;
        cvt_to_dst(dst, *(vector_u *)src);
    }
    if (count) {
        MAKE_SRW_NAME(__small_back_cvt)(&dst, &src, count, batch_count);
    }
}

force_inline void long_cvt(_dst_t *dst, const _src_t *src, usize count) {
    // 16, 32 or 64
    static const usize batch_bytes = COMPILE_SIMD_BITS / 8;
    static const usize batch_count = COMPILE_SIMD_BITS / 8 / sizeof(_src_t);
    static const usize batch4_count = COMPILE_SIMD_BITS / 8 / sizeof(_src_t) * 4;
    //
#if COMPILE_WRITE_UCS_LEVEL > COMPILE_READ_UCS_LEVEL
    usize align_offset = SSRJSON_CAST(uintptr_t, dst) & (batch_bytes - 1);
    assert((align_offset % sizeof(_dst_t)) == 0);
    usize not_aligned_count = align_offset / sizeof(_dst_t);
    if (not_aligned_count) {
        not_aligned_count = batch_bytes / sizeof(_dst_t) - not_aligned_count;
        assert(not_aligned_count);
        not_aligned_count = not_aligned_count > count ? count : not_aligned_count;
        MAKE_SRW_NAME(__small_cvt)(&dst, &src, not_aligned_count, batch_bytes / sizeof(_dst_t));
        count -= not_aligned_count;
    }
#endif
    //
    while (count >= batch4_count) {
        cvt_to_dst(dst + batch_count * 0, *(vector_u *)(src + batch_count * 0));
        cvt_to_dst(dst + batch_count * 1, *(vector_u *)(src + batch_count * 1));
        cvt_to_dst(dst + batch_count * 2, *(vector_u *)(src + batch_count * 2));
        cvt_to_dst(dst + batch_count * 3, *(vector_u *)(src + batch_count * 3));
        src += batch4_count;
        dst += batch4_count;
        count -= batch4_count;
    }
    while (count >= batch_count) {
        cvt_to_dst(dst, *(vector_u *)src);
        src += batch_count;
        dst += batch_count;
        count -= batch_count;
    }
    if (count) {
        MAKE_SRW_NAME(__small_cvt)(&dst, &src, count, batch_count);
    }
}

#include "compile_context/srw_out.inl.h"

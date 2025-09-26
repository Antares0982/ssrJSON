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
#    include "simd/avx512vl_dq_bw/checker.h"
#    include "simd/avx512vl_dq_bw/common.h"
#    include "simd/avx512vl_dq_bw/cvt.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif

#endif
//
#define COMPILE_SIMD_BITS 512

#include "compile_context/srw_in.inl.h"

extern const _dst_t ControlEscapeTable[(_Slash + 1) * 8];
extern const Py_ssize_t _ControlJump[_Slash + 1];

force_inline void encode_unicode_loop4(_dst_t **dst_addr, const _src_t **src_addr, usize *len_addr) {
    register usize len = *len_addr;
    register const _src_t *src = *src_addr;
    register _dst_t *dst = *dst_addr;
    while (len >= READ_BATCH_COUNT * 4) {
        union {
            vector_a x[4];
        } union_vec;

        union {
            avx512_bitmask_t x[4];
        } escape_union_vec;

        memcpy(&union_vec, src, sizeof(union_vec));
        for (usize i = 0; i < 4; ++i) {
            cvt_to_dst(dst + READ_BATCH_COUNT * i, union_vec.x[i]);
            escape_union_vec.x[i] = get_escape_bitmask(union_vec.x[i]);
        }
        if (likely(0 == (escape_union_vec.x[0] | escape_union_vec.x[1] | escape_union_vec.x[2] | escape_union_vec.x[3]))) {
            src += 4 * READ_BATCH_COUNT;
            dst += 4 * READ_BATCH_COUNT;
            len -= 4 * READ_BATCH_COUNT;
        } else {
            usize done_count = joined4_escape_bitmask_to_done_count(escape_union_vec.x[0], escape_union_vec.x[1], escape_union_vec.x[2], escape_union_vec.x[3]);
            const _src_t *escape_pos = src + done_count;
            src += done_count + 1;
            _src_t escape_unicode = *escape_pos;
            assert(escape_unicode == _Quote || escape_unicode == _Slash || escape_unicode < ControlMax);
            dst += done_count;
            len -= done_count + 1;
            memcpy(dst, &ControlEscapeTable[escape_unicode * 8], 8 * sizeof(_dst_t));
            dst += _ControlJump[escape_unicode];
        }
    }
    *len_addr = len;
    *src_addr = src;
    *dst_addr = dst;
}

force_inline void encode_unicode_loop(_dst_t **dst_addr, const _src_t **src_addr, usize *len_addr) {
    register usize len = *len_addr;
    register const _src_t *src = *src_addr;
    register _dst_t *dst = *dst_addr;
    while (len >= READ_BATCH_COUNT) {
        vector_a x = *(vector_u *)src;
        avx512_bitmask_t escape_mask = get_escape_bitmask(x);
        cvt_to_dst(dst, x);
        if (likely(!escape_mask)) {
            src += READ_BATCH_COUNT;
            dst += READ_BATCH_COUNT;
            len -= READ_BATCH_COUNT;
        } else {
            u32 done_count = escape_bitmask_to_done_count(escape_mask);
            const _src_t *escape_pos = src + done_count;
            src += done_count + 1;
            _src_t escape_unicode = *escape_pos;
            assert(escape_unicode == _Quote || escape_unicode == _Slash || escape_unicode < ControlMax);
            dst += done_count;
            len -= done_count + 1;
            memcpy(dst, &ControlEscapeTable[escape_unicode * 8], 8 * sizeof(_dst_t));
            dst += _ControlJump[escape_unicode];
        }
    }
    *len_addr = len;
    *src_addr = src;
    *dst_addr = dst;
}

force_inline void encode_trailing_copy_with_cvt(_dst_t **dst_addr, const _src_t *src, usize len) {
    _dst_t *dst = *dst_addr;
    vector_a vec;
    usize maskz = len_to_maskz(len);
    vec = maskz_loadu(maskz, src);
    avx512_bitmask_t bitmask = get_escape_bitmask(vec);
    bitmask = bitmask & maskz;
restart:;
    cvt_to_dst(dst, vec);
    if (likely(!bitmask)) {
        dst += len;
    } else {
        u32 done_count = escape_bitmask_to_done_count(bitmask);
        const _src_t *escape_pos = src + done_count;
        src += done_count + 1;
        len -= done_count + 1;
        _src_t escape_unicode = *escape_pos;
        assert(escape_unicode == _Quote || escape_unicode == _Slash || escape_unicode < ControlMax);
        dst += done_count;
        memcpy(dst, &ControlEscapeTable[escape_unicode * 8], 8 * sizeof(_dst_t));
        dst += _ControlJump[escape_unicode];
        if (len) {
            // no need to compute bitmask again
            bitmask = bitmask >> (done_count + 1);
            vec = maskz_loadu(len_to_maskz(len), src);
            goto restart;
        }
    }

    *dst_addr = dst;
}

force_inline void encode_unicode_impl(_dst_t **dst_addr, const _src_t *src, usize len, bool is_key) {
    if (!is_key) encode_unicode_loop4(dst_addr, &src, &len);
    encode_unicode_loop(dst_addr, &src, &len);
    if (!len) return;
    encode_trailing_copy_with_cvt(dst_addr, src, len);
}

internal_simd_noinline void encode_unicode_impl_no_key(_dst_t **dst_addr, const _src_t *src, usize len) {
    encode_unicode_impl(dst_addr, src, len, false);
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_SIMD_BITS

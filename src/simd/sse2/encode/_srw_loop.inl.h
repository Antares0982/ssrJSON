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
#    include "simd/sse2/checker.h"
#    include "simd/sse2/common.h"
#    include "simd/sse2/cvt.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif
#endif
//
#define COMPILE_SIMD_BITS 128

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
            vector_a x[4];
        } escape_union_vec;

        memcpy(&union_vec, src, sizeof(union_vec));
        for (usize i = 0; i < 4; ++i) {
            cvt_to_dst(dst + READ_BATCH_COUNT * i, union_vec.x[i]);
        }
        for (usize i = 0; i < 4; ++i) {
            escape_union_vec.x[i] = get_escape_mask(union_vec.x[i]);
        }
        if (likely(testz(escape_union_vec.x[0] | escape_union_vec.x[1] | escape_union_vec.x[2] | escape_union_vec.x[3]))) {
            src += 4 * READ_BATCH_COUNT;
            dst += 4 * READ_BATCH_COUNT;
            len -= 4 * READ_BATCH_COUNT;
        } else {
            usize done_count = joined4_escape_mask_to_done_count(escape_union_vec.x[0], escape_union_vec.x[1], escape_union_vec.x[2], escape_union_vec.x[3]);
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
        vector_a escape_mask = get_escape_mask(x);
        cvt_to_dst(dst, x);
        if (likely(testz(escape_mask))) {
            src += READ_BATCH_COUNT;
            dst += READ_BATCH_COUNT;
            len -= READ_BATCH_COUNT;
        } else {
            u32 done_count = escape_mask_to_done_count(escape_mask);
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

force_inline void encode_trailing_copy_with_cvt(_dst_t **dst_addr, const _src_t *src, usize copy_len) {
    _dst_t *dst = *dst_addr;
    assert(copy_len * sizeof(_src_t) < 16);
    const _src_t *const load_start = src + copy_len - 16 / sizeof(_src_t);
    const vector_a vec = *(vector_u *)load_start;
    vector_a old_escape_mask = get_escape_mask(vec);
restart:;
    vector_a escape_mask = high_mask(old_escape_mask, copy_len);
    vector_a vec_shifted = runtime_byte_rshift_128(vec, 16 - copy_len * sizeof(_src_t));
    cvt_to_dst(dst, vec_shifted);
    if (likely(testz(escape_mask))) {
        dst += copy_len;
    } else {
        usize done_count = escape_mask_to_done_count(escape_mask);
        usize real_done_count = done_count + copy_len - READ_BATCH_COUNT;
        dst += real_done_count;
        const _src_t *escape_ptr = src + real_done_count;
        src += real_done_count + 1;
        copy_len -= real_done_count + 1;
        usize unicode = *escape_ptr;
        assert(unicode < ControlMax || unicode == _Slash || unicode == _Quote);
        memcpy(dst, ControlEscapeTable + unicode * 8, 8 * sizeof(_dst_t));
        dst += _ControlJump[unicode];
        if (copy_len) goto restart;
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

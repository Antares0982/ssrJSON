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

#ifdef SSRJSON_CLANGD_CHECKING
#    include "simd/neon/checker.h"
#    include "simd/neon/common.h"
#    include "simd/neon/cvt.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif
#endif

#define _CompileVectorBits 128

#include "compile_context/srw_in.inl.h"

extern const dst_t ControlEscapeTable[256 * CT_ROW_STRIDE];

force_inline void _addr_cvt(dst_t *restrict dst, const src_t *restrict src) {
    for (usize i = 0; i < READ_BATCH_COUNT; ++i) { dst[i] = (dst_t)src[i]; }
}

force_inline ssrjson_nofail dst_t *encode_unicode_loop(register dst_t *dst, const src_t **src_addr, usize *len_addr) {
    register usize len = *len_addr;
    register const src_t *src = *src_addr;
    while (len >= READ_BATCH_COUNT) {
        vector_a x = *(vector_u *)src;
        vector_a escape_mask = get_escape_mask(x);
        _addr_cvt(dst, src);
        if (likely(testz((u128)escape_mask))) {
            src += READ_BATCH_COUNT;
            dst += READ_BATCH_COUNT;
            len -= READ_BATCH_COUNT;
        } else {
            u32 done_count = escape_mask_to_done_count(escape_mask);
            const src_t *escape_pos = src + done_count;
            src += done_count + 1;
            src_t escape_unicode = *escape_pos;
            assert(escape_unicode == _Quote || escape_unicode == _Slash || escape_unicode < _ControlMax);
            dst += done_count;
            len -= done_count + 1;
            memcpy(dst, ControlEscapeTable + escape_unicode * CT_ROW_STRIDE, 8 * sizeof(dst_t));
            dst += *ssrjson_cast(const u64 *, ControlEscapeTable + escape_unicode * CT_ROW_STRIDE + CT_COUNT_OFFSET);
        }
    }
    *len_addr = len;
    *src_addr = src;
    return dst;
}

force_inline ssrjson_nofail dst_t *encode_trailing_copy_with_cvt(register dst_t *dst, const src_t *src,
                                                                 usize copy_len) {
    assert(copy_len * sizeof(src_t) < 16);
    const src_t *const load_start = src + copy_len - 16 / sizeof(src_t);
    const vector_a vec = *(vector_u *)load_start;
    vector_a old_escape_mask = get_escape_mask(vec);
restart:;
    vector_a escape_mask = high_mask(old_escape_mask, copy_len);
    vector_a vec_shifted = runtime_byte_rshift_128(vec, 16 - copy_len * sizeof(src_t));
    _addr_cvt(dst, (const src_t *)&vec_shifted);
    if (likely(testz(escape_mask))) {
        dst += copy_len;
    } else {
        usize done_count = escape_mask_to_done_count(escape_mask);
        usize real_done_count = done_count + copy_len - READ_BATCH_COUNT;
        dst += real_done_count;
        const src_t *escape_ptr = src + real_done_count;
        src += real_done_count + 1;
        copy_len -= real_done_count + 1;
        usize unicode = *escape_ptr;
        assert(unicode < _ControlMax || unicode == _Slash || unicode == _Quote);
        memcpy(dst, ControlEscapeTable + unicode * CT_ROW_STRIDE, 8 * sizeof(dst_t));
        dst += *ssrjson_cast(const u64 *, ControlEscapeTable + unicode * CT_ROW_STRIDE + CT_COUNT_OFFSET);
        if (copy_len) goto restart;
    }
    return dst;
}

// excess written count = ssrjson_max(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode
// 10 >= excess written count >= 2
force_inline ssrjson_nofail dst_t *encode_unicode_impl(dst_t *dst, const src_t *src, usize len) {
    dst = encode_unicode_loop(dst, &src, &len);
    if (len) dst = encode_trailing_copy_with_cvt(dst, src, len);
    return dst;
}

#include "compile_context/srw_out.inl.h"
#undef _CompileVectorBits

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
#define _CompileVectorBits 512

#include "compile_context/srw_in.inl.h"

extern const dst_t ControlEscapeTable[256 * 8];
extern const Py_ssize_t _ControlJump[256];

force_inline ssrjson_nofail dst_t *encode_unicode_loop(register dst_t *dst, const src_t **src_addr, usize *len_addr) {
    register usize len = *len_addr;
    register const src_t *src = *src_addr;
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
            const src_t *escape_pos = src + done_count;
            src += done_count + 1;
            src_t escape_unicode = *escape_pos;
            assert(escape_unicode == _Quote || escape_unicode == _Slash || escape_unicode < _ControlMax);
            dst += done_count;
            len -= done_count + 1;
            // excess write
            memcpy(dst, &ControlEscapeTable[escape_unicode * 8], 8 * sizeof(dst_t));
            dst += _ControlJump[escape_unicode];
        }
    }
    *len_addr = len;
    *src_addr = src;
    return dst;
}

force_inline ssrjson_nofail dst_t *encode_trailing_copy_with_cvt(register dst_t *dst, const src_t *src, usize len) {
    vector_a vec;
    usize maskz = len_to_maskz(len);
    vec = maskz_loadu(maskz, src);
    avx512_bitmask_t bitmask = get_escape_bitmask(vec);
    bitmask = bitmask & maskz;
restart:;
    // excess write
    cvt_to_dst(dst, vec);
    if (likely(!bitmask)) {
        dst += len;
    } else {
        u32 done_count = escape_bitmask_to_done_count(bitmask);
        const src_t *escape_pos = src + done_count;
        src += done_count + 1;
        len -= done_count + 1;
        src_t escape_unicode = *escape_pos;
        assert(escape_unicode == _Quote || escape_unicode == _Slash || escape_unicode < _ControlMax);
        dst += done_count;
        // excess write
        memcpy(dst, &ControlEscapeTable[escape_unicode * 8], 8 * sizeof(dst_t));
        dst += _ControlJump[escape_unicode];
        if (len) {
            // no need to compute bitmask again
            bitmask = bitmask >> (done_count + 1);
            vec = maskz_loadu(len_to_maskz(len), src);
            goto restart;
        }
    }
    return dst;
}

// excess written count = READ_BATCH_COUNT - max_json_bytes_per_unicode
// 58 >= excess written count >= 10
force_inline ssrjson_nofail dst_t *encode_unicode_impl(dst_t *dst, const src_t *src, usize len) {
    dst = encode_unicode_loop(dst, &src, &len);
    if (len) dst = encode_trailing_copy_with_cvt(dst, src, len);
    return dst;
}

#include "compile_context/srw_out.inl.h"
#undef _CompileVectorBits

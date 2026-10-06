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
#    include "simd/scalar/common.h"
#    define COMPILE_READ_UCS_LEVEL 1
#    define COMPILE_WRITE_UCS_LEVEL 1
#endif
#define _CompileVectorBits 128
#include "compile_context/srw_in.inl.h"
extern const dst_t ControlEscapeTable[256 * 8];
extern const Py_ssize_t _ControlJump[256];

force_inline dst_t *encode_unicode_impl(dst_t *dst, const src_t *src, usize len) {
    for (usize i = 0; i < len; ++i) {
        src_t ch = src[i];
        if (ch < _ControlMax || ch == _Quote || ch == _Slash) {
            memcpy(dst, ControlEscapeTable + ch * 8, 8 * sizeof(dst_t));
            dst += _ControlJump[ch];
        } else
            *dst++ = (dst_t)ch;
    }
    return dst;
}

force_inline dst_t *encode_unicode_loop(dst_t *dst, const src_t **src, usize *len) {
    dst = encode_unicode_impl(dst, *src, *len);
    *src += *len;
    *len = 0;
    return dst;
}

force_inline dst_t *encode_trailing_copy_with_cvt(dst_t *dst, const src_t *src, usize len) {
    return encode_unicode_impl(dst, src, len);
}

#include "compile_context/srw_out.inl.h"
#undef _CompileVectorBits

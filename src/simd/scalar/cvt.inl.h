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
#    include "ssrjson.h"
#    define COMPILE_READ_UCS_LEVEL 1
#    define COMPILE_WRITE_UCS_LEVEL 2
#endif

#include "compile_context/rw_in.inl.h"

force_inline void ssrjson_concat4(long_cvt, src_t, dst_t, scalar)(dst_t *dst, const src_t *src, usize len) {
    for (usize i = 0; i < len; ++i) dst[i] = (dst_t)src[i];
}

#if COMPILE_READ_UCS_LEVEL < COMPILE_WRITE_UCS_LEVEL
force_inline void ssrjson_concat4(long_back_cvt, src_t, dst_t, scalar)(dst_t *dst_end, const src_t *src_end,
                                                                       usize len) {
    while (len--) {
        src_t ch;
        memcpy(&ch, --src_end, sizeof(ch));
        *--dst_end = (dst_t)ch;
    }
}

#endif

#include "compile_context/rw_out.inl.h"

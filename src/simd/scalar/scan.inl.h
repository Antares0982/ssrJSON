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
#    include "decode/str/decoder_impl_wrap.h"
#    define COMPILE_READ_UCS_LEVEL 1
#endif

#include "compile_context/r_in.inl.h"

force_inline int scan_string(const src_t **src_addr, const src_t *end, EscapeInfo *escape, src_t *maximum) {
    const src_t *src = *src_addr;
    src_t max_char = *maximum;
#if SSRJSON_IS_X64
    const usize block_count = 64 / sizeof(src_t);
    while ((usize)(end - src) >= block_count) {
        src_t block_max = 0;
        src_t special = 0;
        for (usize i = 0; i < block_count; ++i) {
            src_t ch = src[i];
            if (ch > block_max) block_max = ch;
            special |= (ch < _ControlMax) | (ch == _Quote) | (ch == _Slash);
        }
        if (special) break;
        if (block_max > max_char) max_char = block_max;
        src += block_count;
    }
#endif
    while (src < end) {
        src_t ch = *src;
        if (ch < _ControlMax || ch == _Quote || ch == _Slash) break;
        if (ch > max_char) max_char = ch;
        ++src;
    }
    *maximum = max_char;
    *src_addr = src;
    if (src == end) {
        PyErr_SetString(JSONDecodeError, "Unexpected end of string");
        return DECODE_LOOPSTATE_INVALID;
    }
    if (*src == _Quote) return DECODE_LOOPSTATE_END;
    if (*src == _Slash) {
        *escape = do_decode_escape(src, end);
        return escape->escape_val == _DECODE_UNICODE_ERR ? DECODE_LOOPSTATE_INVALID : DECODE_LOOPSTATE_ESCAPE;
    }
    PyErr_SetString(JSONDecodeError, "Invalid control character in string");
    return DECODE_LOOPSTATE_INVALID;
}

#include "compile_context/r_out.inl.h"

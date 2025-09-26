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
#    include "decode/decode_shared.h"
#    include "decode/str/tools.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif

#include "compile_context/r_in.inl.h"

/* noinline this to reduce binary size */

force_inline EscapeInfo do_decode_escape(const _src_t *const src, const _src_t *src_end) {
    // escape
    EscapeInfo ret;
#define RETURN_ESCAPE(_val_, _size_) \
    ret.escape_val = (_val_);        \
    ret.escape_size = (_size_);      \
    return ret;
#define RETURN_DECODE_ERR RETURN_ESCAPE(_DECODE_UNICODE_ERR, 0)
    switch (src[1]) { // clang-format off
        case '"':  RETURN_ESCAPE('"', 2);
        case '\\': RETURN_ESCAPE('\\', 2);
        case '/':  RETURN_ESCAPE('/', 2);
        case 'b':  RETURN_ESCAPE('\b', 2);
        case 'f':  RETURN_ESCAPE('\f', 2);
        case 'n':  RETURN_ESCAPE('\n', 2);
        case 'r':  RETURN_ESCAPE('\r', 2);
        case 't':  RETURN_ESCAPE('\t', 2);
        // clang-format on
        case 'u': {
            u16 hi;
            if (unlikely(!verify_escape_hex(src, src_end, 2) || !read_to_hex(src + 2, &hi))) {
                if (!PyErr_Occurred()) {
                    PyErr_SetString(JSONDecodeError, "Invalid escape sequence in string");
                }
                RETURN_DECODE_ERR;
            }
            if (likely((hi & 0xF800) != 0xD800)) {
                RETURN_ESCAPE(hi, 6);
            } else {
                u16 lo;
                /* a non-BMP character, represented as a surrogate pair */
                if (unlikely((hi & 0xFC00) != 0xD800)) {
                    PyErr_SetString(JSONDecodeError, "Invalid high surrogate in string");
                    RETURN_DECODE_ERR;
                }
                if (unlikely(src + 12 > src_end || src[6] != '\\' || src[7] != 'u')) {
                    PyErr_SetString(JSONDecodeError, "No low surrogate in string");
                    RETURN_DECODE_ERR;
                }
                if (unlikely(!verify_escape_hex(src, src_end, 8) || !read_to_hex(src + 8, &lo))) {
                    if (!PyErr_Occurred()) {
                        PyErr_SetString(JSONDecodeError, "Invalid escaped sequence in string");
                    }
                    RETURN_DECODE_ERR;
                }
                if (unlikely((lo & 0xFC00) != 0xDC00)) {
                    PyErr_SetString(JSONDecodeError, "Invalid low surrogate in string");
                    RETURN_DECODE_ERR;
                }
                RETURN_ESCAPE(((((u32)hi - 0xD800) << 10) | ((u32)lo - 0xDC00)) + 0x10000, 12);
            }
        }
        default: {
            // invalid
            PyErr_SetString(JSONDecodeError, "Invalid escape sequence in string");
            RETURN_DECODE_ERR;
        }
    }
#undef RETURN_DECODE_ERR
#undef RETURN_ESCAPE
}

internal_simd_noinline EscapeInfo do_decode_escape_noinline(const _src_t *const src, const _src_t *src_end) {
    return do_decode_escape(src, src_end);
}

#include "compile_context/r_out.inl.h"

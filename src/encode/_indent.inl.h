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
#    include "encode/encode_shared.h"
#    include "utils/unicode.h"
#endif

#include "compile_context/iw_in.inl.h"

force_inline void write_unicode_indent(_dst_t **writer_addr, Py_ssize_t _cur_nested_depth) {
#if COMPILE_INDENT_LEVEL > 0
    _dst_t *writer = *writer_addr;
    *writer++ = '\n';
    usize cur_nested_depth = (usize)_cur_nested_depth;
    for (usize i = 0; i < cur_nested_depth; i++) {
        *writer++ = ' ';
        *writer++ = ' ';
#    if COMPILE_INDENT_LEVEL == 4
        *writer++ = ' ';
        *writer++ = ' ';
#    endif // COMPILE_INDENT_LEVEL == 4
    }
    *writer_addr = writer;
#endif // COMPILE_INDENT_LEVEL > 0
}

// forward declaration
force_inline bool unicode_buffer_reserve(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, usize size);

force_inline bool unicode_indent_writer(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj, Py_ssize_t additional_reserve_count) {
    if (!is_in_obj && COMPILE_INDENT_LEVEL != 0) {
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + additional_reserve_count));
        write_unicode_indent(writer_addr, cur_nested_depth);
    } else {
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, additional_reserve_count));
    }
    return true;
}

#include "compile_context/iw_out.inl.h"

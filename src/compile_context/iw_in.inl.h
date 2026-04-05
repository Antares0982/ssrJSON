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

#ifndef SSRJSON_COMPILE_CONTEXT_IW
#define SSRJSON_COMPILE_CONTEXT_IW

#include "w_in.inl.h"

// fake include and definition to deceive clangd
#ifdef SSRJSON_CLANGD_CHECKING
#    include "ssrjson.h"
#    ifndef COMPILE_INDENT_LEVEL
#        define COMPILE_INDENT_LEVEL 2
#    endif
#endif

/*
 * Basic definitions.
 */
#if COMPILE_INDENT_LEVEL == 4
#elif COMPILE_INDENT_LEVEL == 2
#elif COMPILE_INDENT_LEVEL == 0
#else
#    error "COMPILE_INDENT_LEVEL must be 0, 2 or 4"
#endif

#define __INDENT_NAME ssrjson_simple_concat2(indent, COMPILE_INDENT_LEVEL)

/* Generate function names with indent level. */
#define make_i_name(_x_) ssrjson_concat2(_x_, __INDENT_NAME)
/* Generate function names with indent level and writer type. */
#define make_iw_name(_x_) ssrjson_concat3(_x_, __INDENT_NAME, dst_t)

/*
 * Write ndarray indents.
 */
#define ndarray_write_indent ssrjson_concat3(_write_unicode_indent, __INDENT_NAME, u8)

/*
 * Write indents to unicode buffer. Need to reserve space before calling this function.
 */
#define write_unicode_indent make_iw_name(_write_unicode_indent)

/*
 * Write indents to unicode buffer. Will reserve space if needed.
 */
#define unicode_indent_writer make_iw_name(unicode_indent_writer)

#define bytes_buffer_append_key make_i_name(bytes_buffer_append_key)
#define bytes_buffer_append_str make_i_name(bytes_buffer_append_str)
#define bytes_buffer_append_nonascii_key_write_cache make_i_name(bytes_buffer_append_nonascii_key_write_cache)
#define bytes_buffer_append_nonascii_key_no_write_cache make_i_name(bytes_buffer_append_nonascii_key_no_write_cache)
#define encode_bytes_process_val make_i_name(encode_bytes_process_val)
#define ssrjson_dumps_to_bytes_obj make_i_name(ssrjson_dumps_to_bytes_obj)
#define ndarray_traverse_dispatch make_i_name(ndarray_traverse_dispatch)
#define get_ndarray_reserve_cnt make_i_name(get_ndarray_reserve_cnt)
#define u8_buffer_append_ndarray make_i_name(u8_buffer_append_ndarray)
#define get_1darray_reserve_cnt make_i_name(get_1darray_reserve_cnt)
#define get_ndarray_reserve_cnt_internal make_i_name(get_ndarray_reserve_cnt_internal)
#define test_get_ndarray_reserve_cnt_reference make_i_name(test_get_ndarray_reserve_cnt_reference)
#define test_ndarray_reserve_cnt make_i_name(test_ndarray_reserve_cnt)
#endif // SSRJSON_COMPILE_CONTEXT_IW

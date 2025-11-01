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
#ifdef SSRJSON_CLANGD_DUMMY
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

#define __INDENT_NAME SSRJSON_SIMPLE_CONCAT2(indent, COMPILE_INDENT_LEVEL)

#define MAKE_I_NAME(_x_) SSRJSON_CONCAT2(_x_, __INDENT_NAME)
#define MAKE_IW_NAME(_x_) SSRJSON_CONCAT3(_x_, __INDENT_NAME, _dst_t)

/*
 * Write indents to unicode buffer. Need to reserve space before calling this function.
 */
#define write_unicode_indent MAKE_IW_NAME(write_unicode_indent)

/*
 * Write indents to unicode buffer. Will reserve space if needed.
 */
#define unicode_indent_writer MAKE_IW_NAME(unicode_indent_writer)

#define bytes_buffer_append_key MAKE_I_NAME(bytes_buffer_append_key)
#define bytes_buffer_append_str MAKE_I_NAME(bytes_buffer_append_str)
#define bytes_buffer_append_nonascii_key_write_cache MAKE_I_NAME(bytes_buffer_append_nonascii_key_write_cache)
#define bytes_buffer_append_nonascii_key_no_write_cache MAKE_I_NAME(bytes_buffer_append_nonascii_key_no_write_cache)
#define encode_bytes_process_val MAKE_I_NAME(encode_bytes_process_val)
#define ssrjson_dumps_to_bytes_obj MAKE_I_NAME(ssrjson_dumps_to_bytes_obj)

#endif // SSRJSON_COMPILE_CONTEXT_IW

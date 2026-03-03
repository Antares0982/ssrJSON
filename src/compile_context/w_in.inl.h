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

#ifndef SSRJSON_COMPILE_CONTEXT_W
#define SSRJSON_COMPILE_CONTEXT_W

// fake include and definition to deceive clangd
#ifdef SSRJSON_CLANGD_CHECKING
#    include "ssrjson.h"
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif
#endif

/*
 * Basic definitions.
 */
#if COMPILE_WRITE_UCS_LEVEL == 4
#    define WRITE_BIT_SIZE 32
#    define _CAST_WRITER WRITER_AS_U32
#elif COMPILE_WRITE_UCS_LEVEL == 2
#    define WRITE_BIT_SIZE 16
#    define _CAST_WRITER WRITER_AS_U16
#elif COMPILE_WRITE_UCS_LEVEL == 1
#    define WRITE_BIT_SIZE 8
#    define _CAST_WRITER WRITER_AS_U8
#else
#    error "COMPILE_WRITE_UCS_LEVEL must be 1, 2 or 4"
#endif

// The destination type.
#define dst_t ssrjson_simple_concat2(u, WRITE_BIT_SIZE)

/* Generate function names with writer type. */
#define make_w_name(_x_) ssrjson_concat2(_x_, dst_t)

/*
 * Names using W context.
 */
#define unicode_buffer_reserve make_w_name(unicode_buffer_reserve)
#define u64_to_unicode make_w_name(u64_to_unicode)
#define f64_to_unicode make_w_name(f64_to_unicode)
#define inf_nan_to_unicode make_w_name(inf_nan_to_unicode)
#define ControlEscapeTable make_w_name(ControlEscapeTable)

#endif // SSRJSON_COMPILE_CONTEXT_W

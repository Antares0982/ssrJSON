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
#define u_buf_reserve make_w_name(u_buf_reserve)
#define u64_to_unicode make_w_name(u64_to_unicode)
#define u32_to_unicode make_w_name(u32_to_unicode)
#define u16_to_unicode make_w_name(u16_to_unicode)
#define u8_to_unicode make_w_name(u8_to_unicode)
#define f64_to_unicode make_w_name(f64_to_unicode)
#define f32_to_unicode make_w_name(f32_to_unicode)
#define inf_nan_to_unicode make_w_name(inf_nan_to_unicode)
#define ControlEscapeTable make_w_name(ControlEscapeTable)
#define decode_bytes_block make_w_name(decode_bytes_block)
#define decode_bytes_block_scalar make_w_name(decode_bytes_block_scalar)
//
#define write_unicode_bool make_w_name(_write_unicode_bool)
#define write_unicode_bool_numpy make_w_name(_write_unicode_bool_numpy)
#define write_unicode_null make_w_name(_write_unicode_null)
#define write_unicode_empty_arr make_w_name(_write_unicode_empty_arr)
#define write_unicode_arr_begin make_w_name(_write_unicode_arr_begin)
#define write_unicode_arr_end make_w_name(_write_unicode_arr_end)
#define write_unicode_empty_obj make_w_name(_write_unicode_empty_obj)
#define write_unicode_obj_begin make_w_name(_write_unicode_obj_begin)
#define write_unicode_obj_end make_w_name(_write_unicode_obj_end)

#endif // SSRJSON_COMPILE_CONTEXT_W

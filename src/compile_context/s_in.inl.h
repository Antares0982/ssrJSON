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

#ifndef SSRJSON_COMPILE_CONTEXT_S
#define SSRJSON_COMPILE_CONTEXT_S

// fake include and definition to deceive clangd
#ifdef SSRJSON_CLANGD_CHECKING
#    include "ssrjson.h"
#    ifndef _CompileVectorBits
#        define _CompileVectorBits 256
#    endif
#endif

/*
 * Basic definitions.
 */
#if _CompileVectorBits == 128
#    define SIMD_BITS_DOUBLE 256
#elif _CompileVectorBits == 256
#    define SIMD_BITS_DOUBLE 512
#elif _CompileVectorBits == 512
#    define SIMD_BITS_DOUBLE 1024
#else
#    error "_CompileVectorBits must be 128, 256 or 512"
#endif

/* Generate function names with SIMD level. */
#define make_s_name(_x_) ssrjson_concat2(_x_, _CompileVectorBits)

/*
 * Names using S context.
 */
#define broadcast_u8 make_s_name(broadcast_u8)
#define broadcast_u16 make_s_name(broadcast_u16)
#define broadcast_u32 make_s_name(broadcast_u32)
#define setzero make_s_name(setzero)
#define cvt_u16_to_u32 make_s_name(cvt_u16_to_u32)
#define cvt_u8_to_u16 make_s_name(cvt_u8_to_u16)
#define cvt_u8_to_u32 make_s_name(cvt_u8_to_u32)
#define rshift_u16 make_s_name(rshift_u16)
#define rshift_u32 make_s_name(rshift_u32)
#define lshift_u16 make_s_name(lshift_u16)
#define lshift_u32 make_s_name(lshift_u32)
#define get_bitmask_from_u8 make_s_name(get_bitmask_from_u8)
#define testz make_s_name(testz)
#define bytes_write_ucs1_trailing make_s_name(bytes_write_ucs1_trailing)
#define bytes_write_ucs2_trailing make_s_name(bytes_write_ucs2_trailing)
#define bytes_write_ucs4_trailing make_s_name(bytes_write_ucs4_trailing)
#define bytes_write_ucs1_raw_utf8_trailing make_s_name(bytes_write_ucs1_raw_utf8_trailing)
#define bytes_write_ucs2_raw_utf8_trailing make_s_name(bytes_write_ucs2_raw_utf8_trailing)
#define bytes_write_ucs4_raw_utf8_trailing make_s_name(bytes_write_ucs4_raw_utf8_trailing)
#define __excess_bytes_write_ucs1_trailing make_s_name(__excess_bytes_write_ucs1_trailing)
#define __excess_bytes_write_ucs2_trailing make_s_name(__excess_bytes_write_ucs2_trailing)
#define __excess_bytes_write_ucs4_trailing make_s_name(__excess_bytes_write_ucs4_trailing)
#define __excess_bytes_write_ucs1_raw_utf8_trailing make_s_name(__excess_bytes_write_ucs1_raw_utf8_trailing)
#define __excess_bytes_write_ucs2_raw_utf8_trailing make_s_name(__excess_bytes_write_ucs2_raw_utf8_trailing)
#define __excess_bytes_write_ucs4_raw_utf8_trailing make_s_name(__excess_bytes_write_ucs4_raw_utf8_trailing)
#define fast_skip_spaces_u8 make_s_name(fast_skip_spaces_u8)
#define fast_skip_spaces_u16 make_s_name(fast_skip_spaces_u16)
#define fast_skip_spaces_u32 make_s_name(fast_skip_spaces_u32)
//
#define STR_WRITER_NOINDENT_IMPL(r_t, w_t) ssrjson_concat5(_unicode_buffer_append_str_internal, r_t, w_t, indent0, _CompileVectorBits)
#define KEY_WRITER_NOINDENT_IMPL(r_t, w_t) ssrjson_concat5(_unicode_buffer_append_key_internal, r_t, w_t, indent0, _CompileVectorBits)

#endif // SSRJSON_COMPILE_CONTEXT_S

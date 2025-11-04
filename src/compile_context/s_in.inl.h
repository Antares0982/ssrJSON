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
#ifdef SSRJSON_CLANGD_DUMMY
#    include "ssrjson.h"
#    ifndef COMPILE_SIMD_BITS
#        define COMPILE_SIMD_BITS 256
#    endif
#endif

/*
 * Basic definitions.
 */
#if COMPILE_SIMD_BITS == 128
#    define SIMD_BITS_DOUBLE 256
#elif COMPILE_SIMD_BITS == 256
#    define SIMD_BITS_DOUBLE 512
#elif COMPILE_SIMD_BITS == 512
#    define SIMD_BITS_DOUBLE 1024
#else
#    error "COMPILE_SIMD_BITS must be 128, 256 or 512"
#endif

// Name creation macro.
#define MAKE_S_NAME(_x_) SSRJSON_CONCAT2(_x_, COMPILE_SIMD_BITS)

/*
 * Names using S context.
 */
#define broadcast_u8 MAKE_S_NAME(broadcast_u8)
#define broadcast_u16 MAKE_S_NAME(broadcast_u16)
#define broadcast_u32 MAKE_S_NAME(broadcast_u32)
#define setzero MAKE_S_NAME(setzero)
#define cvt_u16_to_u32 MAKE_S_NAME(cvt_u16_to_u32)
#define cvt_u8_to_u16 MAKE_S_NAME(cvt_u8_to_u16)
#define cvt_u8_to_u32 MAKE_S_NAME(cvt_u8_to_u32)
#define rshift_u16 MAKE_S_NAME(rshift_u16)
#define rshift_u32 MAKE_S_NAME(rshift_u32)
#define lshift_u16 MAKE_S_NAME(lshift_u16)
#define lshift_u32 MAKE_S_NAME(lshift_u32)
#define get_bitmask_from_u8 MAKE_S_NAME(get_bitmask_from_u8)
#define testz MAKE_S_NAME(testz)
#define bytes_write_ucs1_trailing MAKE_S_NAME(bytes_write_ucs1_trailing)
#define bytes_write_ucs2_trailing MAKE_S_NAME(bytes_write_ucs2_trailing)
#define bytes_write_ucs4_trailing MAKE_S_NAME(bytes_write_ucs4_trailing)
#define bytes_write_ucs1_raw_utf8_trailing MAKE_S_NAME(bytes_write_ucs1_raw_utf8_trailing)
#define bytes_write_ucs2_raw_utf8_trailing MAKE_S_NAME(bytes_write_ucs2_raw_utf8_trailing)
#define bytes_write_ucs4_raw_utf8_trailing MAKE_S_NAME(bytes_write_ucs4_raw_utf8_trailing)
#define __excess_bytes_write_ucs1_trailing MAKE_S_NAME(__excess_bytes_write_ucs1_trailing)
#define __excess_bytes_write_ucs2_trailing MAKE_S_NAME(__excess_bytes_write_ucs2_trailing)
#define __excess_bytes_write_ucs4_trailing MAKE_S_NAME(__excess_bytes_write_ucs4_trailing)
#define __excess_bytes_write_ucs1_raw_utf8_trailing MAKE_S_NAME(__excess_bytes_write_ucs1_raw_utf8_trailing)
#define __excess_bytes_write_ucs2_raw_utf8_trailing MAKE_S_NAME(__excess_bytes_write_ucs2_raw_utf8_trailing)
#define __excess_bytes_write_ucs4_raw_utf8_trailing MAKE_S_NAME(__excess_bytes_write_ucs4_raw_utf8_trailing)
#define fast_skip_spaces_u8 MAKE_S_NAME(fast_skip_spaces_u8)
#define fast_skip_spaces_u16 MAKE_S_NAME(fast_skip_spaces_u16)
#define fast_skip_spaces_u32 MAKE_S_NAME(fast_skip_spaces_u32)
//
#define STR_WRITER_NOINDENT_IMPL(r_t, w_t) SSRJSON_CONCAT5(_unicode_buffer_append_str_internal, r_t, w_t, indent0, COMPILE_SIMD_BITS)
#define KEY_WRITER_NOINDENT_IMPL(r_t, w_t) SSRJSON_CONCAT5(_unicode_buffer_append_key_internal, r_t, w_t, indent0, COMPILE_SIMD_BITS)

#endif // SSRJSON_COMPILE_CONTEXT_S

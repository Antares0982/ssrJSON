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

#ifndef SSRJSON_COMPILE_CONTEXT_R
#define SSRJSON_COMPILE_CONTEXT_R

// fake include and definition to deceive clangd
#ifdef SSRJSON_CLANGD_DUMMY
#    include "ssrjson.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif

/*
 * Basic definitions.
 */
#if COMPILE_READ_UCS_LEVEL == 4
#    define READ_BIT_SIZE 32
#    define READ_BIT_SIZEx2 64
#    define READ_BIT_SIZEx4 128
#    define READ_BIT_SIZEx8 256
#    define AVX512BITMASK_SIZE 16
#elif COMPILE_READ_UCS_LEVEL == 2
#    define READ_BIT_SIZE 16
#    define READ_BIT_SIZEx2 32
#    define READ_BIT_SIZEx4 64
#    define READ_BIT_SIZEx8 128
#    define AVX512BITMASK_SIZE 32
#elif COMPILE_READ_UCS_LEVEL == 1
#    define READ_BIT_SIZE 8
#    define READ_BIT_SIZEx2 16
#    define READ_BIT_SIZEx4 32
#    define READ_BIT_SIZEx8 64
#    define AVX512BITMASK_SIZE 64
#else
#    error "COMPILE_READ_UCS_LEVEL must be 1, 2 or 4"
#endif

// The source type.
#define _src_t SSRJSON_SIMPLE_CONCAT2(u, READ_BIT_SIZE)

// Other type definitions.
#define avx512_bitmask_t SSRJSON_SIMPLE_CONCAT2(u, AVX512BITMASK_SIZE)

// Name creation macro.
#define MAKE_R_NAME(_x_) SSRJSON_CONCAT2(_x_, _src_t)

#ifdef COMPILE_UCS_LEVEL
#    if COMPILE_UCS_LEVEL == 0
#        define __UCS_NAME ascii
#    else
#        define __UCS_NAME SSRJSON_SIMPLE_CONCAT2(ucs, COMPILE_UCS_LEVEL)
#    endif
#    define MAKE_UCS_NAME(_x_) SSRJSON_CONCAT2(_x_, __UCS_NAME)
#endif
/*
 * Names using R context.
 */
#define cmpeq_2chars MAKE_R_NAME(cmpeq_2chars)
#define verify_escape_hex MAKE_R_NAME(verify_escape_hex)
#define read_to_hex MAKE_R_NAME(read_to_hex)
#define _read_true MAKE_R_NAME(_read_true)
#define _read_false MAKE_R_NAME(_read_false)
#define _read_null MAKE_R_NAME(_read_null)
#define _read_inf MAKE_R_NAME(_read_inf)
#define _read_nan MAKE_R_NAME(_read_nan)
#define read_inf_or_nan MAKE_R_NAME(read_inf_or_nan)
#define do_decode_escape MAKE_R_NAME(do_decode_escape)
#define do_decode_escape_noinline MAKE_R_NAME(do_decode_escape_noinline)
#define _decode_str_loop4_read_src_impl MAKE_R_NAME(_decode_str_loop4_read_src_impl)
#define _decode_str_loop_read_src_impl MAKE_R_NAME(_decode_str_loop_read_src_impl)
#define _decode_str_trailing_read_src_impl MAKE_R_NAME(_decode_str_trailing_read_src_impl)
#define _decode_str_loop4_decoder_impl MAKE_R_NAME(_decode_str_loop4_decoder_impl)
#define _decode_str_loop_decoder_impl MAKE_R_NAME(_decode_str_loop_decoder_impl)
#define _decode_str_trailing_decoder_impl MAKE_R_NAME(_decode_str_trailing_decoder_impl)
#define read_number MAKE_R_NAME(read_number)
#define digi_is_digit MAKE_R_NAME(digi_is_digit)
#define digi_is_digit_or_fp MAKE_R_NAME(digi_is_digit_or_fp)
#define digi_is_exp MAKE_R_NAME(digi_is_exp)
#define digi_is_sign MAKE_R_NAME(digi_is_sign)
#define digi_is_fp MAKE_R_NAME(digi_is_fp)
#define bigint_set_buf MAKE_R_NAME(bigint_set_buf)
#define bigint_set_buf_noinline MAKE_R_NAME(bigint_set_buf_noinline)

#ifdef COMPILE_UCS_LEVEL
#    define decode MAKE_UCS_NAME(decode)
#    define should_read_pretty MAKE_UCS_NAME(should_read_pretty)
#    define decode_root_pretty MAKE_UCS_NAME(decode_root_pretty)
#    define decode_root_minify MAKE_UCS_NAME(decode_root_minify)
#    define decode_root_single MAKE_UCS_NAME(decode_root_single)
#    define check_and_reserve_str_buffer MAKE_UCS_NAME(check_and_reserve_str_buffer)
#    define get_unicode_buffer_final_len MAKE_UCS_NAME(get_unicode_buffer_final_len)
#    define decode_str MAKE_UCS_NAME(decode_str)
#    define decode_str_with_escape MAKE_UCS_NAME(decode_str_with_escape)
#    define make_unicode_from_src MAKE_UCS_NAME(make_unicode_from_src)
#    define decode_str_fast_loop4 MAKE_UCS_NAME(decode_str_fast_loop4)
#    define decode_str_fast_loop MAKE_UCS_NAME(decode_str_fast_loop)
#    define decode_str_fast_trailing MAKE_UCS_NAME(decode_str_fast_trailing)
#    define get_cache_key_hash_and_size MAKE_UCS_NAME(get_cache_key_hash_and_size)
#endif
#endif // SSRJSON_COMPILE_CONTEXT_R

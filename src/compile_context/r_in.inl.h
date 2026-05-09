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
#ifdef SSRJSON_CLANGD_CHECKING
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
#define src_t ssrjson_simple_concat2(u, READ_BIT_SIZE)

// Other type definitions.
#define avx512_bitmask_t ssrjson_simple_concat2(u, AVX512BITMASK_SIZE)

/* Generate function names with reader type. */
#define make_r_name(_x_) ssrjson_concat2(_x_, src_t)

#ifdef COMPILE_UCS_LEVEL
#    if COMPILE_UCS_LEVEL == 0
#        define __UCS_NAME ascii
#    else
#        define __UCS_NAME ssrjson_simple_concat2(ucs, COMPILE_UCS_LEVEL)
#    endif
/* Generate function names with unicode type. */
#    define make_ucs_name(_x_) ssrjson_concat2(_x_, __UCS_NAME)
#endif

/*
 * Names using R context.
 */
#define cmpeq_2chars make_r_name(cmpeq_2chars)
#define verify_escape_hex make_r_name(verify_escape_hex)
#define to_hex make_r_name(to_hex)
#define _read_true make_r_name(_read_true)
#define _read_false make_r_name(_read_false)
#define _read_null make_r_name(_read_null)
#define _read_inf make_r_name(_read_inf)
#define _read_nan make_r_name(_read_nan)
#define loads_inf_or_nan make_r_name(loads_inf_or_nan)
#define do_decode_escape make_r_name(do_decode_escape)
#define do_decode_escape_noinline make_r_name(do_decode_escape_noinline)
#define _decode_str_loop4_read_src_impl make_r_name(_decode_str_loop4_read_src_impl)
#define _decode_str_loop_read_src_impl make_r_name(_decode_str_loop_read_src_impl)
#define _decode_str_trailing_read_src_impl make_r_name(_decode_str_trailing_read_src_impl)
#define _decode_str_loop4_decoder_impl make_r_name(_decode_str_loop4_decoder_impl)
#define _decode_str_loop_decoder_impl make_r_name(_decode_str_loop_decoder_impl)
#define _decode_str_trailing_decoder_impl make_r_name(_decode_str_trailing_decoder_impl)
#define loads_number make_r_name(loads_number)
#define digi_is_digit make_r_name(digi_is_digit)
#define digi_is_digit_or_fp make_r_name(digi_is_digit_or_fp)
#define digi_is_exp make_r_name(digi_is_exp)
#define digi_is_sign make_r_name(digi_is_sign)
#define digi_is_fp make_r_name(digi_is_fp)
#define bigint_set_buf make_r_name(bigint_set_buf)
#define bigint_set_buf_noinline make_r_name(bigint_set_buf_noinline)

/*
 * Names using UCS context.
 */
#ifdef COMPILE_UCS_LEVEL
#    define decode make_ucs_name(decode)
#    define should_loads_pretty make_ucs_name(should_loads_pretty)
#    define loads_root_pretty make_ucs_name(loads_root_pretty)
#    define loads_root_minify make_ucs_name(loads_root_minify)
#    define loads_root_single make_ucs_name(loads_root_single)
#    define check_and_reserve_str_buffer make_ucs_name(check_and_reserve_str_buffer)
#    define get_u_buf_final_len make_ucs_name(get_u_buf_final_len)
#    define decode_str make_ucs_name(decode_str)
#    define decode_str_with_escape make_ucs_name(decode_str_with_escape)
#    define make_unicode_from_src make_ucs_name(make_unicode_from_src)
#    define decode_str_fast_loop4 make_ucs_name(decode_str_fast_loop4)
#    define decode_str_fast_loop make_ucs_name(decode_str_fast_loop)
#    define decode_str_fast_trailing make_ucs_name(decode_str_fast_trailing)
#    define get_cache_key_hash_and_size make_ucs_name(get_cache_key_hash_and_size)
#endif
#endif // SSRJSON_COMPILE_CONTEXT_R

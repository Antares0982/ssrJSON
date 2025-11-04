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

#undef SSRJSON_COMPILE_CONTEXT_R
//
#undef READ_BIT_SIZE
#undef READ_BIT_SIZEx2
#undef READ_BIT_SIZEx4
#undef READ_BIT_SIZEx8
#undef AVX512BITMASK_SIZE
//
#undef _src_t
#undef avx512_bitmask_t
#undef MAKE_R_NAME
//
#undef __UCS_NAME
#undef MAKE_UCS_NAME
//
#undef cmpeq_2chars
#undef verify_escape_hex
#undef read_to_hex
#undef _read_true
#undef _read_false
#undef _read_null
#undef _read_inf
#undef _read_nan
#undef read_inf_or_nan
#undef do_decode_escape
#undef do_decode_escape_noinline
#undef _decode_str_loop4_read_src_impl
#undef _decode_str_loop_read_src_impl
#undef _decode_str_trailing_read_src_impl
#undef _decode_str_loop4_decoder_impl
#undef _decode_str_loop_decoder_impl
#undef _decode_str_trailing_decoder_impl
#undef read_number
#undef digi_is_digit
#undef digi_is_digit_or_fp
#undef digi_is_exp
#undef digi_is_sign
#undef digi_is_fp
#undef bigint_set_buf
#undef bigint_set_buf_noinline
//
#undef decode
#undef should_read_pretty
#undef decode_root_pretty
#undef decode_root_minify
#undef decode_root_single
#undef check_and_reserve_str_buffer
#undef get_unicode_buffer_final_len
#undef decode_str
#undef decode_str_with_escape
#undef make_unicode_from_src
#undef decode_str_fast_loop4
#undef decode_str_fast_loop
#undef decode_str_fast_trailing
#undef get_cache_key_hash_and_size

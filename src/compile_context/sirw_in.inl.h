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

#ifndef SSRJSON_COMPILE_CONTEXT_SIRW
#define SSRJSON_COMPILE_CONTEXT_SIRW
#include "iw_in.inl.h"
#include "srw_in.inl.h"

/* Generate function names with indent level, reader type and writer type. */
#define make_irw_name(_x_) ssrjson_concat4(_x_, src_t, dst_t, __INDENT_NAME)
/* Generate function names with SIMD level, indent level, reader type and writer type. */
#define make_sirw_name(_x_) ssrjson_concat5(_x_, src_t, dst_t, __INDENT_NAME, _CompileVectorBits)

#define u_buf_apd_key make_irw_name(u_buf_apd_key)
#define u_buf_apd_key_rsv_idt make_irw_name(u_buf_apd_key_rsv_idt)
#define u_buf_apd_str make_irw_name(u_buf_apd_str)
#define u_buf_apd_str_rsv_idt make_irw_name(u_buf_apd_str_rsv_idt)
//
#define u_buf_apd_str_u8_u8 ssrjson_concat2(u_buf_apd_str_u8_u8, __INDENT_NAME)
#define u_buf_apd_str_u8_u16 ssrjson_concat2(u_buf_apd_str_u8_u16, __INDENT_NAME)
#define u_buf_apd_str_u8_u32 ssrjson_concat2(u_buf_apd_str_u8_u32, __INDENT_NAME)
#define u_buf_apd_str_u16_u16 ssrjson_concat2(u_buf_apd_str_u16_u16, __INDENT_NAME)
#define u_buf_apd_str_u16_u32 ssrjson_concat2(u_buf_apd_str_u16_u32, __INDENT_NAME)
#define u_buf_apd_str_u32_u32 ssrjson_concat2(u_buf_apd_str_u32_u32, __INDENT_NAME)

#define u_buf_apd_key_u8_u8 ssrjson_concat2(u_buf_apd_key_u8_u8, __INDENT_NAME)
#define u_buf_apd_key_u8_u16 ssrjson_concat2(u_buf_apd_key_u8_u16, __INDENT_NAME)
#define u_buf_apd_key_u8_u32 ssrjson_concat2(u_buf_apd_key_u8_u32, __INDENT_NAME)
#define u_buf_apd_key_u16_u16 ssrjson_concat2(u_buf_apd_key_u16_u16, __INDENT_NAME)
#define u_buf_apd_key_u16_u32 ssrjson_concat2(u_buf_apd_key_u16_u32, __INDENT_NAME)
#define u_buf_apd_key_u32_u32 ssrjson_concat2(u_buf_apd_key_u32_u32, __INDENT_NAME)


#ifdef COMPILE_UCS_LEVEL
/* Generate function names with indent level and unicode type. */
#    define make_iu_name(_x_) ssrjson_concat3(_x_, __UCS_NAME, __INDENT_NAME)
//
#    define prepare_unicode_write make_iu_name(_prepare_unicode_write)
#    define u_buf_apd_key_wrapped make_iu_name(u_buf_apd_key_wrapped)
#    define u_buf_apd_key_distribute2 make_iu_name(u_buf_apd_key_distribute2)
#    define u_buf_apd_key_distribute4 make_iu_name(u_buf_apd_key_distribute4)
#    define u_buf_apd_str_wrapped make_iu_name(u_buf_apd_str_wrapped)
#    define u_buf_apd_str_distribute2 make_iu_name(u_buf_apd_str_distribute2)
#    define u_buf_apd_str_distribute4 make_iu_name(u_buf_apd_str_distribute4)
#    define u_buf_apd_bool make_iu_name(_u_buf_apd_bool)
#    define u_buf_apd_null make_iu_name(_u_buf_apd_null)
#    define u_buf_apd_float make_iu_name(_u_buf_apd_numpy_float)
#    define u_buf_apd_f32 make_iu_name(_u_buf_apd_f32)
#    define u_buf_apd_empty_arr make_iu_name(_u_buf_apd_empty_arr)
#    define u_buf_apd_arr_begin make_iu_name(_u_buf_apd_arr_begin)
#    define u_buf_apd_arr_end make_iu_name(_u_buf_apd_arr_end)
#    define u_buf_apd_empty_obj make_iu_name(_u_buf_apd_empty_obj)
#    define u_buf_apd_obj_begin make_iu_name(_u_buf_apd_obj_begin)
#    define u_buf_apd_obj_end make_iu_name(_u_buf_apd_obj_end)
#    define ssrjson_dumps_obj make_iu_name(_ssrjson_dumps_obj)
//
#    define encode_process_val make_iu_name(encode_process_val)
#endif

#endif // SSRJSON_COMPILE_CONTEXT_SIRW

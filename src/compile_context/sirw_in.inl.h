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

// Name creation macros.
#define MAKE_SIRW_NAME(_x_) SSRJSON_CONCAT5(_x_, _src_t, _dst_t, __INDENT_NAME, COMPILE_SIMD_BITS)

#define unicode_buffer_append_key_internal MAKE_SIRW_NAME(_unicode_buffer_append_key_internal)
#define unicode_buffer_append_str_internal MAKE_SIRW_NAME(_unicode_buffer_append_str_internal)
//
#define STR_WRITER_IMPL(r_t, w_t) SSRJSON_CONCAT5(_unicode_buffer_append_str_internal, r_t, w_t, __INDENT_NAME, COMPILE_SIMD_BITS)
#define KEY_WRITER_IMPL(r_t, w_t) SSRJSON_CONCAT5(_unicode_buffer_append_key_internal, r_t, w_t, __INDENT_NAME, COMPILE_SIMD_BITS)


#ifdef COMPILE_UCS_LEVEL
// Name creation macros.
#    define MAKE_IU_NAME(_x_) SSRJSON_CONCAT3(_x_, __UCS_NAME, __INDENT_NAME)
//
#    define prepare_unicode_write MAKE_IU_NAME(_prepare_unicode_write)
#    define unicode_buffer_append_key MAKE_IU_NAME(_unicode_buffer_append_key)
#    define unicode_buffer_append_key_distribute2 MAKE_IU_NAME(_unicode_buffer_append_key_distribute2)
#    define unicode_buffer_append_key_distribute4 MAKE_IU_NAME(_unicode_buffer_append_key_distribute4)
#    define unicode_buffer_append_str MAKE_IU_NAME(_unicode_buffer_append_str)
#    define unicode_buffer_append_str_distribute2 MAKE_IU_NAME(_unicode_buffer_append_str_distribute2)
#    define unicode_buffer_append_str_distribute4 MAKE_IU_NAME(_unicode_buffer_append_str_distribute4)
#    define unicode_buffer_append_long MAKE_IU_NAME(_unicode_buffer_append_long)
#    define write_unicode_false MAKE_IU_NAME(_write_unicode_false)
#    define unicode_buffer_append_false MAKE_IU_NAME(_unicode_buffer_append_false)
#    define write_unicode_true MAKE_IU_NAME(_write_unicode_true)
#    define unicode_buffer_append_true MAKE_IU_NAME(_unicode_buffer_append_true)
#    define write_unicode_null MAKE_IU_NAME(_write_unicode_null)
#    define unicode_buffer_append_null MAKE_IU_NAME(_unicode_buffer_append_null)
#    define unicode_buffer_append_float MAKE_IU_NAME(_unicode_buffer_append_float)
#    define write_unicode_empty_arr MAKE_IU_NAME(_write_unicode_empty_arr)
#    define unicode_buffer_append_empty_arr MAKE_IU_NAME(_unicode_buffer_append_empty_arr)
#    define write_unicode_arr_begin MAKE_IU_NAME(_write_unicode_arr_begin)
#    define unicode_buffer_append_arr_begin MAKE_IU_NAME(_unicode_buffer_append_arr_begin)
#    define write_unicode_arr_end MAKE_IU_NAME(_write_unicode_arr_end)
#    define unicode_buffer_append_arr_end MAKE_IU_NAME(_unicode_buffer_append_arr_end)
#    define write_unicode_empty_obj MAKE_IU_NAME(_write_unicode_empty_obj)
#    define unicode_buffer_append_empty_obj MAKE_IU_NAME(_unicode_buffer_append_empty_obj)
#    define write_unicode_obj_begin MAKE_IU_NAME(_write_unicode_obj_begin)
#    define unicode_buffer_append_obj_begin MAKE_IU_NAME(_unicode_buffer_append_obj_begin)
#    define write_unicode_obj_end MAKE_IU_NAME(_write_unicode_obj_end)
#    define unicode_buffer_append_obj_end MAKE_IU_NAME(_unicode_buffer_append_obj_end)
#    define ssrjson_dumps_obj MAKE_IU_NAME(_ssrjson_dumps_obj)
//
#    define encode_process_val MAKE_IU_NAME(encode_process_val)
#endif

#endif // SSRJSON_COMPILE_CONTEXT_SIRW

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

/* Generate function names with SIMD level, indent level, reader type and writer type. */
#define make_sirw_name(_x_) ssrjson_concat5(_x_, src_t, dst_t, __INDENT_NAME, _CompileVectorBits)

#define unicode_buffer_append_key_internal make_sirw_name(_unicode_buffer_append_key_internal)
#define unicode_buffer_append_str_internal make_sirw_name(_unicode_buffer_append_str_internal)
//
#define STR_WRITER_IMPL(r_t, w_t) ssrjson_concat5(_unicode_buffer_append_str_internal, r_t, w_t, __INDENT_NAME, _CompileVectorBits)
#define KEY_WRITER_IMPL(r_t, w_t) ssrjson_concat5(_unicode_buffer_append_key_internal, r_t, w_t, __INDENT_NAME, _CompileVectorBits)


#ifdef COMPILE_UCS_LEVEL
/* Generate function names with indent level and unicode type. */
#    define make_iu_name(_x_) ssrjson_concat3(_x_, __UCS_NAME, __INDENT_NAME)
//
#    define prepare_unicode_write make_iu_name(_prepare_unicode_write)
#    define unicode_buffer_append_key make_iu_name(_unicode_buffer_append_key)
#    define unicode_buffer_append_key_non_compact make_iu_name(_unicode_buffer_append_key_non_compact)
#    define unicode_buffer_append_key_distribute2 make_iu_name(_unicode_buffer_append_key_distribute2)
#    define unicode_buffer_append_key_distribute4 make_iu_name(_unicode_buffer_append_key_distribute4)
#    define unicode_buffer_append_key_non_compact_distribute2 make_iu_name(_unicode_buffer_append_key_non_compact_distribute2)
#    define unicode_buffer_append_key_non_compact_distribute4 make_iu_name(_unicode_buffer_append_key_non_compact_distribute4)
#    define unicode_buffer_append_str make_iu_name(_unicode_buffer_append_str)
#    define unicode_buffer_append_str_non_compact_in_obj make_iu_name(_unicode_buffer_append_str_non_compact_in_obj)
#    define unicode_buffer_append_str_non_compact_in_arr make_iu_name(_unicode_buffer_append_str_non_compact_in_arr)
#    define unicode_buffer_append_str_non_compact_impl make_iu_name(_unicode_buffer_append_str_non_compact_impl)
#    define unicode_buffer_append_str_distribute2 make_iu_name(_unicode_buffer_append_str_distribute2)
#    define unicode_buffer_append_str_distribute4 make_iu_name(_unicode_buffer_append_str_distribute4)
#    define unicode_buffer_append_str_non_compact_distribute2 make_iu_name(_unicode_buffer_append_str_non_compact_distribute2)
#    define unicode_buffer_append_str_non_compact_distribute4 make_iu_name(_unicode_buffer_append_str_non_compact_distribute4)
#    define unicode_buffer_append_bool make_iu_name(_unicode_buffer_append_bool)
#    define unicode_buffer_append_null make_iu_name(_unicode_buffer_append_null)
#    define unicode_buffer_append_float make_iu_name(_unicode_buffer_append_numpy_float)
#    define unicode_buffer_append_f32 make_iu_name(_unicode_buffer_append_f32)
#    define unicode_buffer_append_empty_arr make_iu_name(_unicode_buffer_append_empty_arr)
#    define unicode_buffer_append_arr_begin make_iu_name(_unicode_buffer_append_arr_begin)
#    define unicode_buffer_append_arr_end make_iu_name(_unicode_buffer_append_arr_end)
#    define unicode_buffer_append_empty_obj make_iu_name(_unicode_buffer_append_empty_obj)
#    define unicode_buffer_append_obj_begin make_iu_name(_unicode_buffer_append_obj_begin)
#    define unicode_buffer_append_obj_end make_iu_name(_unicode_buffer_append_obj_end)
#    define ssrjson_dumps_obj make_iu_name(_ssrjson_dumps_obj)
//
#    define encode_process_val make_iu_name(encode_process_val)
#endif

#endif // SSRJSON_COMPILE_CONTEXT_SIRW

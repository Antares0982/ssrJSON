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

#ifdef SSRJSON_CLANGD_CHECKING
#    ifndef COMPILE_CONTEXT_ENCODE
#        define COMPILE_CONTEXT_ENCODE
#    endif
#    ifndef COMPILE_INDENT_LEVEL
#        include "encode/encode_scalar.h"
#        include "encode/encode_shared.h"
#        include "encode/indent_writer.h"
#        include "simd/simd_detect.h"
#        include "simd/simd_impl.h"
#        include "utils/unicode.h"
#        define COMPILE_INDENT_LEVEL 0
#        define COMPILE_READ_UCS_LEVEL 1
#        define COMPILE_WRITE_UCS_LEVEL 1
#        include "simd/compile_feature_check.h"
#    endif
#endif

#include "compile_context/sirw_in.inl.h"

/* Local non-mangled name builders (no SIMD bits suffix). */
#define _NC_STR_VAL_NAME ssrjson_concat4(_unicode_buffer_append_str_value_non_compact, src_t, dst_t, __INDENT_NAME)
#define _NC_STR_VAL_IN_OBJ_NAME ssrjson_concat4(_unicode_buffer_append_str_value_non_compact_in_obj, src_t, dst_t, __INDENT_NAME)
#define _NC_STR_VAL_IN_ARR_NAME ssrjson_concat4(_unicode_buffer_append_str_value_non_compact_in_arr, src_t, dst_t, __INDENT_NAME)
#define _NC_KEY_NAME ssrjson_concat4(_unicode_buffer_append_key_non_compact, src_t, dst_t, __INDENT_NAME)
/* Reference to the force_inline scalar/SIMD-128 encoder defined in encode_scalar.h */
#define _NC_ENCODE_BODY ssrjson_concat3(encode_unicode_noncompact, src_t, dst_t)

force_noinline dst_t *_NC_KEY_NAME(const src_t *str_data, usize len, dst_t *writer,
                                   EncodeUnicodeBufferInfo *unicode_buffer_info,
                                   Py_ssize_t cur_nested_depth) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    {
        const usize excess_count_before = get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 1;
        const usize reserve_count_in_encoding = max_json_bytes_per_unicode * len;
        const usize excess_count_in_encoding = ssrjson_max(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode;
        usize excess_count_after = (COMPILE_INDENT_LEVEL > 0) ? 4 : 2;
        excess_count_after = ssrjson_max(excess_count_after, excess_count_in_encoding);
        writer = unicode_buffer_reserve(writer, unicode_buffer_info, excess_count_before + reserve_count_in_encoding + excess_count_after);
        return_if_unlikely(!writer);
    }
    writer = write_unicode_indent(writer, cur_nested_depth);
    *writer++ = '"';
    writer = _NC_ENCODE_BODY(writer, str_data, len);
    *writer++ = '"';
    *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
    *writer++ = ' ';
#    if COMPILE_WRITE_UCS_LEVEL < 4
    *writer = 0;
#    endif
#endif
    assert(check_unicode_writer_valid(writer, unicode_buffer_info));
    return writer;
}

force_inline dst_t *_NC_STR_VAL_NAME(const src_t *str_data, usize len, dst_t *writer,
                                     EncodeUnicodeBufferInfo *unicode_buffer_info,
                                     Py_ssize_t cur_nested_depth, ssrjson_compiletime bool is_in_obj) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    const usize reserve_count_in_encoding = max_json_bytes_per_unicode * len;
    const usize excess_count_in_encoding = ssrjson_max(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode;
    usize excess_count_after = 2;
    excess_count_after = ssrjson_max(excess_count_after, excess_count_in_encoding);
    if (ssrjson_consteval(is_in_obj)) {
        const usize excess_count_before = 1;
        writer = unicode_buffer_reserve(writer, unicode_buffer_info, excess_count_before + reserve_count_in_encoding + excess_count_after);
        return_if_unlikely(!writer);
    } else {
        const usize excess_count_before = get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 1;
        writer = unicode_buffer_reserve(writer, unicode_buffer_info, excess_count_before + reserve_count_in_encoding + excess_count_after);
        return_if_unlikely(!writer);
        writer = write_unicode_indent(writer, cur_nested_depth);
    }
    *writer++ = '"';
    writer = _NC_ENCODE_BODY(writer, str_data, len);
    *writer++ = '"';
    *writer++ = ',';
    assert(check_unicode_writer_valid(writer, unicode_buffer_info));
    return writer;
}

force_noinline dst_t *_NC_STR_VAL_IN_OBJ_NAME(const src_t *str_data, usize len, dst_t *writer,
                                              EncodeUnicodeBufferInfo *unicode_buffer_info,
                                              Py_ssize_t cur_nested_depth) {
    return _NC_STR_VAL_NAME(str_data, len, writer, unicode_buffer_info, cur_nested_depth, true);
}

force_noinline dst_t *_NC_STR_VAL_IN_ARR_NAME(const src_t *str_data, usize len, dst_t *writer,
                                              EncodeUnicodeBufferInfo *unicode_buffer_info,
                                              Py_ssize_t cur_nested_depth) {
    return _NC_STR_VAL_NAME(str_data, len, writer, unicode_buffer_info, cur_nested_depth, false);
}

#undef _NC_ENCODE_BODY
#undef _NC_KEY_NAME
#undef _NC_STR_VAL_IN_ARR_NAME
#undef _NC_STR_VAL_IN_OBJ_NAME
#undef _NC_STR_VAL_NAME

#include "compile_context/sirw_out.inl.h"

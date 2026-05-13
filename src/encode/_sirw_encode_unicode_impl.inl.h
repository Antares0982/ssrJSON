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
#        include "bytes/encode_utf8.h"
#        include "encode/indent_writer.h"
#        include "encode_shared.h"
#        include "simd/simd_detect.h"
#        include "simd/simd_impl.h"
#        include "utils/unicode.h"
#        define COMPILE_INDENT_LEVEL 0
#        define COMPILE_READ_UCS_LEVEL 1
#        define COMPILE_WRITE_UCS_LEVEL 1
#        include "_rw_encode_unicode_impl.inl.h"
#        include "simd/compile_feature_check.h"
#    endif
#endif

/* Macro IN */
#include "compile_context/sirw_in.inl.h"

force_inline dst_t *u_buf_apd_key_rsv_idt(dst_t *writer, usize len, EncodeUBufInfo *u_buf_info,
                                          usize cur_nested_depth) {
    // write_unicode_indent and '"' writes `get_indent_char_count() + 1` unicodes
    // max_json_bytes_per_unicode * len is the written count when every character needs to be escaped
    // excess `ssrjson_max(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode` unicodes written
    // in encode_unicode_impl (see comments in AVX2 impl of encode_unicode_impl)
    // when indent level > 0, more 4 unicodes are written, else 2 unicodes
    const usize excess_count_before = get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 1;
    const usize reserve_count_in_encoding = max_json_bytes_per_unicode * len;
    const usize excess_count_in_encoding = ssrjson_max(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode;
    usize excess_count_after = (COMPILE_INDENT_LEVEL > 0) ? 4 : 2;
    excess_count_after = ssrjson_max(excess_count_after, excess_count_in_encoding);

    writer = u_buf_reserve(writer, u_buf_info, excess_count_before + reserve_count_in_encoding + excess_count_after);
    if (likely(writer)) writer = write_unicode_indent(writer, cur_nested_depth);
    return writer;
}

force_inline dst_t *u_buf_apd_key(const src_t *str_data, usize len, dst_t *writer, EncodeUBufInfo *u_buf_info,
                                  usize cur_nested_depth, ssrjson_compiletime bool is_compact) {
    static_assert(
            COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    writer = u_buf_apd_key_rsv_idt(writer, len, u_buf_info, cur_nested_depth);
    if (likely(writer)) {
        writer = u_buf_apd_key_impl(writer, str_data, len, is_compact);
#if COMPILE_INDENT_LEVEL > 0
        *writer++ = ' ';
#    if COMPILE_WRITE_UCS_LEVEL < 4
        *writer = 0;
#    endif // COMPILE_WRITE_UCS_LEVEL < 4
#endif     // COMPILE_INDENT_LEVEL > 0
        assert(check_unicode_writer_valid(writer, u_buf_info));
    }
    return writer;
}

force_inline dst_t *u_buf_apd_str_rsv_idt(dst_t *writer, usize len, EncodeUBufInfo *u_buf_info, usize cur_nested_depth,
                                          ssrjson_compiletime bool is_in_obj) {
    const usize reserve_count_in_encoding = max_json_bytes_per_unicode * len;
    const usize excess_count_in_encoding = ssrjson_max(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode;
    usize excess_count_after = 2;
    excess_count_after = ssrjson_max(excess_count_after, excess_count_in_encoding);
    if (ssrjson_consteval(is_in_obj)) {
        // '"' writes 1 unicode
        // max_json_bytes_per_unicode * len is the written count when every character needs to be escaped
        // excess `ssrjson_max(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode` unicodes written
        // in encode_unicode_impl_no_key (see comments in AVX2 impl of encode_unicode_impl)
        // '"' and ',': 2 unicodes
        const usize excess_count_before = 1;
        return u_buf_reserve(writer, u_buf_info, excess_count_before + reserve_count_in_encoding + excess_count_after);
    } else {
        // write_unicode_indent and '"' writes `get_indent_char_count() + 1` unicodes
        // max_json_bytes_per_unicode * len is the written count when every character needs to be escaped
        // excess `ssrjson_max(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode` unicodes written
        // in encode_unicode_impl_no_key (see comments in AVX2 impl of encode_unicode_impl)
        // '"' and ',': 2 unicodes
        const usize excess_count_before = get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 1;
        writer = u_buf_reserve(
                writer, u_buf_info, excess_count_before + reserve_count_in_encoding + excess_count_after);
        if (likely(writer)) writer = write_unicode_indent(writer, cur_nested_depth);
        return writer;
    }
}

force_inline dst_t *u_buf_apd_str(dst_t *writer, const src_t *str_data, usize len, EncodeUBufInfo *u_buf_info,
                                  usize cur_nested_depth, ssrjson_compiletime bool is_in_obj,
                                  ssrjson_compiletime bool is_compact) {
    static_assert(
            COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    //
    writer = u_buf_apd_str_rsv_idt(writer, len, u_buf_info, cur_nested_depth, is_in_obj);
    if (likely(writer)) {
        writer = u_buf_apd_str_impl(writer, str_data, len, is_compact);
        assert(check_unicode_writer_valid(writer, u_buf_info));
    }
    return writer;
}

#include "compile_context/sirw_out.inl.h"

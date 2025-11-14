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

#ifdef SSRJSON_CLANGD_DUMMY
#    ifndef COMPILE_CONTEXT_ENCODE
#        define COMPILE_CONTEXT_ENCODE
#    endif
#    ifndef COMPILE_INDENT_LEVEL
#        include "encode/indent_writer.h"
#        include "encode_shared.h"
#        include "simd/simd_detect.h"
#        include "simd/simd_impl.h"
#        include "utils/unicode.h"
#        define COMPILE_INDENT_LEVEL 0
#        define COMPILE_READ_UCS_LEVEL 1
#        define COMPILE_WRITE_UCS_LEVEL 1
#        include "simd/compile_feature_check.h"
#    endif
#endif

/* Macro IN */
#include "compile_context/sirw_in.inl.h"

force_inline bool unicode_buffer_append_key_internal(const _src_t *str_data, usize len, _dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    {
        // write_unicode_indent and '"' writes `get_indent_char_count() + 1` unicodes
        // max_json_bytes_per_unicode * len is the written count when every character needs to be escaped
        // excess `SSRJSON_MAX(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode` unicodes written in encode_unicode_impl (see comments in AVX2 impl of encode_unicode_impl)
        // when indent level > 0, more 4 unicodes are written, else 2 unicodes
        const usize excess_count_before = get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 1;
        const usize reserve_count_in_encoding = max_json_bytes_per_unicode * len;
        const usize excess_count_in_encoding = SSRJSON_MAX(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode;
        usize excess_count_after = (COMPILE_INDENT_LEVEL > 0) ? 4 : 2;
        excess_count_after = SSRJSON_MAX(excess_count_after, excess_count_in_encoding);
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, excess_count_before + reserve_count_in_encoding + excess_count_after));
    }
    _dst_t *writer = *writer_addr;
    write_unicode_indent(&writer, cur_nested_depth);
    *writer++ = '"';
    encode_unicode_impl(&writer, str_data, len, true);
    *writer++ = '"';
    *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
    *writer++ = ' ';
#    if COMPILE_WRITE_UCS_LEVEL < 4
    *writer = 0;
#    endif // COMPILE_WRITE_UCS_LEVEL < 4
#endif     // COMPILE_INDENT_LEVEL > 0
    *writer_addr = writer;
    assert(check_unicode_writer_valid(writer, unicode_buffer_info));
    return true;
}

force_inline bool unicode_buffer_append_str_internal(const _src_t *str_data, usize len, _dst_t **writer_addr,
                                                     EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    _dst_t *writer;
    //
    const usize reserve_count_in_encoding = max_json_bytes_per_unicode * len;
    const usize excess_count_in_encoding = SSRJSON_MAX(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode;
    usize excess_count_after = 2;
    excess_count_after = SSRJSON_MAX(excess_count_after, excess_count_in_encoding);
    if (is_in_obj) {
        // '"' writes 1 unicode
        // max_json_bytes_per_unicode * len is the written count when every character needs to be escaped
        // excess `SSRJSON_MAX(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode` unicodes written in encode_unicode_impl_no_key (see comments in AVX2 impl of encode_unicode_impl)
        // '"' and ',': 2 unicodes
        const usize excess_count_before = 1;
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, excess_count_before + reserve_count_in_encoding + excess_count_after));
        writer = *writer_addr;
    } else {
        // write_unicode_indent and '"' writes `get_indent_char_count() + 1` unicodes
        // max_json_bytes_per_unicode * len is the written count when every character needs to be escaped
        // excess `SSRJSON_MAX(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode` unicodes written in encode_unicode_impl_no_key (see comments in AVX2 impl of encode_unicode_impl)
        // '"' and ',': 2 unicodes
        const usize excess_count_before = get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 1;
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, excess_count_before + reserve_count_in_encoding + excess_count_after));
        writer = *writer_addr;
        write_unicode_indent(&writer, cur_nested_depth);
    }
    *writer++ = '"';
    encode_unicode_impl_no_key(&writer, str_data, len);
    *writer++ = '"';
    *writer++ = ',';
    *writer_addr = writer;
    assert(check_unicode_writer_valid(writer, unicode_buffer_info));
    return true;
}

#include "compile_context/sirw_out.inl.h"

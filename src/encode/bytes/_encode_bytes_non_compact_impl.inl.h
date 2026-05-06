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
#        include "encode/bytes/encode_utf8.h"
#        include "encode/encode_scalar.h"
#        include "encode/encode_shared.h"
#        include "encode/encode_utf8_shared.h"
#        include "encode/indent_writer.h"
#        include "pyutils.h"
#        include "simd/simd_detect.h"
#        include "simd/simd_impl.h"
#        define COMPILE_INDENT_LEVEL 0
#        define COMPILE_READ_UCS_LEVEL 1
#        define COMPILE_WRITE_UCS_LEVEL 1
#        include "simd/compile_feature_check.h"
#    endif
#endif

#include "compile_context/sirw_in.inl.h"

/* Local non-mangled name builders. dst_t is fixed at u8 (the bytes path). */
#define _NC_BYTES_VAL_NAME ssrjson_concat3(_bytes_buffer_append_str_value_non_compact, src_t, __INDENT_NAME)
#define _NC_BYTES_VAL_IN_OBJ_NAME ssrjson_concat3(_bytes_buffer_append_str_value_non_compact_in_obj, src_t, __INDENT_NAME)
#define _NC_BYTES_VAL_IN_ARR_NAME ssrjson_concat3(_bytes_buffer_append_str_value_non_compact_in_arr, src_t, __INDENT_NAME)
#define _NC_BYTES_KEY_NAME ssrjson_concat3(_bytes_buffer_append_key_non_compact, src_t, __INDENT_NAME)
#define _NC_BYTES_BODY_FN ssrjson_concat3(_nc_bytes_encode_body, src_t, __INDENT_NAME)
/* Per-kind helpers in encode_scalar.h: encode_bytes_noncompact_<ucsk>. */
#if COMPILE_READ_UCS_LEVEL == 1
#    define _NC_BYTES_BODY encode_bytes_noncompact_ucs1
#elif COMPILE_READ_UCS_LEVEL == 2
#    define _NC_BYTES_BODY encode_bytes_noncompact_ucs2
#else
#    define _NC_BYTES_BODY encode_bytes_noncompact_ucs4
#endif
/* PyUnicode_KIND value matching src_t. */
#define _NC_BYTES_PYKIND COMPILE_READ_UCS_LEVEL

/* Inline helper: encode body bytes (no surrounding quotes). Returns NULL on
 * surrogate failure (ucs2/ucs4) or write_cache_impl OOM. */
force_inline u8 *_NC_BYTES_BODY_FN(u8 *writer, const src_t *src, usize len, PyObject *str, bool is_write_cache) {
#if COMPILE_READ_UCS_LEVEL == 1
    /* For ucs1 (kind=1), strings can be ASCII or 1-byte non-ASCII. The
     * encode_bytes_noncompact_ucs1 helper handles both via per-byte dispatch.
     * Cache only kicks in for non-ASCII (PyCompactUnicodeObject.utf8 is the
     * cache slot; ASCII strings don't allocate it).
     */
    bool is_ascii = PyUnicode_IS_ASCII(str);
    if (likely(is_ascii)) {
        return _NC_BYTES_BODY(writer, src, len);
    }
#endif
    if (is_write_cache) {
        const u8 *utf8_cache;
        usize utf8_length;
        get_utf8_cache(str, &utf8_cache, &utf8_length);
        if (!utf8_cache) {
            if (unlikely(!nc_write_cache_impl(_NC_BYTES_PYKIND, src, len, &utf8_cache, &utf8_length))) return NULL;
            set_cache(str, &utf8_cache, utf8_length);
        }
        assert(utf8_cache);
        if (USING_AVX512 || utf8_length >= 16) {
            return bytes_write_utf8(writer, utf8_cache, utf8_length, true);
        }
        return _NC_BYTES_BODY(writer, src, len);
    } else {
        const u8 *utf8_cache;
        usize utf8_length;
        get_utf8_cache(str, &utf8_cache, &utf8_length);
        if (utf8_cache && (USING_AVX512 || utf8_length >= 16)) {
            return bytes_write_utf8(writer, utf8_cache, utf8_length, true);
        }
        return _NC_BYTES_BODY(writer, src, len);
    }
}

force_noinline u8 *_NC_BYTES_KEY_NAME(u8 *writer, const src_t *src_data, usize len, PyObject *key,
                                      EncodeUnicodeBufferInfo *unicode_buffer_info,
                                      Py_ssize_t cur_nested_depth, bool is_write_cache) {
    /* indent + " + body + " + : (+ ' ' if indent>0) */
    const usize excess_bytes_before = get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 1;
    const usize reserve_bytes_in_encoding = max_json_bytes_per_unicode * len;
    usize excess_bytes_in_encoding = 16 - max_json_bytes_per_unicode;
    excess_bytes_in_encoding = ssrjson_max(excess_bytes_in_encoding, __excess_bytes_write_ucs1_trailing);
    excess_bytes_in_encoding = ssrjson_max(excess_bytes_in_encoding, __excess_bytes_write_ucs2_trailing);
    excess_bytes_in_encoding = ssrjson_max(excess_bytes_in_encoding, __excess_bytes_write_ucs4_trailing);
    const usize excess_bytes_after = excess_bytes_in_encoding;
    writer = unicode_buffer_reserve(writer, unicode_buffer_info, excess_bytes_before + reserve_bytes_in_encoding + excess_bytes_after);
    return_if_unlikely(!writer);
    writer = write_unicode_indent(writer, cur_nested_depth);
    *writer++ = '"';
    writer = _NC_BYTES_BODY_FN(writer, src_data, len, key, is_write_cache);
    return_if_unlikely(!writer);
    *writer++ = '"';
    *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
    *writer++ = ' ';
#endif
    return writer;
}

force_inline u8 *_NC_BYTES_VAL_NAME(u8 *writer, const src_t *src_data, usize len, PyObject *str,
                                    EncodeUnicodeBufferInfo *unicode_buffer_info,
                                    Py_ssize_t cur_nested_depth, ssrjson_compiletime bool is_in_obj, bool is_write_cache) {
    const usize reserve_bytes_in_encoding = max_json_bytes_per_unicode * len;
    usize excess_bytes_in_encoding = 16 - max_json_bytes_per_unicode;
    excess_bytes_in_encoding = ssrjson_max(excess_bytes_in_encoding, __excess_bytes_write_ucs1_trailing);
    excess_bytes_in_encoding = ssrjson_max(excess_bytes_in_encoding, __excess_bytes_write_ucs2_trailing);
    excess_bytes_in_encoding = ssrjson_max(excess_bytes_in_encoding, __excess_bytes_write_ucs4_trailing);
    const usize excess_bytes_after = excess_bytes_in_encoding;
    if (ssrjson_consteval(is_in_obj)) {
        const usize excess_bytes_before = 1;
        writer = unicode_buffer_reserve(writer, unicode_buffer_info, excess_bytes_before + reserve_bytes_in_encoding + excess_bytes_after);
        return_if_unlikely(!writer);
    } else {
        const usize excess_bytes_before = get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 1;
        writer = unicode_buffer_reserve(writer, unicode_buffer_info, excess_bytes_before + reserve_bytes_in_encoding + excess_bytes_after);
        return_if_unlikely(!writer);
        writer = write_unicode_indent(writer, cur_nested_depth);
    }
    *writer++ = '"';
    writer = _NC_BYTES_BODY_FN(writer, src_data, len, str, is_write_cache);
    return_if_unlikely(!writer);
    *writer++ = '"';
    *writer++ = ',';
    return writer;
}

force_noinline u8 *_NC_BYTES_VAL_IN_OBJ_NAME(u8 *writer, const src_t *src_data, usize len, PyObject *str,
                                             EncodeUnicodeBufferInfo *unicode_buffer_info,
                                             Py_ssize_t cur_nested_depth, bool is_write_cache) {
    return _NC_BYTES_VAL_NAME(writer, src_data, len, str, unicode_buffer_info, cur_nested_depth, true, is_write_cache);
}

force_noinline u8 *_NC_BYTES_VAL_IN_ARR_NAME(u8 *writer, const src_t *src_data, usize len, PyObject *str,
                                             EncodeUnicodeBufferInfo *unicode_buffer_info,
                                             Py_ssize_t cur_nested_depth, bool is_write_cache) {
    return _NC_BYTES_VAL_NAME(writer, src_data, len, str, unicode_buffer_info, cur_nested_depth, false, is_write_cache);
}

#undef _NC_BYTES_PYKIND
#undef _NC_BYTES_BODY
#undef _NC_BYTES_BODY_FN
#undef _NC_BYTES_KEY_NAME
#undef _NC_BYTES_VAL_IN_ARR_NAME
#undef _NC_BYTES_VAL_IN_OBJ_NAME
#undef _NC_BYTES_VAL_NAME

#include "compile_context/sirw_out.inl.h"

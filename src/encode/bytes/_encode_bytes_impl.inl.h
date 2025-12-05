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
#    include "encode/bytes/encode_utf8.h"
#    include "encode/encode_impl_wrap.h"
#    include "encode/encode_shared.h"
#    include "non_ascii.h"
#    include "pyutils.h"
#    include "ssrjson.h"
#    include "tls.h"
#    include "utils/unicode.h"
//
#    ifndef COMPILE_INDENT_LEVEL
#        define COMPILE_INDENT_LEVEL 2
#    endif
#endif

// use writer: ascii
#include "simd/compile_feature_check.h"
#define COMPILE_UCS_LEVEL 0
#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
//
#include "compile_context/sirw_in.inl.h"

force_inline bool bytes_buffer_append_nonascii_key_write_cache(u8 **writer_addr, int src_pykind, const void *src_voidp, usize len, PyObject *key) {
    assert(SSRJSON_PYASCII_CAST(key)->state.compact);
    const u8 *utf8_cache;
    usize utf8_length;
    get_utf8_cache(key, &utf8_cache, &utf8_length);
    if (!utf8_cache) {
        if (!USING_AVX512 && len < 6) {
            // For short strings, directly encode without caching
            // Why is 6: we assume that each character encodes to 3 bytes in most cases,
            // and 3 * 6 = 18 >= 16.
            goto no_cache_encode;
        }
        if (unlikely(!write_key_cache_impl(src_voidp, src_pykind, len, &utf8_cache, &utf8_length))) return false;
        set_cache(key, &utf8_cache, &utf8_length);
    }
    assert(utf8_cache);
    // Also see comment in bytes_write_utf8
    if (USING_AVX512 || utf8_length >= 16) {
        bytes_write_utf8(writer_addr, utf8_cache, utf8_length, true);
        u8 *writer = *writer_addr;
        *writer++ = '"';
        *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
        *writer++ = ' ';
        *writer = 0;
#endif // COMPILE_INDENT_LEVEL > 0
        *writer_addr = writer;
        return true;
    } else {
    no_cache_encode:;
        switch (src_pykind) {
            case 1: {
                bytes_write_ucs1(writer_addr, src_voidp, len, true);
                break;
            }
            case 2: {
                if (unlikely(!bytes_write_ucs2(writer_addr, src_voidp, len, true))) return false;
                break;
            }
            case 4: {
                if (unlikely(!bytes_write_ucs4(writer_addr, src_voidp, len, true))) return false;
                break;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
        u8 *writer = *writer_addr;
        *writer++ = '"';
        *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
        *writer++ = ' ';
        *writer = 0;
#endif // COMPILE_INDENT_LEVEL > 0
        *writer_addr = writer;
        return true;
    }
}

static force_noinline bool bytes_buffer_append_nonascii_key_no_write_cache(u8 **writer_addr, int src_pykind, const void *src_voidp, usize len, PyObject *str) {
    assert(SSRJSON_CAST(PyASCIIObject *, str)->state.compact);
    const u8 *utf8_cache;
    usize utf8_length;
    get_utf8_cache(str, &utf8_cache, &utf8_length);
    // Also see comment in bytes_write_utf8
    if (utf8_cache && (USING_AVX512 || utf8_length >= 16)) {
        bytes_write_utf8(writer_addr, utf8_cache, utf8_length, true);
    } else {
        switch (src_pykind) {
            case 1: {
                bytes_write_ucs1(writer_addr, src_voidp, len, true);
                break;
            }
            case 2: {
                if (unlikely(!bytes_write_ucs2(writer_addr, src_voidp, len, true))) return false;
                break;
            }
            case 4: {
                if (unlikely(!bytes_write_ucs4(writer_addr, src_voidp, len, true))) return false;
                break;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
    }
    u8 *writer = *writer_addr;
    *writer++ = '"';
    *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
    *writer++ = ' ';
    *writer = 0;
#endif // COMPILE_INDENT_LEVEL > 0
    *writer_addr = writer;
    return true;
}

force_inline bool bytes_buffer_append_key(PyObject *key, u8 **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_write_cache) {
    int src_pykind = PyUnicode_KIND(key);
    bool is_ascii = PyUnicode_IS_ASCII(key);
    usize len = PyUnicode_GET_LENGTH(key);
    const void *src_voidp = is_ascii ? PYUNICODE_ASCII_START(key) : PYUNICODE_UCS1_START(key);
    // write_unicode_indent and '"' writes `get_indent_char_count() + 1` bytes
    // max_json_bytes_per_unicode * len is the written bytes when every character needs to be escaped
    // excess `16 - max_json_bytes_per_unicode` bytes written in bytes_write_utf8 or bytes_write_ascii (see comments in AVX2 impl of encode_unicode_impl)
    // for ucs1,2,4: see AVX2 __excess_bytes_write_ucs2_trailing as an example
    // when indent level > 0, more 4 unicodes are written, else 2 unicodes
    const usize excess_bytes_before = get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 1;
    const usize reserve_bytes_in_encoding = max_json_bytes_per_unicode * len;
    usize excess_bytes_in_encoding = 16 - max_json_bytes_per_unicode;                                     // ascii
    excess_bytes_in_encoding = SSRJSON_MAX(excess_bytes_in_encoding, __excess_bytes_write_ucs1_trailing); // ucs1
    excess_bytes_in_encoding = SSRJSON_MAX(excess_bytes_in_encoding, __excess_bytes_write_ucs2_trailing); // ucs2
    excess_bytes_in_encoding = SSRJSON_MAX(excess_bytes_in_encoding, __excess_bytes_write_ucs4_trailing); // ucs4
    assert(excess_bytes_in_encoding >= 4);
    const usize excess_bytes_after = excess_bytes_in_encoding;
    RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, excess_bytes_before + reserve_bytes_in_encoding + excess_bytes_after));
    u8 *writer = *writer_addr;
    write_unicode_indent(&writer, cur_nested_depth);
    *writer++ = '"';
    if (likely(is_ascii)) {
        bytes_write_ascii(&writer, src_voidp, len, true);
        *writer++ = '"';
        *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
        *writer++ = ' ';
        *writer = 0;
#endif // COMPILE_INDENT_LEVEL > 0
        *writer_addr = writer;
        return true;
    } else {
        *writer_addr = writer;
        if (is_write_cache) {
            return bytes_buffer_append_nonascii_key_write_cache(writer_addr, src_pykind, src_voidp, len, key);
        }
        return bytes_buffer_append_nonascii_key_no_write_cache(writer_addr, src_pykind, src_voidp, len, key);
    }
}

force_inline bool bytes_buffer_append_str(PyObject *str,
                                          u8 **writer_addr,
                                          EncodeUnicodeBufferInfo *unicode_buffer_info,
                                          Py_ssize_t cur_nested_depth,
                                          bool is_in_obj,
                                          bool is_write_cache) {
    int src_pykind = PyUnicode_KIND(str);
    bool is_ascii = PyUnicode_IS_ASCII(str);
    usize len = PyUnicode_GET_LENGTH(str);
    const void *src_voidp = is_ascii ? PYUNICODE_ASCII_START(str) : PYUNICODE_UCS1_START(str);
    //
    u8 *writer;
    //
    const usize reserve_bytes_in_encoding = max_json_bytes_per_unicode * len;
    usize excess_bytes_in_encoding = 16 - max_json_bytes_per_unicode;                                     // ascii
    excess_bytes_in_encoding = SSRJSON_MAX(excess_bytes_in_encoding, __excess_bytes_write_ucs1_trailing); // ucs1
    excess_bytes_in_encoding = SSRJSON_MAX(excess_bytes_in_encoding, __excess_bytes_write_ucs2_trailing); // ucs2
    excess_bytes_in_encoding = SSRJSON_MAX(excess_bytes_in_encoding, __excess_bytes_write_ucs4_trailing); // ucs4
    assert(excess_bytes_in_encoding >= 4);
    const usize excess_bytes_after = excess_bytes_in_encoding;
    if (is_in_obj) {
        // '"' writes 1 byte
        // max_json_bytes_per_unicode * len is the written bytes when every character needs to be escaped
        // excess `16 - max_json_bytes_per_unicode` bytes written in bytes_write_utf8 or bytes_write_ascii (see comments in AVX2 impl of encode_unicode_impl)
        // for ucs1,2,4: see AVX2 __excess_bytes_write_ucs2_trailing as an example
        // '"' and ',': 2 bytes
        const usize excess_bytes_before = 1;
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, excess_bytes_before + reserve_bytes_in_encoding + excess_bytes_after));
        writer = *writer_addr;
    } else {
        // write_unicode_indent and '"' writes `get_indent_char_count() + 1` bytes
        // max_json_bytes_per_unicode * len is the written bytes when every character needs to be escaped
        // excess `16 - max_json_bytes_per_unicode` bytes written in bytes_write_utf8 or bytes_write_ascii (see comments in AVX2 impl of encode_unicode_impl)
        // for ucs1,2,4: see AVX2 __excess_bytes_write_ucs2_trailing as an example
        // '"' and ',': 2 bytes
        const usize excess_bytes_before = get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 1;
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, excess_bytes_before + reserve_bytes_in_encoding + excess_bytes_after));
        writer = *writer_addr;
        write_unicode_indent(&writer, cur_nested_depth);
    }
    *writer++ = '"';
    if (likely(is_ascii)) {
        bytes_write_ascii_not_key(&writer, src_voidp, len);
        *writer++ = '"';
        *writer++ = ',';
        *writer_addr = writer;
        return true;
    } else {
        *writer_addr = writer;
        if (is_write_cache) {
            return bytes_buffer_append_nonascii_str_write_cache(writer_addr, src_pykind, src_voidp, len, str);
        }
        return bytes_buffer_append_nonascii_str_no_write_cache(writer_addr, src_pykind, src_voidp, len, str);
    }
}

force_inline EncodeValJumpFlag encode_bytes_process_val(
        u8 **writer_addr,
        EncodeUnicodeBufferInfo *unicode_buffer_info, PyObject *val,
        PyObject **cur_obj_addr,
        Py_ssize_t *cur_pos_addr,
        Py_ssize_t *cur_nested_depth_addr,
        Py_ssize_t *cur_list_size_addr,
        EncodeCtnWithIndex *ctn_stack,
        // EncodeUnicodeInfo *unicode_info_addr,
        bool is_in_obj, bool is_in_tuple, bool is_write_cache) {
#define CTN_SIZE_GROW()                                                         \
    do {                                                                        \
        if (unlikely(*cur_nested_depth_addr == SSRJSON_ENCODE_MAX_RECURSION)) { \
            PyErr_SetString(JSONEncodeError, "Too many nested structures");     \
            return JumpFlag_Fail;                                               \
        }                                                                       \
    } while (0)
#define RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(_cond_) \
    do {                                         \
        if (unlikely(_cond_)) {                  \
            return JumpFlag_Fail;                \
        }                                        \
    } while (0)

    ssrjson_py_types obj_type = ssrjson_type_check(val);

    switch (obj_type) {
        case T_Unicode: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!bytes_buffer_append_str(val, writer_addr, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, is_write_cache));
            break;
        }
        case T_Long: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_long(&_CAST_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, val, is_in_obj));
            break;
        }
        case T_Bool: {
            if (val == Py_False) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_false(&_CAST_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            } else {
                assert(val == Py_True);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_true(&_CAST_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            }
            break;
        }
        case T_None: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_null(&_CAST_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            break;
        }
        case T_Float: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_float(&_CAST_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, val, is_in_obj));
            break;
        }
        case T_List: {
            Py_ssize_t this_list_size = PyList_GET_SIZE(val);
            if (unlikely(this_list_size == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_empty_arr(&_CAST_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_arr_begin(&_CAST_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = ctn_stack + ((*cur_nested_depth_addr)++);
                cur_write_ctn->ctn = *cur_obj_addr;
                set_index_and_type(cur_write_ctn, *cur_pos_addr, get_encode_ctn_type(is_in_obj, is_in_tuple));
                *cur_obj_addr = val;
                *cur_pos_addr = 0;
                *cur_list_size_addr = this_list_size;
                return JumpFlag_ArrValBegin;
            }
            break;
        }
        case T_Dict: {
            if (unlikely(PyDict_GET_SIZE(val) == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_empty_obj(&_CAST_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_obj_begin(&_CAST_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = ctn_stack + ((*cur_nested_depth_addr)++);
                cur_write_ctn->ctn = *cur_obj_addr;
                set_index_and_type(cur_write_ctn, *cur_pos_addr, get_encode_ctn_type(is_in_obj, is_in_tuple));
                *cur_obj_addr = val;
                *cur_pos_addr = 0;
                return JumpFlag_DictPairBegin;
            }
            break;
        }
        case T_Tuple: {
            Py_ssize_t this_list_size = PyTuple_Size(val);
            if (unlikely(this_list_size == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_empty_arr(&_CAST_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_arr_begin(&_CAST_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = ctn_stack + ((*cur_nested_depth_addr)++);
                cur_write_ctn->ctn = *cur_obj_addr;
                set_index_and_type(cur_write_ctn, *cur_pos_addr, get_encode_ctn_type(is_in_obj, is_in_tuple));
                *cur_obj_addr = val;
                *cur_pos_addr = 0;
                *cur_list_size_addr = this_list_size;
                return JumpFlag_TupleValBegin;
            }
            break;
        }
        default: {
            PyErr_SetString(JSONEncodeError, "Unsupported type to encode");
            return JumpFlag_Fail;
        }
    }

    return JumpFlag_Default;
#undef RETURN_JUMP_FAIL_ON_UNLIKELY_ERR
#undef CTN_SIZE_GROW
}

internal_simd_noinline PyObject *
ssrjson_dumps_to_bytes_obj(PyObject *in_obj, int is_write_cache) {
#define GOTO_FAIL_ON_UNLIKELY_ERR(_condition) \
    do {                                      \
        if (unlikely(_condition)) {           \
            goto fail;                        \
        }                                     \
    } while (0)

    u8 *writer;
    EncodeUnicodeBufferInfo _unicode_buffer_info;
    PyObject *key, *val;
    PyObject *cur_obj = in_obj;
    Py_ssize_t cur_pos = 0;
    Py_ssize_t cur_nested_depth = 0;
    Py_ssize_t cur_list_size;
    // alias thread local buffer
    EncodeCtnWithIndex *ctn_stack;
    bool cur_is_tuple;
    //
    GOTO_FAIL_ON_UNLIKELY_ERR(!init_bytes_buffer(&writer, &_unicode_buffer_info) || !init_encode_ctn_stack(&ctn_stack));

    // this is the starting, we don't need an indent before container.
    // so is_in_obj always pass true
    if (PyDict_Check(cur_obj)) {
        if (unlikely(PyDict_GET_SIZE(cur_obj) == 0)) {
            bool _c = unicode_buffer_append_empty_obj(&_CAST_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = unicode_buffer_append_obj_begin(&_CAST_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
        }
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        goto dict_pair_begin;
    } else if (PyList_Check(cur_obj)) {
        cur_list_size = PyList_GET_SIZE(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            bool _c = unicode_buffer_append_empty_arr(&_CAST_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = unicode_buffer_append_arr_begin(&_CAST_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
        }
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        cur_is_tuple = false;
        goto arr_val_begin;
    } else {
        if (unlikely(!PyTuple_Check(cur_obj))) {
            goto fail_ctntype;
        }
        cur_list_size = PyTuple_GET_SIZE(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            bool _c = unicode_buffer_append_empty_arr(&_CAST_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = unicode_buffer_append_arr_begin(&_CAST_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
        }
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        cur_is_tuple = true;
        goto arr_val_begin;
    }
    // ---unreachable here---
dict_pair_begin:;
    assert(PyDict_GET_SIZE(cur_obj) != 0);
    if (pydict_next(cur_obj, &cur_pos, &key, &val)) {
        if (unlikely(!PyUnicode_CheckExact(key))) {
            goto fail_keytype;
        }
        GOTO_FAIL_ON_UNLIKELY_ERR(!bytes_buffer_append_key(key, &writer, &_unicode_buffer_info, cur_nested_depth, is_write_cache));
    dict_key_done:;
        //
        EncodeValJumpFlag jump_flag = encode_bytes_process_val(&writer, &_unicode_buffer_info, val, &cur_obj, &cur_pos, &cur_nested_depth, &cur_list_size, ctn_stack, true, false, is_write_cache);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                break;
            }
            case JumpFlag_ArrValBegin: {
                cur_is_tuple = false;
                goto arr_val_begin;
            }
            case JumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case JumpFlag_TupleValBegin: {
                cur_is_tuple = true;
                goto arr_val_begin;
            }
            case JumpFlag_Fail: {
                goto fail;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
        goto dict_pair_begin;
    } else {
        // dict end
        assert(cur_nested_depth);
        EncodeCtnWithIndex *last_pos = ctn_stack + (--cur_nested_depth);

        GOTO_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_obj_end(&_CAST_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth));
        if (unlikely(cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        cur_obj = last_pos->ctn;
        EncodeContainerType ctn_type;
        extract_index_and_type(last_pos, &cur_pos, &ctn_type);

        switch (ctn_type) {
            case EncodeContainerType_Dict: {
                goto dict_pair_begin;
            }
            case EncodeContainerType_List: {
                cur_list_size = PyList_GET_SIZE(cur_obj);
                cur_is_tuple = false;
                goto arr_val_begin;
            }
            case EncodeContainerType_Tuple: {
                cur_list_size = PyTuple_GET_SIZE(cur_obj);
                cur_is_tuple = true;
                goto arr_val_begin;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
    }
    // ---unreachable here---
arr_val_begin:;
    assert(cur_list_size != 0);

    if (cur_pos < cur_list_size) {
        if (likely(!cur_is_tuple)) {
            val = PyList_GET_ITEM(cur_obj, cur_pos);
        } else {
            val = PyTuple_GET_ITEM(cur_obj, cur_pos);
        }
        cur_pos++;
        //
        EncodeValJumpFlag jump_flag = encode_bytes_process_val(&writer, &_unicode_buffer_info, val, &cur_obj, &cur_pos, &cur_nested_depth, &cur_list_size, ctn_stack, false, cur_is_tuple, is_write_cache);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                break;
            }
            case JumpFlag_ArrValBegin: {
                cur_is_tuple = false;
                goto arr_val_begin;
            }
            case JumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case JumpFlag_TupleValBegin: {
                cur_is_tuple = true;
                goto arr_val_begin;
            }
            case JumpFlag_Fail: {
                goto fail;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
        //
        goto arr_val_begin;
    } else {
        // list end
        assert(cur_nested_depth);
        EncodeCtnWithIndex *last_pos = ctn_stack + (--cur_nested_depth);

        GOTO_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_arr_end(&_CAST_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth));
        if (unlikely(cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        cur_obj = last_pos->ctn;
        EncodeContainerType ctn_type;
        extract_index_and_type(last_pos, &cur_pos, &ctn_type);

        switch (ctn_type) {
            case EncodeContainerType_Dict: {
                goto dict_pair_begin;
            }
            case EncodeContainerType_List: {
                cur_list_size = PyList_GET_SIZE(cur_obj);
                cur_is_tuple = false;
                goto arr_val_begin;
            }
            case EncodeContainerType_Tuple: {
                cur_list_size = PyTuple_GET_SIZE(cur_obj);
                cur_is_tuple = true;
                goto arr_val_begin;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
    }
    // ---unreachable here---
success:;
    assert(cur_nested_depth == 0);
    // remove trailing comma
    (_CAST_WRITER(&writer))--;
    usize final_len = get_bytes_buffer_final_len(writer, _unicode_buffer_info.head);
    GOTO_FAIL_ON_UNLIKELY_ERR(!resize_to_fit_pybytes(&_unicode_buffer_info, final_len));
    init_pybytes(_unicode_buffer_info.head, final_len);
    return (PyObject *)_unicode_buffer_info.head;
fail:;
    if (_unicode_buffer_info.head) {
        PyObject_Free(_unicode_buffer_info.head);
    }
    return NULL;
fail_ctntype:;
    PyErr_SetString(JSONEncodeError, "Unsupported type to encode");
    goto fail;
fail_keytype:;
    PyErr_SetString(JSONEncodeError, "Expected `str` as key");
    goto fail;
#undef GOTO_FAIL_ON_UNLIKELY_ERR
}

#include "compile_context/sirw_out.inl.h"
#undef COMPILE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_SIMD_BITS

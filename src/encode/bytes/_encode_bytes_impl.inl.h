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

static force_noinline u8 *bytes_buffer_append_nonascii_key_write_cache(u8 *writer, int src_pykind, const void *src_voidp, usize len, PyObject *key) {
    assert(ssrjson_pyascii_cast(key)->state.compact);
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
        if (unlikely(!write_cache_impl(src_voidp, src_pykind, len, &utf8_cache, &utf8_length, true))) return NULL;
        set_cache(key, &utf8_cache, utf8_length);
    }
    assert(utf8_cache);
    // Also see comment in bytes_write_utf8
    if (USING_AVX512 || utf8_length >= 16) {
        writer = bytes_write_utf8(writer, utf8_cache, utf8_length, true);
        *writer++ = '"';
        *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
        *writer++ = ' ';
        *writer = 0;
#endif // COMPILE_INDENT_LEVEL > 0
        return writer;
    } else {
    no_cache_encode:;
        switch (src_pykind) {
            case 1: {
                writer = bytes_write_ucs1(writer, src_voidp, len, true);
                // bytes_write_ucs1 does not fail
                break;
            }
            case 2: {
                writer = bytes_write_ucs2(writer, src_voidp, len, true);
                if (unlikely(!writer)) return NULL;
                break;
            }
            case 4: {
                writer = bytes_write_ucs4(writer, src_voidp, len, true);
                if (unlikely(!writer)) return NULL;
                break;
            }
            default: {
                ssrjson_unreachable();
            }
        }
        *writer++ = '"';
        *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
        *writer++ = ' ';
        *writer = 0;
#endif // COMPILE_INDENT_LEVEL > 0
        return writer;
    }
}

static force_noinline u8 *bytes_buffer_append_nonascii_key_no_write_cache(u8 *writer, int src_pykind, const void *src_voidp, usize len, PyObject *str) {
    assert(ssrjson_pyascii_cast(str)->state.compact);
    const u8 *utf8_cache;
    usize utf8_length;
    get_utf8_cache(str, &utf8_cache, &utf8_length);
    // Also see comment in bytes_write_utf8
    if (utf8_cache && (USING_AVX512 || utf8_length >= 16)) {
        writer = bytes_write_utf8(writer, utf8_cache, utf8_length, true);
    } else {
        switch (src_pykind) {
            case 1: {
                writer = bytes_write_ucs1(writer, src_voidp, len, true);
                // bytes_write_ucs1 does not fail
                break;
            }
            case 2: {
                writer = bytes_write_ucs2(writer, src_voidp, len, true);
                if (unlikely(!writer)) return NULL;
                break;
            }
            case 4: {
                writer = bytes_write_ucs4(writer, src_voidp, len, true);
                if (unlikely(!writer)) return NULL;
                break;
            }
            default: {
                ssrjson_unreachable();
            }
        }
    }
    *writer++ = '"';
    *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
    *writer++ = ' ';
    *writer = 0;
#endif // COMPILE_INDENT_LEVEL > 0
    return writer;
}

static force_noinline u8 *bytes_buffer_append_key(u8 *writer, PyObject *key,
                                                  EncodeUnicodeBufferInfo *unicode_buffer_info,
                                                  Py_ssize_t cur_nested_depth, bool is_write_cache) {
    int src_pykind = PyUnicode_KIND(key);
    bool is_ascii = PyUnicode_IS_ASCII(key);
    usize len = PyUnicode_GET_LENGTH(key);
    const void *src_voidp = is_ascii ? ssrjson_pyunicode_ascii_start(key) : ssrjson_pyunicode_ucs1_start(key);
    // write_unicode_indent and '"' writes `get_indent_char_count() + 1` bytes
    // max_json_bytes_per_unicode * len is the written bytes when every character needs to be escaped
    // excess `16 - max_json_bytes_per_unicode` bytes written in bytes_write_utf8 or bytes_write_ascii (see comments in AVX2 impl of encode_unicode_impl)
    // for ucs1,2,4: see AVX2 __excess_bytes_write_ucs2_trailing as an example
    // when indent level > 0, more 4 unicodes are written, else 2 unicodes
    const usize excess_bytes_before = get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 1;
    const usize reserve_bytes_in_encoding = max_json_bytes_per_unicode * len;
    usize excess_bytes_in_encoding = 16 - max_json_bytes_per_unicode;                                     // ascii
    excess_bytes_in_encoding = ssrjson_max(excess_bytes_in_encoding, __excess_bytes_write_ucs1_trailing); // ucs1
    excess_bytes_in_encoding = ssrjson_max(excess_bytes_in_encoding, __excess_bytes_write_ucs2_trailing); // ucs2
    excess_bytes_in_encoding = ssrjson_max(excess_bytes_in_encoding, __excess_bytes_write_ucs4_trailing); // ucs4
    assert(excess_bytes_in_encoding >= 4);
    const usize excess_bytes_after = excess_bytes_in_encoding;
    writer = unicode_buffer_reserve(writer, unicode_buffer_info, excess_bytes_before + reserve_bytes_in_encoding + excess_bytes_after);
    return_if_unlikely(!writer);
    writer = write_unicode_indent(writer, cur_nested_depth);
    *writer++ = '"';
    if (likely(is_ascii)) {
        writer = bytes_write_ascii(writer, src_voidp, len, true);
        *writer++ = '"';
        *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
        *writer++ = ' ';
        *writer = 0;
#endif // COMPILE_INDENT_LEVEL > 0
        return writer;
    } else {
        if (is_write_cache) {
            return bytes_buffer_append_nonascii_key_write_cache(writer, src_pykind, src_voidp, len, key);
        } else {
            return bytes_buffer_append_nonascii_key_no_write_cache(writer, src_pykind, src_voidp, len, key);
        }
    }
}

static force_noinline u8 *bytes_buffer_append_str(u8 *writer, PyObject *str,
                                                  EncodeUnicodeBufferInfo *unicode_buffer_info,
                                                  Py_ssize_t cur_nested_depth,
                                                  ssrjson_compiletime bool is_in_obj,
                                                  bool is_write_cache) {
    int src_pykind = PyUnicode_KIND(str);
    bool is_ascii = PyUnicode_IS_ASCII(str);
    usize len = PyUnicode_GET_LENGTH(str);
    const void *src_voidp = is_ascii ? ssrjson_pyunicode_ascii_start(str) : ssrjson_pyunicode_ucs1_start(str);
    //
    const usize reserve_bytes_in_encoding = max_json_bytes_per_unicode * len;
    usize excess_bytes_in_encoding = 16 - max_json_bytes_per_unicode;                                     // ascii
    excess_bytes_in_encoding = ssrjson_max(excess_bytes_in_encoding, __excess_bytes_write_ucs1_trailing); // ucs1
    excess_bytes_in_encoding = ssrjson_max(excess_bytes_in_encoding, __excess_bytes_write_ucs2_trailing); // ucs2
    excess_bytes_in_encoding = ssrjson_max(excess_bytes_in_encoding, __excess_bytes_write_ucs4_trailing); // ucs4
    assert(excess_bytes_in_encoding >= 4);
    const usize excess_bytes_after = excess_bytes_in_encoding;
    if (ssrjson_consteval(is_in_obj)) {
        // '"' writes 1 byte
        // max_json_bytes_per_unicode * len is the written bytes when every character needs to be escaped
        // excess `16 - max_json_bytes_per_unicode` bytes written in bytes_write_utf8 or bytes_write_ascii (see comments in AVX2 impl of encode_unicode_impl)
        // for ucs1,2,4: see AVX2 __excess_bytes_write_ucs2_trailing as an example
        // '"' and ',': 2 bytes
        const usize excess_bytes_before = 1;
        writer = unicode_buffer_reserve(writer, unicode_buffer_info, excess_bytes_before + reserve_bytes_in_encoding + excess_bytes_after);
        return_if_unlikely(!writer);
    } else {
        // write_unicode_indent and '"' writes `get_indent_char_count() + 1` bytes
        // max_json_bytes_per_unicode * len is the written bytes when every character needs to be escaped
        // excess `16 - max_json_bytes_per_unicode` bytes written in bytes_write_utf8 or bytes_write_ascii (see comments in AVX2 impl of encode_unicode_impl)
        // for ucs1,2,4: see AVX2 __excess_bytes_write_ucs2_trailing as an example
        // '"' and ',': 2 bytes
        const usize excess_bytes_before = get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 1;
        writer = unicode_buffer_reserve(writer, unicode_buffer_info, excess_bytes_before + reserve_bytes_in_encoding + excess_bytes_after);
        return_if_unlikely(!writer);
        writer = write_unicode_indent(writer, cur_nested_depth);
    }
    *writer++ = '"';
    if (likely(is_ascii)) {
        writer = bytes_write_ascii(writer, src_voidp, len, false);
        *writer++ = '"';
        *writer++ = ',';
        return writer;
    } else {
        // tail-call noinline functions
        if (is_write_cache) {
            return bytes_buffer_append_nonascii_str_write_cache(writer, src_pykind, src_voidp, len, str);
        } else {
            return bytes_buffer_append_nonascii_str_no_write_cache(writer, src_pykind, src_voidp, len, str);
        }
    }
}

force_inline u8 *encode_bytes_process_val(
        u8 *writer,
        EncodeValJumpFlag *jump_flag_out,
        EncodeUnicodeBufferInfo *unicode_buffer_info, PyObject *val,
        PyObject **cur_obj_addr,
        Py_ssize_t *cur_pos_addr,
        Py_ssize_t *cur_nested_depth_addr,
        Py_ssize_t *cur_list_size_addr,
        EncodeCtnWithIndex *ctn_stack,
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
        khash_t(ptr_set) * pyobj_set,
#endif
        ssrjson_compiletime bool is_in_obj,
        bool is_in_tuple,
        bool is_write_cache) {
#define ctn_size_grow()                                                         \
    do {                                                                        \
        if (unlikely(*cur_nested_depth_addr == SSRJSON_ENCODE_MAX_RECURSION)) { \
            PyErr_SetString(JSONEncodeError, "Too many nested structures");     \
            *jump_flag_out = JumpFlag_Fail;                                     \
            return NULL;                                                        \
        }                                                                       \
    } while (0)
#define return_jump_fail_if_unlikely(_cond_) \
    do {                                     \
        if (unlikely((_cond_))) {            \
            *jump_flag_out = JumpFlag_Fail;  \
            return NULL;                     \
        }                                    \
    } while (0)

    EncodePyTypes obj_type = ssrjson_type_check(val);

    switch (obj_type) {
        case T_Unicode: {
            writer = bytes_buffer_append_str(writer, val, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, is_write_cache);
            return_jump_fail_if_unlikely(!writer);
            break;
        }
        t_long_zero:;
            writer = unicode_indent_writer(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, 2);
            return_jump_fail_if_unlikely(!writer);
            *writer++ = '0';
            *writer++ = ',';
            break;

            {
                u64 value;
                int sign;

                case T_Long: {
                    return_jump_fail_if_unlikely(!pylong_to_clong(val, &value, &sign));
                    if (ssrjson_consteval(sign == -1)) goto t_long_zero;
                t_long_nonzero:;
                    writer = unicode_indent_writer(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, 32);
                    return_jump_fail_if_unlikely(!writer);
                    writer = u64_to_unicode_u8(writer, value, sign);
                    *writer++ = ',';
                    break;
                }

                case T_NumpyInt64: {
                    i64 v = pyobj_scalar_value(i64, val);
                    if (v == 0) goto t_long_zero;
                    sign = v < 0;
                    value = sign ? (u64)(-(i64)v) : (u64)v;
                    goto t_long_nonzero;
                }
                case T_NumpyUint64: {
                    u64 v = pyobj_scalar_value(u64, val);
                    if (v == 0) goto t_long_zero;
                    value = v;
                    sign = 0;
                    goto t_long_nonzero;
                }
            }

        case T_Bool: {
            const bool is_false = (val == Py_False);
            writer = unicode_buffer_append_bool(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, is_false);
            return_jump_fail_if_unlikely(!writer);
            break;
        }
        case T_None: {
            writer = unicode_buffer_append_null(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
            return_jump_fail_if_unlikely(!writer);
            break;
        }
        case T_Float: {
            double v = PyFloat_AS_DOUBLE(val);
            writer = unicode_buffer_append_float(writer, unicode_buffer_info, *cur_nested_depth_addr, v, is_in_obj);
            return_jump_fail_if_unlikely(!writer);
            break;
        }
        case T_List: {
            Py_ssize_t this_list_size = PyList_GET_SIZE(val);
            if (unlikely(this_list_size == 0)) {
                writer = unicode_buffer_append_empty_arr(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
                return_jump_fail_if_unlikely(!writer);
            } else {
                writer = unicode_buffer_append_arr_begin(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
                return_jump_fail_if_unlikely(!writer);
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
                {
                    int ret;
                    kh_put(ptr_set, pyobj_set, (u64)val, &ret);
                    if (unlikely(ret == 0)) {
                        PyErr_SetString(JSONEncodeError, "Circular reference detected");
                        *jump_flag_out = JumpFlag_Fail;
                        return NULL;
                    }
                    PyMutex_Lock(&ssrjson_pyobj_cast(val)->ob_mutex);
                }
#endif
                ctn_size_grow();
                EncodeCtnWithIndex *cur_write_ctn = ctn_stack + ((*cur_nested_depth_addr)++);
                cur_write_ctn->ctn = *cur_obj_addr;
                set_index_and_type(cur_write_ctn, *cur_pos_addr, get_encode_ctn_type(is_in_obj, is_in_tuple));
                *cur_obj_addr = val;
                *cur_pos_addr = 0;
                *cur_list_size_addr = this_list_size;
                *jump_flag_out = JumpFlag_ArrValBegin;
                return writer;
            }
            break;
        }
        case T_Dict: {
            if (unlikely(PyDict_GET_SIZE(val) == 0)) {
                writer = unicode_buffer_append_empty_obj(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
                return_jump_fail_if_unlikely(!writer);
            } else {
                writer = unicode_buffer_append_obj_begin(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
                return_jump_fail_if_unlikely(!writer);
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
                {
                    int ret;
                    kh_put(ptr_set, pyobj_set, (u64)val, &ret);
                    if (unlikely(ret == 0)) {
                        PyErr_SetString(JSONEncodeError, "Circular reference detected");
                        *jump_flag_out = JumpFlag_Fail;
                        return NULL;
                    }
                    PyMutex_Lock(&ssrjson_pyobj_cast(val)->ob_mutex);
                }
#endif
                ctn_size_grow();
                EncodeCtnWithIndex *cur_write_ctn = ctn_stack + ((*cur_nested_depth_addr)++);
                cur_write_ctn->ctn = *cur_obj_addr;
                set_index_and_type(cur_write_ctn, *cur_pos_addr, get_encode_ctn_type(is_in_obj, is_in_tuple));
                *cur_obj_addr = val;
                *cur_pos_addr = 0;
                *jump_flag_out = JumpFlag_DictPairBegin;
                return writer;
            }
            break;
        }
        case T_Tuple: {
            Py_ssize_t this_list_size = PyTuple_GET_SIZE(val);
            if (unlikely(this_list_size == 0)) {
                writer = unicode_buffer_append_empty_arr(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
                return_jump_fail_if_unlikely(!writer);
            } else {
                writer = unicode_buffer_append_arr_begin(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
                return_jump_fail_if_unlikely(!writer);
                ctn_size_grow();
                EncodeCtnWithIndex *cur_write_ctn = ctn_stack + ((*cur_nested_depth_addr)++);
                cur_write_ctn->ctn = *cur_obj_addr;
                set_index_and_type(cur_write_ctn, *cur_pos_addr, get_encode_ctn_type(is_in_obj, is_in_tuple));
                *cur_obj_addr = val;
                *cur_pos_addr = 0;
                *cur_list_size_addr = this_list_size;
                *jump_flag_out = JumpFlag_TupleValBegin;
                return writer;
            }
            break;
        }
        case T_NumpyArray: {
            writer = u8_buffer_append_ndarray(writer, unicode_buffer_info, *cur_nested_depth_addr, val, is_in_obj);
            return_jump_fail_if_unlikely(!writer);
            break;
        }

            {
                float v;
                case T_NumpyFloat32: {
                    v = pyobj_scalar_value(float, val);
                t_f32:;
                    writer = unicode_buffer_append_f32(writer, unicode_buffer_info, *cur_nested_depth_addr, v, is_in_obj);
                    return_jump_fail_if_unlikely(!writer);
                    break;
                }
                case T_NumpyFloat16: {
                    v = f16_to_f32(pyobj_scalar_value(u16, val));
                    goto t_f32;
                }
            }
            {
                u32 value;
                int sign;

                case T_NumpyUint32: {
                    u32 v = pyobj_scalar_value(u32, val);
                    if (v == 0) goto t_long_zero;
                    value = (u32)v;
                    sign = 0;
                    goto t_iu32_nonzero;
                }
                case T_NumpyInt32: {
                    i32 v = pyobj_scalar_value(i32, val);
                    if (v == 0) goto t_long_zero;
                    sign = v < 0;
                    value = sign ? (u32)(-(i32)v) : (u32)v;
                    goto t_iu32_nonzero;
                }

                t_iu32_nonzero:;
                    writer = unicode_indent_writer(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, 16);
                    return_jump_fail_if_unlikely(!writer);
                    writer = u32_to_unicode_u8(writer, value, sign);
                    *writer++ = ',';
                    break;
            }

            {
                u16 value;
                int sign;

                case T_NumpyUint16: {
                    u16 v = pyobj_scalar_value(u16, val);
                    if (v == 0) goto t_long_zero;
                    value = (u16)v;
                    sign = 0;
                    goto t_iu16_nonzero;
                }
                case T_NumpyInt16: {
                    i16 v = pyobj_scalar_value(i16, val);
                    if (v == 0) goto t_long_zero;
                    sign = v < 0;
                    value = sign ? (u16)(-(i16)v) : (u16)v;
                    goto t_iu16_nonzero;
                }

                t_iu16_nonzero:;
                    writer = unicode_indent_writer(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, 8);
                    return_jump_fail_if_unlikely(!writer);
                    writer = u16_to_unicode_u8(writer, value, sign);
                    *writer++ = ',';
                    break;
            }

            {
                u8 value;
                int sign;

                case T_NumpyUint8: {
                    u8 v = pyobj_scalar_value(u8, val);
                    if (v == 0) goto t_long_zero;
                    value = (u8)v;
                    sign = 0;
                    goto t_iu8_nonzero;
                }
                case T_NumpyInt8: {
                    i8 v = pyobj_scalar_value(i8, val);
                    if (v == 0) goto t_long_zero;
                    sign = v < 0;
                    value = sign ? (u8)(-(i8)v) : (u8)v;
                    goto t_iu8_nonzero;
                }

                t_iu8_nonzero:;
                    writer = unicode_indent_writer(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, 4);
                    return_jump_fail_if_unlikely(!writer);
                    writer = u8_to_unicode_u8(writer, value, sign);
                    *writer++ = ',';
                    break;
            }

        case T_NumpyBool: {
            bool is_true = pyobj_scalar_value(u8, val);
            const bool is_false = !is_true;
            writer = unicode_buffer_append_bool(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, is_false);
            return_jump_fail_if_unlikely(!writer);
            break;
        }
        default: {
            PyErr_SetString(JSONEncodeError, "Unsupported type to encode");
            *jump_flag_out = JumpFlag_Fail;
            return NULL;
        }
    }

    *jump_flag_out = JumpFlag_Default;
    return writer;
#undef return_jump_fail_if_unlikely
#undef ctn_size_grow
}

internal_simd_noinline PyObject *
ssrjson_dumps_to_bytes_obj(PyObject *in_obj, int is_write_cache) {
#define goto_fail_if_unlikely(_condition) \
    do {                                  \
        if (unlikely((_condition))) {     \
            goto fail;                    \
        }                                 \
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
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
    khash_t(ptr_set) * pyobj_set;
    PyObject *toplevel_locked_obj = NULL;
#endif
    bool cur_is_tuple;
    //
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
    pyobj_set = kh_init(ptr_set);
    if (unlikely(kh_resize(ptr_set, pyobj_set, 512) != 0)) {
        PyErr_NoMemory();
        return NULL;
    }
#endif
    writer = init_bytes_buffer(&_unicode_buffer_info);
    goto_fail_if_unlikely(!writer || !init_encode_ctn_stack(&ctn_stack));

    // this is the starting, we don't need an indent before container.
    // so is_in_obj always pass true
    if (PyDict_Check(cur_obj)) {
        if (unlikely(PyDict_GET_SIZE(cur_obj) == 0)) {
            writer = unicode_buffer_append_empty_obj(writer, &_unicode_buffer_info, 0, true);
            assert(writer);
            goto success;
        }
        writer = unicode_buffer_append_obj_begin(writer, &_unicode_buffer_info, 0, true);
        assert(writer && !cur_nested_depth);
        cur_nested_depth = 1;
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
        {
            toplevel_locked_obj = cur_obj;
            int ret;
            kh_put(ptr_set, pyobj_set, (u64)cur_obj, &ret);
            assert(ret == 1);
            PyMutex_Lock(&ssrjson_pyobj_cast(cur_obj)->ob_mutex);
        }
#endif
        // NOTE: ctn_stack[0] is always invalid
        goto dict_pair_begin;
    } else if (PyList_Check(cur_obj)) {
        cur_list_size = PyList_GET_SIZE(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            writer = unicode_buffer_append_empty_arr(writer, &_unicode_buffer_info, 0, true);
            assert(writer);
            goto success;
        }
        writer = unicode_buffer_append_arr_begin(writer, &_unicode_buffer_info, 0, true);
        assert(writer && !cur_nested_depth);
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
        {
            toplevel_locked_obj = cur_obj;
            int ret;
            kh_put(ptr_set, pyobj_set, (u64)cur_obj, &ret);
            assert(ret == 1);
            PyMutex_Lock(&ssrjson_pyobj_cast(cur_obj)->ob_mutex);
        }
#endif
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
            writer = unicode_buffer_append_empty_arr(writer, &_unicode_buffer_info, 0, true);
            assert(writer);
            goto success;
        }
        writer = unicode_buffer_append_arr_begin(writer, &_unicode_buffer_info, 0, true);
        assert(writer && !cur_nested_depth);
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
        writer = bytes_buffer_append_key(writer, key, &_unicode_buffer_info, cur_nested_depth, is_write_cache);
        goto_fail_if_unlikely(!writer);
    dict_key_done:;
        //
        EncodeValJumpFlag jump_flag;
        writer = encode_bytes_process_val(writer, &jump_flag, &_unicode_buffer_info, val, &cur_obj, &cur_pos, &cur_nested_depth, &cur_list_size, ctn_stack,
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
                                          pyobj_set,
#endif
                                          true, false, is_write_cache);
        switch (ssrjson_consteval(jump_flag)) {
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
                ssrjson_unreachable();
            }
        }
        goto dict_pair_begin;
    } else {
        // dict end
        assert(cur_nested_depth);
        EncodeCtnWithIndex *last_pos = ctn_stack + (--cur_nested_depth);

        writer = unicode_buffer_append_obj_end(writer, &_unicode_buffer_info, cur_nested_depth);
        goto_fail_if_unlikely(!writer);
        if (cur_nested_depth == 0) {
            goto success;
        }

#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
        {
            khiter_t k = kh_get(ptr_set, pyobj_set, (u64)cur_obj);
            assert(k != kh_end(pyobj_set));
            kh_del(ptr_set, pyobj_set, k);
            PyMutex_Unlock(&ssrjson_pyobj_cast(cur_obj)->ob_mutex);
        }
#endif

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
                ssrjson_unreachable();
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
        EncodeValJumpFlag jump_flag;
        writer = encode_bytes_process_val(writer, &jump_flag, &_unicode_buffer_info, val, &cur_obj, &cur_pos, &cur_nested_depth, &cur_list_size, ctn_stack,
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
                                          pyobj_set,
#endif
                                          false, cur_is_tuple, is_write_cache);
        switch (ssrjson_consteval(jump_flag)) {
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
                ssrjson_unreachable();
            }
        }
        //
        goto arr_val_begin;
    } else {
        // list end
        assert(cur_nested_depth);
        EncodeCtnWithIndex *last_pos = ctn_stack + (--cur_nested_depth);

        writer = unicode_buffer_append_arr_end(writer, &_unicode_buffer_info, cur_nested_depth);
        goto_fail_if_unlikely(!writer);
        if (cur_nested_depth == 0) {
            goto success;
        }

        /* pop: unlock current child before restoring parent (use cur_is_tuple) */
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
        if (!cur_is_tuple) {
            khiter_t k = kh_get(ptr_set, pyobj_set, (u64)cur_obj);
            assert(k != kh_end(pyobj_set));
            kh_del(ptr_set, pyobj_set, k);
            PyMutex_Unlock(&ssrjson_pyobj_cast(cur_obj)->ob_mutex);
        }
#endif

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
                ssrjson_unreachable();
            }
        }
    }
    // ---unreachable here---
success:;
    assert(cur_nested_depth == 0);
    // remove trailing comma
    writer--;
    usize final_len = get_bytes_buffer_final_len(writer, _unicode_buffer_info.head);
    goto_fail_if_unlikely(!resize_to_fit_pybytes(&_unicode_buffer_info, final_len));
    init_pybytes(_unicode_buffer_info.head, final_len);
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
    {
#    ifndef NDEBUG
        int size = kh_size(pyobj_set);
        assert(size == (toplevel_locked_obj ? 1 : 0));
#    endif
        if (likely(toplevel_locked_obj)) {
            PyMutex_Unlock(&ssrjson_pyobj_cast(toplevel_locked_obj)->ob_mutex);
        }
        kh_destroy(ptr_set, pyobj_set);
    }
#endif
    return (PyObject *)_unicode_buffer_info.head;
fail:;
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
    /* unwind: unlock all pyobj in pyobj_set*/

    for (khiter_t k = kh_begin(pyobj_set); k != kh_end(pyobj_set); ++k) {
        if (kh_exist(pyobj_set, k)) {
            PyObject *obj = (PyObject *)kh_key(pyobj_set, k);
            PyMutex_Unlock(&ssrjson_pyobj_cast(obj)->ob_mutex);
        }
    }
    kh_destroy(ptr_set, pyobj_set);
#endif
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
#undef goto_fail_if_unlikely
}

#include "compile_context/sirw_out.inl.h"
#undef COMPILE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL
#undef _CompileVectorBits

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
#        include "encode_shared.h"
#        include "encode_unicode_impl_wrap.h"
#        include "encode_utils_impl_wrap.h"
#        include "states.h"
#        include "tls.h"
#        define COMPILE_UCS_LEVEL 0
#        define COMPILE_INDENT_LEVEL 0
#        include "simd/compile_feature_check.h"
#    endif
#endif

#ifndef COMPILE_UCS_LEVEL
#    error "COMPILE_UCS_LEVEL is not defined"
#endif
#ifndef COMPILE_INDENT_LEVEL
#    error "COMPILE_INDENT_LEVEL is not defined"
#endif

#if COMPILE_UCS_LEVEL <= 1
#    define COMPILE_READ_UCS_LEVEL 1
#    define COMPILE_WRITE_UCS_LEVEL 1
#else
#    define COMPILE_READ_UCS_LEVEL COMPILE_UCS_LEVEL
#    define COMPILE_WRITE_UCS_LEVEL COMPILE_UCS_LEVEL
#endif
//
#include "compile_context/sirw_in.inl.h"


#define write_indent_return_if_fail(_writer_, _unicode_buffer_info_, _cur_nested_depth_, _is_in_obj_, _additional_reserve_count_)           \
    do {                                                                                                                                    \
        (_writer_) = unicode_indent_writer((_writer_), _unicode_buffer_info_, _cur_nested_depth_, _is_in_obj_, _additional_reserve_count_); \
        if (unlikely(!(_writer_))) return NULL;                                                                                             \
    } while (0)

force_inline ssrjson_nofail EncodeUnicodeWriter prepare_unicode_write(PyObject *obj, EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *restrict unicode_info, usize *out_len, unsigned int *read_pykind, unsigned int *write_kind, const void **src_addr) {
    usize out_len_val = (usize)PyUnicode_GET_LENGTH(obj);
    *out_len = out_len_val;
    unsigned int r_kind_val = PyUnicode_KIND(obj);
    *read_pykind = r_kind_val;
    bool is_ascii = PyUnicode_IS_ASCII(obj);
    *src_addr = is_ascii ? ssrjson_pyunicode_ascii_start(obj) : ssrjson_pyunicode_ucs1_start(obj);

    // to eliminate redundant instructions when checking pykind
    assume(r_kind_val == 1 || r_kind_val == 2 || r_kind_val == 4);

#if COMPILE_UCS_LEVEL == 4
    *write_kind = 4;
#elif COMPILE_UCS_LEVEL == 2
    if (likely(r_kind_val & 3)) {
        assume(r_kind_val == 1 || r_kind_val == 2);
        *write_kind = 2;
    } else {
        assume(r_kind_val == 4);
        writer = memorize_ucs2_to_ucs4(writer, unicode_buffer_info, unicode_info);
        *write_kind = 4;
    }
#elif COMPILE_UCS_LEVEL == 1
    if (likely(r_kind_val == 1)) {
        *write_kind = 1;
    } else if (r_kind_val == 2) {
        writer = memorize_ucs1_to_ucs2(writer, unicode_buffer_info, unicode_info);
        *write_kind = 2;
    } else {
        assume(r_kind_val == 4);
        writer = memorize_ucs1_to_ucs4(writer, unicode_buffer_info, unicode_info);
        *write_kind = 4;
    }
#elif COMPILE_UCS_LEVEL == 0
    if (likely(r_kind_val == 1 && is_ascii)) {
        *write_kind = 0;
    } else if (r_kind_val == 2) {
        writer = memorize_ascii_to_ucs2(writer, unicode_buffer_info, unicode_info);
        *write_kind = 2;
    } else if (r_kind_val == 4) {
        writer = memorize_ascii_to_ucs4(writer, unicode_buffer_info, unicode_info);
        *write_kind = 4;
    } else {
        assume(r_kind_val == 1 && !is_ascii);
        writer = memorize_ascii_to_ucs1(writer, unicode_buffer_info, unicode_info);
        *write_kind = 1;
    }
#endif
    return writer;
}

#if COMPILE_UCS_LEVEL < 4
force_inline EncodeUnicodeWriter unicode_buffer_append_key_distribute2(EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, usize len, unsigned int pykind, const void *src_voidp) {
#    if COMPILE_UCS_LEVEL == 2
    if (pykind == 1) {
        const u8 *src = src_voidp;
        return KEY_WRITER_IMPL(u8, u16)(src, len, WRITER_AS_U16(writer), unicode_buffer_info, cur_nested_depth);
    } else {
#    endif
        assert(pykind == 2);
        const u16 *src = src_voidp;
        return KEY_WRITER_IMPL(u16, u16)(src, len, WRITER_AS_U16(writer), unicode_buffer_info, cur_nested_depth);
#    if COMPILE_UCS_LEVEL == 2
    }
#    endif
    return writer;
}
#endif

force_inline EncodeUnicodeWriter unicode_buffer_append_key_distribute4(EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, usize len, unsigned int pykind, const void *src_voidp) {
#if COMPILE_READ_UCS_LEVEL == 4
    if (pykind == 1) {
        const u8 *src = src_voidp;
        return KEY_WRITER_IMPL(u8, u32)(src, len, WRITER_AS_U32(writer), unicode_buffer_info, cur_nested_depth);
    } else if (pykind == 2) {
        const u16 *src = src_voidp;
        return KEY_WRITER_IMPL(u16, u32)(src, len, WRITER_AS_U32(writer), unicode_buffer_info, cur_nested_depth);
    } else {
#endif
        assert(pykind == 4);
        const u32 *src = src_voidp;
        return KEY_WRITER_IMPL(u32, u32)(src, len, WRITER_AS_U32(writer), unicode_buffer_info, cur_nested_depth);
#if COMPILE_READ_UCS_LEVEL == 4
    }
#endif
    return writer;
}

static force_noinline EncodeUnicodeWriter unicode_buffer_append_key(PyObject *key, EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info, Py_ssize_t cur_nested_depth) {
    usize len;
    unsigned int pykind, write_kind;
    const void *src_voidp;
    assert(ssrjson_pyascii_cast(key)->state.compact);
    writer = prepare_unicode_write(key, writer, unicode_buffer_info, unicode_info, &len, &pykind, &write_kind, &src_voidp);

    switch (write_kind) {
#if COMPILE_UCS_LEVEL < 1
        case 0:
#endif
#if COMPILE_UCS_LEVEL < 2
        case 1: {
            const u8 *src = src_voidp;
            return KEY_WRITER_IMPL(u8, u8)(src, len, WRITER_AS_U8(writer), unicode_buffer_info, cur_nested_depth);
        }
#endif
#if COMPILE_UCS_LEVEL < 4
        case 2: {
            return unicode_buffer_append_key_distribute2(writer, unicode_buffer_info, cur_nested_depth, len, pykind, src_voidp);
        }
#endif
        case 4: {
            return unicode_buffer_append_key_distribute4(writer, unicode_buffer_info, cur_nested_depth, len, pykind, src_voidp);
        }
        default: {
            ssrjson_unreachable();
            return NULL;
        }
    }
}

#if COMPILE_UCS_LEVEL < 4
force_inline EncodeUnicodeWriter unicode_buffer_append_str_distribute2(EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, usize len, unsigned int pykind, bool is_in_obj, const void *src_voidp) {
#    if COMPILE_UCS_LEVEL == 2
    if (pykind == 1) {
        const u8 *src = src_voidp;
        return STR_WRITER_IMPL(u8, u16)(src, len, WRITER_AS_U16(writer), unicode_buffer_info, cur_nested_depth, is_in_obj);
    } else {
#    endif
        assert(pykind == 2);
        const u16 *src = src_voidp;
        return STR_WRITER_IMPL(u16, u16)(src, len, WRITER_AS_U16(writer), unicode_buffer_info, cur_nested_depth, is_in_obj);
#    if COMPILE_UCS_LEVEL == 2
    }
#    endif
    return writer;
}
#endif

force_inline EncodeUnicodeWriter unicode_buffer_append_str_distribute4(EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, usize len, unsigned int pykind, bool is_in_obj, const void *src_voidp) {
#if COMPILE_UCS_LEVEL == 4
    if (pykind == 1) {
        const u8 *src = src_voidp;
        return STR_WRITER_IMPL(u8, u32)(src, len, WRITER_AS_U32(writer), unicode_buffer_info, cur_nested_depth, is_in_obj);
    } else if (pykind == 2) {
        const u16 *src = src_voidp;
        return STR_WRITER_IMPL(u16, u32)(src, len, WRITER_AS_U32(writer), unicode_buffer_info, cur_nested_depth, is_in_obj);
    } else {
#endif
        assert(pykind == 4);
        const u32 *src = src_voidp;
        return STR_WRITER_IMPL(u32, u32)(src, len, WRITER_AS_U32(writer), unicode_buffer_info, cur_nested_depth, is_in_obj);
#if COMPILE_UCS_LEVEL == 4
    }
#endif
    return writer;
}

force_inline EncodeUnicodeWriter unicode_buffer_append_str(EncodeUnicodeWriter writer, PyObject *val, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    usize len;
    unsigned int kind, write_kind;
    const void *src_voidp;
    assert(ssrjson_pyascii_cast(val)->state.compact);
    writer = prepare_unicode_write(val, writer, unicode_buffer_info, unicode_info, &len, &kind, &write_kind, &src_voidp);

    switch (write_kind) {
#if COMPILE_UCS_LEVEL < 1
        case 0:
#endif
#if COMPILE_UCS_LEVEL < 2
        case 1: {
            const u8 *src = src_voidp;
            return STR_WRITER_IMPL(u8, u8)(src, len, WRITER_AS_U8(writer), unicode_buffer_info, cur_nested_depth, is_in_obj);
        }
#endif
#if COMPILE_UCS_LEVEL < 4
        case 2: {
            return unicode_buffer_append_str_distribute2(writer, unicode_buffer_info, cur_nested_depth, len, kind, is_in_obj, src_voidp);
        }
#endif
        case 4: {
            return unicode_buffer_append_str_distribute4(writer, unicode_buffer_info, cur_nested_depth, len, kind, is_in_obj, src_voidp);
        }
        default: {
            ssrjson_unreachable();
            return NULL;
        }
    }
}

force_inline dst_t *unicode_buffer_append_long(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, PyObject *val, bool is_in_obj) {
    assert(PyLong_CheckExact(val));
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, 64);

    if (pylong_is_zero(val)) {
        *writer++ = '0';
        *writer++ = ',';
    } else {
        u64 v;
        usize sign;
        if (pylong_is_unsigned(val)) {
            if (unlikely(!pylong_value_unsigned(val, &v))) {
                PyErr_SetString(JSONEncodeError, "convert value to unsigned long long failed");
                return NULL;
            }
            sign = 0;
        } else {
            i64 v2;
            if (unlikely(!pylong_value_signed(val, &v2))) {
                PyErr_SetString(JSONEncodeError, "convert value to long long failed");
                return NULL;
            }
            assert(v2 <= 0);
            v = -v2;
            sign = 1;
        }
        writer = u64_to_unicode(writer, v, sign);
        *writer++ = ',';
    }
    assert(check_unicode_writer_valid(writer, unicode_buffer_info));
    return writer;
}

force_inline ssrjson_nofail dst_t *write_unicode_bool(dst_t *writer, bool is_false) {
    // copy 24 bytes if bit vec size <= 128 and ucs level is 4
    ssrjson_compiletime const usize copy_cnt = (_CompileVectorBits <= 128 && COMPILE_UCS_LEVEL == 4) ? 6 : 8;
    static const dst_t true_buf[8] = {'t', 'r', 'u', 'e', ',', 0, 0, 0};
    static const dst_t false_buf[8] = {'f', 'a', 'l', 's', 'e', ',', 0, 0};
    const dst_t *copy_from = is_false ? false_buf : true_buf;
    memcpy(writer, copy_from, copy_cnt * sizeof(dst_t));
    writer += 5 + is_false;
    return writer;
}

force_inline dst_t *unicode_buffer_append_bool(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj, bool is_false) {
    // copy 24 bytes if bit vec size <= 128 and ucs level is 4
    ssrjson_compiletime const usize copy_cnt = (_CompileVectorBits <= 128 && COMPILE_UCS_LEVEL == 4) ? 6 : 8;
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, copy_cnt);
    return write_unicode_bool(writer, is_false);
}

force_inline ssrjson_nofail dst_t *write_unicode_null(dst_t *writer) {
    // ucs case       -> 1, 2, 4
    // expected bytes -> 5,10,20
    // written bytes  -> 8,16,24/32
    // written count  -> 8, 8,6/8
    // reserve count = 8
    *writer++ = 'n';
    *writer++ = 'u';
    *writer++ = 'l';
    *writer++ = 'l';
    *writer++ = ',';
    dst_t *writer2 = writer;
#if COMPILE_UCS_LEVEL < 4
    *writer2++ = 0;
    *writer2++ = 0;
    *writer2++ = 0;
#else // COMPILE_UCS_LEVEL == 4
    *writer2++ = 0;
#    if __AVX__
    *writer2++ = 0;
    *writer2++ = 0;
#    endif // __AVX__
#endif     // COMPILE_UCS_LEVEL
    return writer;
}

force_inline dst_t *unicode_buffer_append_null(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, 8);
    return write_unicode_null(writer);
}

force_inline dst_t *unicode_buffer_append_float(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, PyObject *val, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, ssrjson_dtoa_write_length);
    double v = PyFloat_AS_DOUBLE(val);
    if (!ssrjson_dtoa_handle_inf_nan && unlikely(isinf(v) || isnan(v))) {
        writer = inf_nan_to_unicode(writer, v);
    } else {
        writer = f64_to_unicode(writer, v);
    }
    *writer++ = ',';
    return writer;
}

force_inline ssrjson_nofail dst_t *write_unicode_empty_arr(dst_t *writer) {
    // reserve count = 4
    *writer++ = '[';
    *writer++ = ']';
    *writer++ = ',';
#if COMPILE_UCS_LEVEL != 4
    *writer = 0;
#endif
    return writer;
}

force_inline dst_t *unicode_buffer_append_empty_arr(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, 4);
    return write_unicode_empty_arr(writer);
}

force_inline ssrjson_nofail dst_t *write_unicode_arr_begin(dst_t *writer) {
    // reserve count = 1
    *writer++ = '[';
    return writer;
}

force_inline dst_t *unicode_buffer_append_arr_begin(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, 1);
    return write_unicode_arr_begin(writer);
}

force_inline ssrjson_nofail dst_t *write_unicode_empty_obj(dst_t *writer) {
    // reserve count = 4
    *writer++ = '{';
    *writer++ = '}';
    *writer++ = ',';
#if COMPILE_UCS_LEVEL != 4
    *writer = 0;
#endif
    return writer;
}

force_inline dst_t *unicode_buffer_append_empty_obj(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, 4);
    return write_unicode_empty_obj(writer);
}

force_inline ssrjson_nofail dst_t *write_unicode_obj_begin(dst_t *writer) {
    // reserve count = 1
    *writer++ = '{';
    return writer;
}

force_inline dst_t *unicode_buffer_append_obj_begin(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, 1);
    return write_unicode_obj_begin(writer);
}

force_inline ssrjson_nofail dst_t *write_unicode_obj_end(dst_t *writer) {
    // reserve count = 2
    *writer++ = '}';
    *writer++ = ',';
    return writer;
}

force_inline dst_t *unicode_buffer_append_obj_end(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    // remove last comma
    writer--;
    // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, false, 2);
    return write_unicode_obj_end(writer);
}

force_inline ssrjson_nofail dst_t *write_unicode_arr_end(dst_t *writer) {
    // reserve count = 2
    *writer++ = ']';
    *writer++ = ',';
    return writer;
}

force_inline dst_t *unicode_buffer_append_arr_end(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    // remove last comma
    writer--;
    // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, false, 2);
    return write_unicode_arr_end(writer);
}

force_inline EncodeUnicodeWriter encode_process_val(
        EncodeUnicodeWriter writer,
        EncodeValJumpFlag *jump_flag_out,
        EncodeUnicodeBufferInfo *unicode_buffer_info, PyObject *val,
        PyObject **cur_obj_addr,
        Py_ssize_t *cur_pos_addr,
        Py_ssize_t *cur_nested_depth_addr,
        Py_ssize_t *cur_list_size_addr,
        EncodeCtnWithIndex *ctn_stack,
        EncodeUnicodeInfo *unicode_info_addr,
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
        khash_t(ptr_set) * pyobj_set,
#endif
        ssrjson_compiletime bool is_in_obj,
        bool is_in_tuple) {
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
            writer = unicode_buffer_append_str(writer, val, unicode_buffer_info, unicode_info_addr, *cur_nested_depth_addr, is_in_obj);
            return_jump_fail_if_unlikely(!writer);
#if COMPILE_UCS_LEVEL < 1
            if (unlikely(unicode_info_addr->cur_ucs_type == 1)) {
                *jump_flag_out = is_in_obj ? JumpFlag_Elevate1_ObjVal : JumpFlag_Elevate1_ArrVal;
                return writer;
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            if (unlikely(unicode_info_addr->cur_ucs_type == 2)) {
                *jump_flag_out = is_in_obj ? JumpFlag_Elevate2_ObjVal : JumpFlag_Elevate2_ArrVal;
                return writer;
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            if (unlikely(unicode_info_addr->cur_ucs_type == 4)) {
                *jump_flag_out = is_in_obj ? JumpFlag_Elevate4_ObjVal : JumpFlag_Elevate4_ArrVal;
                return writer;
            }
#endif
            break;
        }
        case T_Long: {
            _CAST_WRITER(writer) = unicode_buffer_append_long(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, val, is_in_obj);
            return_jump_fail_if_unlikely(!writer);
            break;
        }
        case T_Bool: {
            const bool is_false = (val == Py_False);
            _CAST_WRITER(writer) = unicode_buffer_append_bool(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, is_false);
            return_jump_fail_if_unlikely(!writer);
            break;
        }
        case T_None: {
            _CAST_WRITER(writer) = unicode_buffer_append_null(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
            return_jump_fail_if_unlikely(!writer);
            break;
        }
        case T_Float: {
            _CAST_WRITER(writer) = unicode_buffer_append_float(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, val, is_in_obj);
            return_jump_fail_if_unlikely(!writer);
            break;
        }
        case T_List: {
            Py_ssize_t this_list_size = PyList_GET_SIZE(val);
            if (unlikely(this_list_size == 0)) {
                _CAST_WRITER(writer) = unicode_buffer_append_empty_arr(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
                return_jump_fail_if_unlikely(!writer);
            } else {
                _CAST_WRITER(writer) = unicode_buffer_append_arr_begin(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
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
                _CAST_WRITER(writer) = unicode_buffer_append_empty_obj(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
                return_jump_fail_if_unlikely(!writer);

            } else {
                _CAST_WRITER(writer) = unicode_buffer_append_obj_begin(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
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
                _CAST_WRITER(writer) = unicode_buffer_append_empty_arr(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
                return_jump_fail_if_unlikely(!writer);
            } else {
                _CAST_WRITER(writer) = unicode_buffer_append_arr_begin(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj);
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

#define dumps_next(_u_) ssrjson_concat3(_ssrjson_dumps_obj, _u_, __INDENT_NAME)
#if SSRJSON_GIL_ENABLED || SSRJSON_FREE_THREADING_LOCKFREE
#    define _DUMPS_PASS_ARGSDECL EncodeUnicodeWriter writer, PyObject *key, PyObject *val, PyObject *cur_obj, Py_ssize_t cur_pos, Py_ssize_t cur_nested_depth, Py_ssize_t cur_list_size, EncodeCtnWithIndex *ctn_stack, EncodeUnicodeInfo unicode_info, bool cur_is_tuple, EncodeUnicodeBufferInfo _unicode_buffer_info, EncodeCallFlag encode_call_flag
#    define _DUMPS_PASS_ARGS writer, key, val, cur_obj, cur_pos, cur_nested_depth, cur_list_size, ctn_stack, unicode_info, cur_is_tuple
#else
#    define _DUMPS_PASS_ARGSDECL EncodeUnicodeWriter writer, PyObject *key, PyObject *val, PyObject *cur_obj, Py_ssize_t cur_pos, Py_ssize_t cur_nested_depth, Py_ssize_t cur_list_size, EncodeCtnWithIndex *ctn_stack, EncodeUnicodeInfo unicode_info, bool cur_is_tuple, khash_t(ptr_set) * pyobj_set, PyObject *toplevel_locked_obj, EncodeUnicodeBufferInfo _unicode_buffer_info, EncodeCallFlag encode_call_flag
#    define _DUMPS_PASS_ARGS writer, key, val, cur_obj, cur_pos, cur_nested_depth, cur_list_size, ctn_stack, unicode_info, cur_is_tuple, pyobj_set, toplevel_locked_obj
#endif

// forward declaration
internal_simd_noinline PyObject *dumps_next(ucs1)(_DUMPS_PASS_ARGSDECL);
internal_simd_noinline PyObject *dumps_next(ucs2)(_DUMPS_PASS_ARGSDECL);
internal_simd_noinline PyObject *dumps_next(ucs4)(_DUMPS_PASS_ARGSDECL);

internal_simd_noinline PyObject *
ssrjson_dumps_obj(
#if COMPILE_UCS_LEVEL > 0
        _DUMPS_PASS_ARGSDECL
#else
        PyObject *in_obj
#endif
) {
#define goto_fail_if_unlikely(_condition)    \
    do {                                     \
        if (unlikely(_condition)) goto fail; \
    } while (0)

#if COMPILE_UCS_LEVEL == 0
    EncodeUnicodeWriter writer;
    EncodeUnicodeBufferInfo _unicode_buffer_info;
    PyObject *key, *val;
    PyObject *cur_obj = in_obj;
    Py_ssize_t cur_pos = 0;
    Py_ssize_t cur_nested_depth = 0;
    Py_ssize_t cur_list_size;
    // alias thread local buffer
    EncodeCtnWithIndex *ctn_stack;
    EncodeUnicodeInfo unicode_info;
#    if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
    khash_t(ptr_set) * pyobj_set;
    PyObject *toplevel_locked_obj = NULL;
#    endif
    bool cur_is_tuple;
    //
    memset(&unicode_info, 0, sizeof(unicode_info));
#    if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
    pyobj_set = kh_init(ptr_set);
    if (unlikely(kh_resize(ptr_set, pyobj_set, 512) != 0)) {
        PyErr_NoMemory();
        return NULL;
    }
#    endif
    writer = init_unicode_buffer(&_unicode_buffer_info);
    goto_fail_if_unlikely(!writer || !init_encode_ctn_stack(&ctn_stack));

    // this is the starting, we don't need an indent before container.
    // so is_in_obj always pass true
    if (PyDict_Check(cur_obj)) {
        if (unlikely(PyDict_GET_SIZE(cur_obj) == 0)) {
            _CAST_WRITER(writer) = unicode_buffer_append_empty_obj(_CAST_WRITER(writer), &_unicode_buffer_info, 0, true);
            assert(writer);
            goto success;
        }
        _CAST_WRITER(writer) = unicode_buffer_append_obj_begin(_CAST_WRITER(writer), &_unicode_buffer_info, 0, true);
        assert(writer && !cur_nested_depth);
#    if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
        {
            toplevel_locked_obj = cur_obj;
            int ret;
            kh_put(ptr_set, pyobj_set, (u64)cur_obj, &ret);
            assert(ret == 1);
            PyMutex_Lock(&ssrjson_pyobj_cast(cur_obj)->ob_mutex);
        }
#    endif
        cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        goto dict_pair_begin;
    } else if (PyList_Check(cur_obj)) {
        cur_list_size = PyList_GET_SIZE(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            _CAST_WRITER(writer) = unicode_buffer_append_empty_arr(_CAST_WRITER(writer), &_unicode_buffer_info, 0, true);
            assert(writer);
            goto success;
        }
        _CAST_WRITER(writer) = unicode_buffer_append_arr_begin(_CAST_WRITER(writer), &_unicode_buffer_info, 0, true);
        assert(writer && !cur_nested_depth);
#    if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
        {
            toplevel_locked_obj = cur_obj;
            int ret;
            kh_put(ptr_set, pyobj_set, (u64)cur_obj, &ret);
            assert(ret == 1);
            PyMutex_Lock(&ssrjson_pyobj_cast(cur_obj)->ob_mutex);
        }
#    endif
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
            _CAST_WRITER(writer) = unicode_buffer_append_empty_arr(_CAST_WRITER(writer), &_unicode_buffer_info, 0, true);
            assert(writer);
            goto success;
        }
        _CAST_WRITER(writer) = unicode_buffer_append_arr_begin(_CAST_WRITER(writer), &_unicode_buffer_info, 0, true);
        assert(writer && !cur_nested_depth);
        cur_nested_depth = 1;
        cur_is_tuple = true;
        goto arr_val_begin;
    }

    // ---unreachable here---
#else
    switch (encode_call_flag) {
        case CallFlag_ArrVal: {
            if (PyList_CheckExact(cur_obj)) {
                cur_is_tuple = false;
                goto arr_val_begin;
            } else {
                cur_is_tuple = true;
                goto arr_val_begin;
            }
        }
        case CallFlag_ObjVal: {
            goto dict_pair_begin;
        }
        case CallFlag_Key: {
            goto dict_key_done;
        }
        default: {
            ssrjson_unreachable();
        }
    }
#endif

dict_pair_begin:;
    assert(PyDict_GET_SIZE(cur_obj) != 0);
    if (pydict_next(cur_obj, &cur_pos, &key, &val)) {
        if (unlikely(!PyUnicode_CheckExact(key))) {
            goto fail_keytype;
        }
        writer = unicode_buffer_append_key(key, writer, &_unicode_buffer_info, &unicode_info, cur_nested_depth);
        goto_fail_if_unlikely(!writer);
        {
#if COMPILE_UCS_LEVEL < 1
            if (unlikely(unicode_info.cur_ucs_type == 1)) {
                return dumps_next(ucs1)(_DUMPS_PASS_ARGS, _unicode_buffer_info, CallFlag_Key);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            if (unlikely(unicode_info.cur_ucs_type == 2)) {
                return dumps_next(ucs2)(_DUMPS_PASS_ARGS, _unicode_buffer_info, CallFlag_Key);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            if (unlikely(unicode_info.cur_ucs_type == 4)) {
                return dumps_next(ucs4)(_DUMPS_PASS_ARGS, _unicode_buffer_info, CallFlag_Key);
            }
#endif
        }
    dict_key_done:;
        //
        EncodeValJumpFlag jump_flag;
        writer = encode_process_val(writer, &jump_flag, &_unicode_buffer_info, val, &cur_obj, &cur_pos, &cur_nested_depth, &cur_list_size, ctn_stack, &unicode_info,
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
                                    pyobj_set,
#endif
                                    true, false);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                goto dict_pair_begin;
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
#if COMPILE_UCS_LEVEL < 1
            case JumpFlag_Elevate1_ObjVal: {
                return dumps_next(ucs1)(_DUMPS_PASS_ARGS, _unicode_buffer_info, CallFlag_ObjVal);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            case JumpFlag_Elevate2_ObjVal: {
                return dumps_next(ucs2)(_DUMPS_PASS_ARGS, _unicode_buffer_info, CallFlag_ObjVal);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            case JumpFlag_Elevate4_ObjVal: {
                return dumps_next(ucs4)(_DUMPS_PASS_ARGS, _unicode_buffer_info, CallFlag_ObjVal);
            }
#endif
            default: {
                ssrjson_unreachable();
            }
        }
    } else {
        // dict end
        assert(cur_nested_depth);
        EncodeCtnWithIndex *last_pos = ctn_stack + (--cur_nested_depth);

        _CAST_WRITER(writer) = unicode_buffer_append_obj_end(_CAST_WRITER(writer), &_unicode_buffer_info, cur_nested_depth);
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
        writer = encode_process_val(writer, &jump_flag, &_unicode_buffer_info, val, &cur_obj, &cur_pos, &cur_nested_depth, &cur_list_size, ctn_stack, &unicode_info,
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
                                    pyobj_set,
#endif
                                    false, cur_is_tuple);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                goto arr_val_begin;
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
#if COMPILE_UCS_LEVEL < 1
            case JumpFlag_Elevate1_ArrVal: {
                return dumps_next(ucs1)(_DUMPS_PASS_ARGS, _unicode_buffer_info, CallFlag_ArrVal);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            case JumpFlag_Elevate2_ArrVal: {
                return dumps_next(ucs2)(_DUMPS_PASS_ARGS, _unicode_buffer_info, CallFlag_ArrVal);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            case JumpFlag_Elevate4_ArrVal: {
                return dumps_next(ucs4)(_DUMPS_PASS_ARGS, _unicode_buffer_info, CallFlag_ArrVal);
            }
#endif
            default: {
                ssrjson_unreachable();
            }
        }
    } else {
        // list end
        assert(cur_nested_depth);
        EncodeCtnWithIndex *last_pos = ctn_stack + (--cur_nested_depth);

        _CAST_WRITER(writer) = unicode_buffer_append_arr_end(_CAST_WRITER(writer), &_unicode_buffer_info, cur_nested_depth);
        goto_fail_if_unlikely(!writer);
        if (cur_nested_depth == 0) {
            goto success;
        }

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

success:;
    assert(cur_nested_depth == 0);
    // remove trailing comma
    (_CAST_WRITER(writer))--;

#if COMPILE_UCS_LEVEL == 4
    ucs2_elevate4(&_unicode_buffer_info, &unicode_info);
    ucs1_elevate4(&_unicode_buffer_info, &unicode_info);
    ascii_elevate4(&_unicode_buffer_info, &unicode_info);
#endif
#if COMPILE_UCS_LEVEL == 2
    ucs1_elevate2(&_unicode_buffer_info, &unicode_info);
    ascii_elevate2(&_unicode_buffer_info, &unicode_info);
#endif
#if COMPILE_UCS_LEVEL == 1
    ascii_elevate1(&_unicode_buffer_info, &unicode_info);
#endif
    assert(unicode_info.cur_ucs_type == COMPILE_UCS_LEVEL);
    Py_ssize_t final_len = get_unicode_buffer_final_len(writer, &_unicode_buffer_info);
    goto_fail_if_unlikely(!resize_to_fit_pyunicode(&_unicode_buffer_info, final_len, COMPILE_UCS_LEVEL));
    init_pyunicode_noinline(_unicode_buffer_info.head, final_len, COMPILE_UCS_LEVEL);
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

#undef _DUMPS_PASS_ARGS
#undef _DUMPS_PASS_ARGSDECL
#undef dumps_next

#include "compile_context/sirw_out.inl.h"

#undef write_indent_return_if_fail
//
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

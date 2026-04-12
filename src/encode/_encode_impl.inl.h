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
#    ifndef COMPILE_INDENT_LEVEL
#        define COMPILE_CONTEXT_ENCODE
#        include "encode_shared.h"
#        include "encode_unicode_impl_wrap.h"
#        include "encode_utils_impl_wrap.h"
#        include "ndarray/ndarray.h"
#        include "states.h"
#        include "tls.h"
#        include "writer_wrap.h"
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

    // compile-time optimized
    switch (ssrjson_consteval(write_kind)) {
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

static force_noinline EncodeUnicodeWriter unicode_buffer_append_str(EncodeUnicodeWriter writer, PyObject *val, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    usize len;
    unsigned int kind, write_kind;
    const void *src_voidp;
    assert(ssrjson_pyascii_cast(val)->state.compact);
    writer = prepare_unicode_write(val, writer, unicode_buffer_info, unicode_info, &len, &kind, &write_kind, &src_voidp);

    // compile-time optimized
    switch (ssrjson_consteval(write_kind)) {
#if COMPILE_UCS_LEVEL < 1
        case 0:
#endif
#if COMPILE_UCS_LEVEL < 2
        case 1: {
            return STR_WRITER_IMPL(u8, u8)(src_voidp, len, WRITER_AS_U8(writer), unicode_buffer_info, cur_nested_depth, is_in_obj);
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

force_inline dst_t *unicode_buffer_append_bool(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj, bool is_false) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, _WriteBoolCopyCnt);
    return write_unicode_bool(writer, is_false);
}

force_inline dst_t *unicode_buffer_append_null(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, 8);
    return write_unicode_null(writer);
}

force_inline dst_t *unicode_buffer_append_float(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, double v, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, ssrjson_dtoa_write_length);
    if (!ssrjson_dtoa_handle_inf_nan && unlikely(isinf(v) || isnan(v))) {
        writer = inf_nan_to_unicode(writer, v);
    } else {
        writer = f64_to_unicode(writer, v);
    }
    *writer++ = ',';
    return writer;
}

force_inline dst_t *unicode_buffer_append_f32(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, float v, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, ssrjson_ftoa_write_length);
    if (!ssrjson_ftoa_handle_inf_nan && unlikely(isinf(v) || isnan(v))) {
        writer = inf_nan_to_unicode(writer, v);
    } else {
        writer = f32_to_unicode(writer, v);
    }
    *writer++ = ',';
    return writer;
}

force_inline dst_t *unicode_buffer_append_empty_arr(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, 4);
    return write_unicode_empty_arr(writer);
}

force_inline dst_t *unicode_buffer_append_arr_begin(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, 1);
    return write_unicode_arr_begin(writer);
}

force_inline dst_t *unicode_buffer_append_empty_obj(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, 4);
    return write_unicode_empty_obj(writer);
}

force_inline dst_t *unicode_buffer_append_obj_begin(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, is_in_obj, 1);
    return write_unicode_obj_begin(writer);
}

force_inline dst_t *unicode_buffer_append_obj_end(dst_t *writer, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    // remove last comma
    writer--;
    // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
    write_indent_return_if_fail(writer, unicode_buffer_info, cur_nested_depth, false, 2);
    return write_unicode_obj_end(writer);
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
            if (ssrjson_consteval(COMPILE_UCS_LEVEL < 4) && unlikely(unicode_info_addr->cur_ucs_type > COMPILE_UCS_LEVEL)) {
#if COMPILE_UCS_LEVEL < 1
                if (unicode_info_addr->cur_ucs_type == 1) {
                    *jump_flag_out = is_in_obj ? JumpFlag_Elevate1_ObjVal : JumpFlag_Elevate1_ArrVal;
                    return writer;
                }
#endif
#if COMPILE_UCS_LEVEL < 2
                if (unicode_info_addr->cur_ucs_type == 2) {
                    *jump_flag_out = is_in_obj ? JumpFlag_Elevate2_ObjVal : JumpFlag_Elevate2_ArrVal;
                    return writer;
                }
#endif
#if COMPILE_UCS_LEVEL < 4
                assume(unicode_info_addr->cur_ucs_type == 4);
                *jump_flag_out = is_in_obj ? JumpFlag_Elevate4_ObjVal : JumpFlag_Elevate4_ArrVal;
                return writer;
#endif
            }
            break;
        }

        t_long_zero:;
            write_indent_return_if_fail(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, 2);
            *_CAST_WRITER(writer)++ = '0';
            *_CAST_WRITER(writer)++ = ',';
            break;

            {
                u64 value;
                int sign;

                case T_Long: {
                    return_jump_fail_if_unlikely(!pylong_to_clong(val, &value, &sign));
                    if (ssrjson_consteval(sign == -1)) goto t_long_zero;
                t_long_nonzero:;
                    write_indent_return_if_fail(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, 32);
                    _CAST_WRITER(writer) = u64_to_unicode(_CAST_WRITER(writer), value, sign);
                    *_CAST_WRITER(writer)++ = ',';
                    break;
                }

                case T_NumpyInt64: {
                    i64 v = pyobj_scalar_value(i64, val);
                    if (v == 0) goto t_long_zero;
                    sign = v < 0;
                    value = sign ? -(u64)v : (u64)v;
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
            double v = PyFloat_AS_DOUBLE(val);
            _CAST_WRITER(writer) = unicode_buffer_append_float(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, v, is_in_obj);
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
        case T_NumpyArray: {
#if COMPILE_WRITE_UCS_LEVEL > 1
            // keep the original offset here, because u8_buffer_append_ndarray may realloc buffer
            const usize original_u8_offset = ssrjson_cast(u8 *, writer) - ssrjson_cast(u8 *, unicode_buffer_info->head);
#endif
            u8 *new_writer = u8_buffer_append_ndarray(ssrjson_cast(u8 *, writer), unicode_buffer_info, *cur_nested_depth_addr, val, is_in_obj);
#if COMPILE_WRITE_UCS_LEVEL > 1
            return_jump_fail_if_unlikely(!new_writer);
            const usize after_write_new_u8_offset = new_writer - ssrjson_cast(u8 *, unicode_buffer_info->head);
            const usize written_cnt = after_write_new_u8_offset - original_u8_offset;
            dst_t *target_ptr = ssrjson_cast(dst_t *, ssrjson_cast(u8 *, unicode_buffer_info->head) + original_u8_offset + written_cnt * COMPILE_WRITE_UCS_LEVEL);
            if (unlikely(target_ptr > ssrjson_cast(dst_t *, unicode_buffer_info->end))) {
                usize target_u8_size = ssrjson_cast(u8 *, target_ptr) - ssrjson_cast(u8 *, unicode_buffer_info->head);
                // reserve
                EncodeUnicodeBufferInfo new_unicode_buffer_info = _unicode_buffer_reserve(*unicode_buffer_info, target_u8_size);
                return_jump_fail_if_unlikely(!new_unicode_buffer_info.head);
                *unicode_buffer_info = new_unicode_buffer_info;
            }
            _CAST_WRITER(writer) = ssrjson_cast(dst_t *, ssrjson_cast(u8 *, unicode_buffer_info->head) + original_u8_offset);
            // long back cvt
            SIMD_NAME_MODIFIER(ssrjson_concat2(long_back_cvt_noinline_u8, dst_t))(_CAST_WRITER(writer), ssrjson_cast(u8 *, writer), written_cnt);
            _CAST_WRITER(writer) += written_cnt;
#else
            _CAST_WRITER(writer) = new_writer;
            return_jump_fail_if_unlikely(!writer);
#endif
            break;
        }

            {
                float v;
                case T_NumpyFloat32: {
                    v = pyobj_scalar_value(float, val);
                t_f32:;
                    _CAST_WRITER(writer) = unicode_buffer_append_f32(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, v, is_in_obj);
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
                    value = sign ? -(u32)v : (u32)v;
                    goto t_iu32_nonzero;
                }

                t_iu32_nonzero:;
                    write_indent_return_if_fail(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, 16);
                    _CAST_WRITER(writer) = u32_to_unicode(_CAST_WRITER(writer), value, sign);
                    *_CAST_WRITER(writer)++ = ',';
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
                    value = sign ? -(u16)v : (u16)v;
                    goto t_iu16_nonzero;
                }

                t_iu16_nonzero:;
                    write_indent_return_if_fail(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, 8);
                    _CAST_WRITER(writer) = u16_to_unicode(_CAST_WRITER(writer), value, sign);
                    *_CAST_WRITER(writer)++ = ',';
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
                    value = sign ? -(u8)v : (u8)v;
                    goto t_iu8_nonzero;
                }

                t_iu8_nonzero:;
                    write_indent_return_if_fail(writer, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, 4);
                    _CAST_WRITER(writer) = u8_to_unicode(_CAST_WRITER(writer), value, sign);
                    *_CAST_WRITER(writer)++ = ',';
                    break;
            }

        case T_NumpyBool: {
            // Convert numpy bool to Python bool
            bool is_true = pyobj_scalar_value(u8, val);
            const bool is_false = !is_true;
            _CAST_WRITER(writer) = unicode_buffer_append_bool(_CAST_WRITER(writer), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj, is_false);
            return_jump_fail_if_unlikely(!writer);
            break;
        }
        default: {
            PyErr_SetString(JSONEncodeError, "Unsupported type to encode");
            *jump_flag_out = JumpFlag_Fail;
            return NULL;
        }
    }
    assert(check_unicode_writer_valid(_CAST_WRITER(writer), unicode_buffer_info));

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
            cur_is_tuple = !PyList_Check(cur_obj);
            goto arr_val_begin;
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
        if (ssrjson_consteval(COMPILE_UCS_LEVEL < 4) && unlikely(unicode_info.cur_ucs_type > COMPILE_UCS_LEVEL)) {
#if COMPILE_UCS_LEVEL < 1
            if (unicode_info.cur_ucs_type == 1) {
                return dumps_next(ucs1)(_DUMPS_PASS_ARGS, _unicode_buffer_info, CallFlag_Key);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            if (unicode_info.cur_ucs_type == 2) {
                return dumps_next(ucs2)(_DUMPS_PASS_ARGS, _unicode_buffer_info, CallFlag_Key);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            assume(unicode_info.cur_ucs_type == 4);
            // if (unlikely(unicode_info.cur_ucs_type == 4)) {
            return dumps_next(ucs4)(_DUMPS_PASS_ARGS, _unicode_buffer_info, CallFlag_Key);
            // }
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
        switch (ssrjson_consteval(jump_flag)) {
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
        switch (ssrjson_consteval(jump_flag)) {
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

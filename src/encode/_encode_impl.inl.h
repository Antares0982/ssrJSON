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


#define WRITE_INDENT_RETURN_IF_FAIL(_writer_addr_, _unicode_buffer_info_, _cur_nested_depth_, _is_in_obj_, _additional_reserve_count_)                         \
    do {                                                                                                                                                       \
        if (unlikely(!unicode_indent_writer(_writer_addr_, _unicode_buffer_info_, _cur_nested_depth_, _is_in_obj_, _additional_reserve_count_))) return false; \
    } while (0)

force_inline void prepare_unicode_write(PyObject *obj, EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *restrict unicode_info, usize *out_len, unsigned int *read_pykind, unsigned int *write_kind, const void **src_addr) {
    usize out_len_val = (usize)PyUnicode_GET_LENGTH(obj);
    *out_len = out_len_val;
    unsigned int read_kind_val = PyUnicode_KIND(obj);
    *read_pykind = read_kind_val;
    bool is_ascii = PyUnicode_IS_ASCII(obj);
    *src_addr = is_ascii ? PYUNICODE_ASCII_START(obj) : PYUNICODE_UCS1_START(obj);

#if COMPILE_UCS_LEVEL == 4
    *write_kind = 4;
#elif COMPILE_UCS_LEVEL == 2
    *write_kind = 2;
    if (unlikely(read_kind_val == 4)) {
        memorize_ucs2_to_ucs4(writer_addr, unicode_buffer_info, unicode_info);
        *write_kind = 4;
    }
#elif COMPILE_UCS_LEVEL == 1
    *write_kind = 1;
    if (unlikely(read_kind_val == 2)) {
        memorize_ucs1_to_ucs2(writer_addr, unicode_buffer_info, unicode_info);
        *write_kind = 2;
    } else if (unlikely(read_kind_val == 4)) {
        memorize_ucs1_to_ucs4(writer_addr, unicode_buffer_info, unicode_info);
        *write_kind = 4;
    }
#elif COMPILE_UCS_LEVEL == 0
    *write_kind = 0;
    if (unlikely(read_kind_val == 2)) {
        memorize_ascii_to_ucs2(writer_addr, unicode_buffer_info, unicode_info);
        *write_kind = 2;
    } else if (unlikely(read_kind_val == 4)) {
        memorize_ascii_to_ucs4(writer_addr, unicode_buffer_info, unicode_info);
        *write_kind = 4;
    } else if (unlikely(!is_ascii)) {
        memorize_ascii_to_ucs1(writer_addr, unicode_buffer_info, unicode_info);
        *write_kind = 1;
    }
#endif
}

#if COMPILE_UCS_LEVEL < 4
force_inline bool unicode_buffer_append_key_distribute2(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, usize len, unsigned int pykind, const void *src_voidp) {
#    if COMPILE_UCS_LEVEL == 2
    if (pykind == 1) {
        const u8 *src = src_voidp;
        RETURN_ON_UNLIKELY_ERR(!KEY_WRITER_IMPL(u8, u16)(src, len, &WRITER_ADDR_AS_U16(writer_addr), unicode_buffer_info, cur_nested_depth));
        return true;
    } else {
#    endif
        assert(pykind == 2);
        const u16 *src = src_voidp;
        RETURN_ON_UNLIKELY_ERR(!KEY_WRITER_IMPL(u16, u16)(src, len, &WRITER_ADDR_AS_U16(writer_addr), unicode_buffer_info, cur_nested_depth));
        return true;
#    if COMPILE_UCS_LEVEL == 2
    }
#    endif
}
#endif

force_inline bool unicode_buffer_append_key_distribute4(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, usize len, unsigned int pykind, const void *src_voidp) {
#if COMPILE_READ_UCS_LEVEL == 4
    if (pykind == 1) {
        const u8 *src = src_voidp;
        RETURN_ON_UNLIKELY_ERR(!KEY_WRITER_IMPL(u8, u32)(src, len, &WRITER_ADDR_AS_U32(writer_addr), unicode_buffer_info, cur_nested_depth));
        return true;
    } else if (pykind == 2) {
        const u16 *src = src_voidp;
        RETURN_ON_UNLIKELY_ERR(!KEY_WRITER_IMPL(u16, u32)(src, len, &WRITER_ADDR_AS_U32(writer_addr), unicode_buffer_info, cur_nested_depth));
        return true;
    } else {
#endif
        assert(pykind == 4);
        const u32 *src = src_voidp;
        RETURN_ON_UNLIKELY_ERR(!KEY_WRITER_IMPL(u32, u32)(src, len, &WRITER_ADDR_AS_U32(writer_addr), unicode_buffer_info, cur_nested_depth));
        return true;
#if COMPILE_READ_UCS_LEVEL == 4
    }
#endif
}

force_inline bool unicode_buffer_append_key(PyObject *key, EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info, Py_ssize_t cur_nested_depth) {
    usize len;
    unsigned int pykind, write_kind;
    const void *src_voidp;
    assert(SSRJSON_CAST(PyASCIIObject *, key)->state.compact);
    prepare_unicode_write(key, writer_addr, unicode_buffer_info, unicode_info, &len, &pykind, &write_kind, &src_voidp);

    switch (write_kind) {
#if COMPILE_UCS_LEVEL < 1
        case 0:
#endif
#if COMPILE_UCS_LEVEL < 2
        case 1: {
            const u8 *src = src_voidp;
            RETURN_ON_UNLIKELY_ERR(!KEY_WRITER_IMPL(u8, u8)(src, len, &WRITER_ADDR_AS_U8(writer_addr), unicode_buffer_info, cur_nested_depth));
            return true;
        }
#endif
#if COMPILE_UCS_LEVEL < 4
        case 2: {
            return unicode_buffer_append_key_distribute2(writer_addr, unicode_buffer_info, cur_nested_depth, len, pykind, src_voidp);
        }
#endif
        case 4: {
            return unicode_buffer_append_key_distribute4(writer_addr, unicode_buffer_info, cur_nested_depth, len, pykind, src_voidp);
        }
        default: {
            SSRJSON_UNREACHABLE();
            return false;
        }
    }
}

#if COMPILE_UCS_LEVEL < 4
force_inline bool unicode_buffer_append_str_distribute2(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, usize len, unsigned int pykind, bool is_in_obj, const void *src_voidp) {
#    if COMPILE_UCS_LEVEL == 2
    if (pykind == 1) {
        const u8 *src = src_voidp;
        RETURN_ON_UNLIKELY_ERR(!STR_WRITER_IMPL(u8, u16)(src, len, &WRITER_ADDR_AS_U16(writer_addr), unicode_buffer_info, cur_nested_depth, is_in_obj));
        return true;
    } else {
#    endif
        assert(pykind == 2);
        const u16 *src = src_voidp;
        RETURN_ON_UNLIKELY_ERR(!STR_WRITER_IMPL(u16, u16)(src, len, &WRITER_ADDR_AS_U16(writer_addr), unicode_buffer_info, cur_nested_depth, is_in_obj));
        return true;
#    if COMPILE_UCS_LEVEL == 2
    }
#    endif
}
#endif

force_inline bool unicode_buffer_append_str_distribute4(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, usize len, unsigned int pykind, bool is_in_obj, const void *src_voidp) {
#if COMPILE_UCS_LEVEL == 4
    if (pykind == 1) {
        const u8 *src = src_voidp;
        RETURN_ON_UNLIKELY_ERR(!STR_WRITER_IMPL(u8, u32)(src, len, &WRITER_ADDR_AS_U32(writer_addr), unicode_buffer_info, cur_nested_depth, is_in_obj));
        return true;
    } else if (pykind == 2) {
        const u16 *src = src_voidp;
        RETURN_ON_UNLIKELY_ERR(!STR_WRITER_IMPL(u16, u32)(src, len, &WRITER_ADDR_AS_U32(writer_addr), unicode_buffer_info, cur_nested_depth, is_in_obj));
        return true;
    } else {
#endif
        assert(pykind == 4);
        const u32 *src = src_voidp;
        RETURN_ON_UNLIKELY_ERR(!STR_WRITER_IMPL(u32, u32)(src, len, &WRITER_ADDR_AS_U32(writer_addr), unicode_buffer_info, cur_nested_depth, is_in_obj));
        return true;
#if COMPILE_UCS_LEVEL == 4
    }
#endif
}

force_inline bool unicode_buffer_append_str(PyObject *val, EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    usize len;
    unsigned int kind, write_kind;
    const void *src_voidp;
    assert(SSRJSON_CAST(PyASCIIObject *, val)->state.compact);
    prepare_unicode_write(val, writer_addr, unicode_buffer_info, unicode_info, &len, &kind, &write_kind, &src_voidp);

    switch (write_kind) {
#if COMPILE_UCS_LEVEL < 1
        case 0:
#endif
#if COMPILE_UCS_LEVEL < 2
        case 1: {
            const u8 *src = src_voidp;
            RETURN_ON_UNLIKELY_ERR(!STR_WRITER_IMPL(u8, u8)(src, len, &WRITER_ADDR_AS_U8(writer_addr), unicode_buffer_info, cur_nested_depth, is_in_obj));
            return true;
        }
#endif
#if COMPILE_UCS_LEVEL < 4
        case 2: {
            return unicode_buffer_append_str_distribute2(writer_addr, unicode_buffer_info, cur_nested_depth, len, kind, is_in_obj, src_voidp);
        }
#endif
        case 4: {
            return unicode_buffer_append_str_distribute4(writer_addr, unicode_buffer_info, cur_nested_depth, len, kind, is_in_obj, src_voidp);
        }
        default: {
            SSRJSON_UNREACHABLE();
            return false;
        }
    }
}

force_inline bool unicode_buffer_append_long(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, PyObject *val, bool is_in_obj) {
    assert(PyLong_CheckExact(val));
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, 64);
    _dst_t *writer = *writer_addr;

    if (pylong_is_zero(val)) {
        *writer++ = '0';
        *writer++ = ',';
    } else {
        u64 v;
        usize sign;
        if (pylong_is_unsigned(val)) {
            if (unlikely(!pylong_value_unsigned(val, &v))) {
                PyErr_SetString(JSONEncodeError, "convert value to unsigned long long failed");
                return false;
            }
            sign = 0;
        } else {
            i64 v2;
            if (unlikely(!pylong_value_signed(val, &v2))) {
                PyErr_SetString(JSONEncodeError, "convert value to long long failed");
                return false;
            }
            assert(v2 <= 0);
            v = -v2;
            sign = 1;
        }
        u64_to_unicode(&writer, v, sign);
        *writer++ = ',';
    }
    assert(check_unicode_writer_valid(writer, unicode_buffer_info));
    *writer_addr = writer;
    return true;
}

force_inline void write_unicode_false(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    // ucs case       -> 1, 2, 4
    // expected bytes -> 6,12,24
    // written bytes  -> 8,16,24/32
    // written count  -> 8, 8,6/8
    // reserve count = 8
    *writer++ = 'f';
    *writer++ = 'a';
    *writer++ = 'l';
    *writer++ = 's';
    *writer++ = 'e';
    *writer++ = ',';
    _dst_t *writer2 = writer;
#if COMPILE_UCS_LEVEL < 4
    *writer2++ = 0;
    *writer2++ = 0;
#else // COMPILE_UCS_LEVEL == 4
#    if __AVX__
    *writer2++ = 0;
    *writer2++ = 0;
#    endif // __AVX__
#endif     // COMPILE_UCS_LEVEL
    *writer_addr = writer;
}

force_inline bool unicode_buffer_append_false(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, 8);
    write_unicode_false(writer_addr);
    return true;
}

force_inline void write_unicode_true(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    _dst_t *writer2 = writer;
    // ucs case       -> 1, 2, 4
    // expected bytes -> 5,10,20
    // written bytes  -> 8,16,24/32
    // written count  -> 8, 8,6/8
    // reserve count = 8
    *writer++ = 't';
    *writer++ = 'r';
    *writer++ = 'u';
    *writer++ = 'e';
    *writer++ = ',';
#if COMPILE_UCS_LEVEL < 4
    *writer++ = 0;
    *writer++ = 0;
    *writer++ = 0;
#else // COMPILE_UCS_LEVEL == 4
    *writer++ = 0;
#    if __AVX__
    *writer++ = 0;
    *writer++ = 0;
#    endif // __AVX__
#endif     // COMPILE_UCS_LEVEL
    *writer_addr = writer2 + 5;
}

force_inline bool unicode_buffer_append_true(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, 8);
    write_unicode_true(writer_addr);
    return true;
}

force_inline void write_unicode_null(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    _dst_t *writer2 = writer;
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
#if COMPILE_UCS_LEVEL < 4
    *writer++ = 0;
    *writer++ = 0;
    *writer++ = 0;
#else // COMPILE_UCS_LEVEL == 4
    *writer++ = 0;
#    if __AVX__
    *writer++ = 0;
    *writer++ = 0;
#    endif // __AVX__
#endif     // COMPILE_UCS_LEVEL
    *writer_addr = writer2 + 5;
}

force_inline bool unicode_buffer_append_null(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, 8);
    write_unicode_null(writer_addr);
    return true;
}

force_inline bool unicode_buffer_append_float(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, PyObject *val, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, 32);
    double v = PyFloat_AS_DOUBLE(val);
    u64 raw = *SSRJSON_CAST(u64 *, &v);
    _dst_t *writer = *writer_addr;
    f64_to_unicode(&writer, raw);
    *writer++ = ',';
    *writer_addr = writer;
    return true;
}

force_inline void write_unicode_empty_arr(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    // reserve count = 4
    *writer++ = '[';
    *writer++ = ']';
    *writer++ = ',';
#if COMPILE_UCS_LEVEL != 4
    *writer = 0;
#endif
    *writer_addr = writer;
}

force_inline bool unicode_buffer_append_empty_arr(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, 4);
    write_unicode_empty_arr(writer_addr);
    return true;
}

force_inline void write_unicode_arr_begin(_dst_t **writer_addr) {
    // reserve count = 1
    *(*writer_addr)++ = '[';
}

force_inline bool unicode_buffer_append_arr_begin(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, 1);
    write_unicode_arr_begin(writer_addr);
    return true;
}

force_inline void write_unicode_empty_obj(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    // reserve count = 4
    *writer++ = '{';
    *writer++ = '}';
    *writer++ = ',';
#if COMPILE_UCS_LEVEL != 4
    *writer = 0;
#endif
    *writer_addr = writer;
}

force_inline bool unicode_buffer_append_empty_obj(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, 4);
    write_unicode_empty_obj(writer_addr);
    return true;
}

force_inline void write_unicode_obj_begin(_dst_t **writer_addr) {
    // reserve count = 1
    *(*writer_addr)++ = '{';
}

force_inline bool unicode_buffer_append_obj_begin(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, 1);
    write_unicode_obj_begin(writer_addr);
    return true;
}

force_inline void write_unicode_obj_end(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    // reserve count = 2
    *writer++ = '}';
    *writer++ = ',';
    *writer_addr = writer;
}

force_inline bool unicode_buffer_append_obj_end(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    _dst_t *writer = *writer_addr;
    // remove last comma
    writer--;
    // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
    WRITE_INDENT_RETURN_IF_FAIL(&writer, unicode_buffer_info, cur_nested_depth, false, 2);
    write_unicode_obj_end(&writer);
    *writer_addr = writer;
    return true;
}

force_inline void write_unicode_arr_end(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    // reserve count = 2
    *writer++ = ']';
    *writer++ = ',';
    *writer_addr = writer;
}

force_inline bool unicode_buffer_append_arr_end(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    _dst_t *writer = *writer_addr;
    // remove last comma
    writer--;
    // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
    WRITE_INDENT_RETURN_IF_FAIL(&writer, unicode_buffer_info, cur_nested_depth, false, 2);
    write_unicode_arr_end(&writer);
    *writer_addr = writer;
    return true;
}

force_inline EncodeValJumpFlag encode_process_val(
        EncodeUnicodeWriter *writer_addr,
        EncodeUnicodeBufferInfo *unicode_buffer_info, PyObject *val,
        PyObject **cur_obj_addr,
        Py_ssize_t *cur_pos_addr,
        Py_ssize_t *cur_nested_depth_addr,
        Py_ssize_t *cur_list_size_addr,
        EncodeCtnWithIndex *ctn_stack,
        EncodeUnicodeInfo *unicode_info_addr,
        bool is_in_obj, bool is_in_tuple) {
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
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_str(val, writer_addr, unicode_buffer_info, unicode_info_addr, *cur_nested_depth_addr, is_in_obj));
#if COMPILE_UCS_LEVEL < 1
            if (unlikely(unicode_info_addr->cur_ucs_type == 1)) {
                return is_in_obj ? JumpFlag_Elevate1_ObjVal : JumpFlag_Elevate1_ArrVal;
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            if (unlikely(unicode_info_addr->cur_ucs_type == 2)) {
                return is_in_obj ? JumpFlag_Elevate2_ObjVal : JumpFlag_Elevate2_ArrVal;
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            if (unlikely(unicode_info_addr->cur_ucs_type == 4)) {
                return is_in_obj ? JumpFlag_Elevate4_ObjVal : JumpFlag_Elevate4_ArrVal;
            }
#endif
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

#define dumps_next(_u_) SSRJSON_CONCAT3(_ssrjson_dumps_obj, _u_, __INDENT_NAME)
#define _DUMPS_PASS_ARGSDECL EncodeUnicodeWriter writer, PyObject *key, PyObject *val, PyObject *cur_obj, Py_ssize_t cur_pos, Py_ssize_t cur_nested_depth, Py_ssize_t cur_list_size, EncodeCtnWithIndex *ctn_stack, EncodeUnicodeInfo unicode_info, bool cur_is_tuple, EncodeUnicodeBufferInfo _unicode_buffer_info, EncodeCallFlag encode_call_flag
#define _DUMPS_PASS_ARGS writer, key, val, cur_obj, cur_pos, cur_nested_depth, cur_list_size, ctn_stack, unicode_info, cur_is_tuple

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
#define GOTO_FAIL_ON_UNLIKELY_ERR(_condition) \
    do {                                      \
        if (unlikely(_condition)) goto fail;  \
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
    bool cur_is_tuple;
    memset(&unicode_info, 0, sizeof(unicode_info));
    //
    GOTO_FAIL_ON_UNLIKELY_ERR(!init_unicode_buffer(&writer, &_unicode_buffer_info) || !init_encode_ctn_stack(&ctn_stack));

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
            SSRJSON_UNREACHABLE();
        }
    }
#endif

dict_pair_begin:;
    assert(PyDict_GET_SIZE(cur_obj) != 0);
    if (pydict_next(cur_obj, &cur_pos, &key, &val)) {
        if (unlikely(!PyUnicode_CheckExact(key))) {
            goto fail_keytype;
        }
        GOTO_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_key(key, &writer, &_unicode_buffer_info, &unicode_info, cur_nested_depth));
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
        EncodeValJumpFlag jump_flag = encode_process_val(&writer, &_unicode_buffer_info, val, &cur_obj, &cur_pos, &cur_nested_depth, &cur_list_size, ctn_stack, &unicode_info, true, false);
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
                SSRJSON_UNREACHABLE();
            }
        }
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
        EncodeValJumpFlag jump_flag = encode_process_val(&writer, &_unicode_buffer_info, val, &cur_obj, &cur_pos, &cur_nested_depth, &cur_list_size, ctn_stack, &unicode_info, false, cur_is_tuple);
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
                SSRJSON_UNREACHABLE();
            }
        }
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

success:;
    assert(cur_nested_depth == 0);
    // remove trailing comma
    (_CAST_WRITER(&writer))--;

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
    GOTO_FAIL_ON_UNLIKELY_ERR(!resize_to_fit_pyunicode(&_unicode_buffer_info, final_len, COMPILE_UCS_LEVEL));
    init_pyunicode_noinline(_unicode_buffer_info.head, final_len, COMPILE_UCS_LEVEL);
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

#undef _DUMPS_PASS_ARGS
#undef _DUMPS_PASS_ARGSDECL
#undef dumps_next

#include "compile_context/sirw_out.inl.h"

#undef WRITE_INDENT_RETURN_IF_FAIL
//
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

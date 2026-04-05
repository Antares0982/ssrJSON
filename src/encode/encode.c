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

#define COMPILE_CONTEXT_ENCODE

#include "encode_shared.h"
#include "simd/cvt.h"
#include "simd/memcpy.h"
#include "simd/simd_detect.h"
#include "simd/simd_impl.h"
#include "tls.h"
#include "utils/unicode.h"

/* Implmentations of some inline functions used in current scope */
#include "encode/indent_writer.h"
#include "reserve_wrap.h"

#include "encode_cvt.h"
#include "pyutils.h"
#include "states.h"

/* 
 * Some utility functions only related to *write*, like unicode buffer reserve, writing number
 * need macro: COMPILE_WRITE_UCS_LEVEL, value: 1, 2, or 4.
 */
#include "encode_utils_impl_wrap.h"

/* 
 * Top-level encode functions for encoding container types: dict, list and tuple.
 * need macro:
 *      COMPILE_UCS_LEVEL, value: 0, 1, 2, or 4. COMPILE_UCS_LEVEL is the current writing level.
 *          This differs from COMPILE_WRITE_UCS_LEVEL: `0` stands for ascii. Since we always start from
 *          writing ascii, `0` also defines the entrance of encoding containers. See `ssrjson_dumps_obj`
 *          for more details.
 *      COMPILE_INDENT_LEVEL, value: 0, 2, or 4.
 */
#include "encode_impl_wrap.h"

#include "bytes/encode_utf8.h"

/* 
 * Top-level encode functions for encoding container types tp bytes.
 * need macro:
 *      COMPILE_INDENT_LEVEL, value: 0, 2, or 4.
 */
#include "bytes/encode_bytes_impl_wrap.h"

#include "simd/compile_feature_check.h"
//
#include "compile_context/s_in.inl.h"

/* Encodes non-container types. */
force_inline PyObject *_ssrjson_dumps_single_unicode(PyObject *unicode, ssrjson_compiletime bool to_bytes_obj, bool is_write_cache) {
    EncodeUnicodeWriter writer;
    EncodeUnicodeBufferInfo _unicode_buffer_info; //, new_unicode_buffer_info;
    _unicode_buffer_info.head = PyObject_Malloc(SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    return_if_no_memory(_unicode_buffer_info.head);
    //
    bool compact = ssrjson_pyascii_cast(unicode)->state.compact;
    assert(compact);
    usize len;
    int unicode_kind;
    bool is_ascii;
    //
    usize write_offset;
    if (ssrjson_consteval(to_bytes_obj)) {
        write_offset = PYBYTES_START_OFFSET;
    } else {
        len = (usize)PyUnicode_GET_LENGTH(unicode);
        unicode_kind = PyUnicode_KIND(unicode);
        is_ascii = PyUnicode_IS_ASCII(unicode);
        write_offset = is_ascii ? sizeof(PyASCIIObject) : sizeof(PyCompactUnicodeObject);
    }
    WRITER_AS_U8(writer) = ssrjson_cast(u8 *, _unicode_buffer_info.head) + write_offset;
    _unicode_buffer_info.end = ssrjson_cast(u8 *, _unicode_buffer_info.head) + SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE;
    //
    bool success;
    if (ssrjson_consteval(to_bytes_obj)) {
        WRITER_AS_U8(writer) = bytes_buffer_append_str_indent0(WRITER_AS_U8(writer), unicode, &_unicode_buffer_info, 0, true, is_write_cache);
        success = !!writer;
        WRITER_AS_U8(writer)
        --;
    } else {
        switch (unicode_kind) {
            // pass `is_in_obj = true` to avoid unwanted indent check
            case 1: {
                const u8 *src = is_ascii ? ssrjson_pyunicode_ascii_start(unicode) : ssrjson_pyunicode_ucs1_start(unicode);
                WRITER_AS_U8(writer) = STR_WRITER_NOINDENT_IMPL(u8, u8)(src, len, WRITER_AS_U8(writer), &_unicode_buffer_info, 0, true);
                success = !!writer;
                WRITER_AS_U8(writer)
                --;
                break;
            }
            case 2: {
                const u16 *src = ssrjson_pyunicode_ucs2_start(unicode);
                WRITER_AS_U16(writer) = STR_WRITER_NOINDENT_IMPL(u16, u16)(src, len, WRITER_AS_U16(writer), &_unicode_buffer_info, 0, true);
                success = !!writer;
                WRITER_AS_U16(writer)
                --;
                break;
            }
            case 4: {
                const u32 *src = ssrjson_pyunicode_ucs4_start(unicode);
                WRITER_AS_U32(writer) = STR_WRITER_NOINDENT_IMPL(u32, u32)(src, len, WRITER_AS_U32(writer), &_unicode_buffer_info, 0, true);
                success = !!writer;
                WRITER_AS_U32(writer)
                --;
                break;
            }
            default: {
                ssrjson_unreachable();
            }
        }
    }
    if (unlikely(!success)) {
        // realloc failed when encoding, the original buffer is still valid
        PyObject_Free(_unicode_buffer_info.head);
        return NULL;
    }
    usize written_len = (uintptr_t)writer - (uintptr_t)_unicode_buffer_info.head - write_offset;
    if (ssrjson_consteval(!to_bytes_obj)) {
        written_len /= unicode_kind;
    }
    assert(written_len >= 2);
    bool resize_success;
    if (ssrjson_consteval(to_bytes_obj)) {
        resize_success = resize_to_fit_pybytes(&_unicode_buffer_info, written_len);
    } else {
        resize_success = resize_to_fit_pyunicode(&_unicode_buffer_info, written_len, is_ascii ? 0 : unicode_kind);
    }
    if (unlikely(!resize_success)) {
        // realloc failed when encoding, the original buffer is still valid
        PyObject_Free(_unicode_buffer_info.head);
        return NULL;
    }
    if (ssrjson_consteval(to_bytes_obj)) {
        init_pybytes(_unicode_buffer_info.head, written_len);
    } else {
        init_pyunicode_noinline(_unicode_buffer_info.head, written_len, is_ascii ? 0 : unicode_kind);
    }
    return (PyObject *)_unicode_buffer_info.head;
}

static force_noinline PyObject *ssrjson_dumps_single_unicode_to_str(PyObject *unicode) {
    return _ssrjson_dumps_single_unicode(unicode, false, false);
}

static force_noinline PyObject *ssrjson_dumps_single_unicode_to_bytes(PyObject *unicode, bool is_write_cache) {
    return _ssrjson_dumps_single_unicode(unicode, true, is_write_cache);
}

#include "compile_context/s_out.inl.h"
#undef _CompileVectorBits

force_inline PyObject *create_empty_pybytes(usize size) {
    PyObject *bytes = PyObject_Malloc(PYBYTES_START_OFFSET + size + 1);
    return_if_no_memory(bytes);
    init_pybytes(bytes, size);
    return bytes;
}

force_inline u8 *create_obj_and_get_writer(PyObject **obj_out, usize size, ssrjson_compiletime bool to_bytes_obj) {
    u8 *writer;
    PyObject *obj;
    if (ssrjson_consteval(to_bytes_obj)) {
        *obj_out = obj = create_empty_pybytes(size);
        return_if_unlikely(!obj);
        writer = ssrjson_cast(u8 *, ssrjson_pybytes_cast(obj)->ob_sval);
    } else {
        *obj_out = obj = create_empty_unicode(size, 0);
        return_if_unlikely(!obj);
        writer = ssrjson_pyunicode_ascii_start(obj);
    }
    return writer;
}

force_inline PyObject *create_obj_from_buffer(const u8 *buffer, usize size, ssrjson_compiletime bool to_bytes_obj) {
    assert(buffer[size] == 0);
    PyObject *obj;
    u8 *writer = create_obj_and_get_writer(&obj, size, to_bytes_obj);
    if (likely(writer)) {
        memcpy(writer, buffer, size + 1);
    }
    return obj;
}

force_inline PyObject *create_obj_from_literal(const char *literal, ssrjson_compiletime bool to_bytes_obj) {
    return create_obj_from_buffer(ssrjson_cast(const u8 *, literal), strlen(literal), to_bytes_obj);
}

force_inline PyObject *_ssrjson_dumps_single_inf_nan(double v, ssrjson_compiletime bool to_bytes_obj) {
    if (isinf(v)) {
        const bool sign = v < 0;
        const usize length = 8 + sign;
        PyObject *obj;
        u8 *const writer = create_obj_and_get_writer(&obj, length, to_bytes_obj);
        return_if_unlikely(!writer);
        *writer = '-';
        memcpy(writer + sign, "Infinity", 9);
        return obj;
    } else {
        return create_obj_from_literal("NaN", to_bytes_obj);
    }
}

force_inline PyObject *ssrjson_dumps_single_float_from_f64(double v, ssrjson_compiletime bool to_bytes_obj) {
    u8 buffer[ssrjson_dtoa_write_length];
    u8 *buffer_end;
    if (ssrjson_consteval(!ssrjson_dtoa_handle_inf_nan) && unlikely(isinf(v) || isnan(v))) {
        return _ssrjson_dumps_single_inf_nan(v, to_bytes_obj);
    }
    buffer_end = xjb64(v, buffer);
    const usize string_size = buffer_end - buffer;
    assert(string_size <= ssrjson_dtoa_output_maxlen);
    buffer[string_size] = 0;
    return create_obj_from_buffer(buffer, string_size, to_bytes_obj);
}

force_inline PyObject *ssrjson_dumps_single_float(PyObject *val, ssrjson_compiletime bool to_bytes_obj) {
    return ssrjson_dumps_single_float_from_f64(PyFloat_AS_DOUBLE(val), to_bytes_obj);
}

force_inline PyObject *ssrjson_dumps_single_long_zero(ssrjson_compiletime bool to_bytes_obj) {
    return create_obj_from_literal("0", to_bytes_obj);
}

force_inline PyObject *ssrjson_dumps_single_long_from_u64_nonzero(u64 v, bool sign, ssrjson_compiletime bool to_bytes_obj) {
    PyObject *ret;
    u8 buffer[32];
    *buffer = '-';
    u8 *buffer_end = write_u64(v, buffer + sign);
    const usize string_size = buffer_end - buffer;
    assert(string_size <= 21);
    buffer[string_size] = 0;
    return create_obj_from_buffer(buffer, string_size, to_bytes_obj);
}

force_inline PyObject *ssrjson_dumps_single_long_from_u32_nonzero(u32 v, bool sign, ssrjson_compiletime bool to_bytes_obj) {
    PyObject *ret;
    u8 buffer[16];
    *buffer = '-';
    u8 *buffer_end = write_u32(v, buffer + sign);
    const usize string_size = buffer_end - buffer;
    assert(string_size <= 11);
    buffer[string_size] = 0;
    return create_obj_from_buffer(buffer, string_size, to_bytes_obj);
}

force_inline PyObject *ssrjson_dumps_single_long_from_u16_nonzero(u16 v, bool sign, ssrjson_compiletime bool to_bytes_obj) {
    PyObject *ret;
    u8 buffer[8];
    *buffer = '-';
    u8 *buffer_end = write_u16(v, buffer + sign);
    const usize string_size = buffer_end - buffer;
    assert(string_size <= 6);
    buffer[string_size] = 0;
    return create_obj_from_buffer(buffer, string_size, to_bytes_obj);
}

force_inline PyObject *ssrjson_dumps_single_long_from_u8_nonzero(u8 v, bool sign, ssrjson_compiletime bool to_bytes_obj) {
    PyObject *ret;
    u8 buffer[8];
    *buffer = '-';
    u8 *buffer_end = write_u8(v, buffer + sign);
    const usize string_size = buffer_end - buffer;
    assert(string_size <= 4);
    buffer[string_size] = 0;
    return create_obj_from_buffer(buffer, string_size, to_bytes_obj);
}

static force_noinline PyObject *ssrjson_dumps_to_str_single_long_from_u64(u64 v, bool sign) {
    if (v == 0) return ssrjson_dumps_single_long_zero(false);
    return ssrjson_dumps_single_long_from_u64_nonzero(v, sign, false);
}

static force_noinline PyObject *ssrjson_dumps_to_bytes_single_long_from_u64(u64 v, bool sign) {
    if (v == 0) return ssrjson_dumps_single_long_zero(true);
    return ssrjson_dumps_single_long_from_u64_nonzero(v, sign, true);
}

static force_noinline PyObject *ssrjson_dumps_to_str_single_long_from_u32(u32 v, bool sign) {
    if (v == 0) return ssrjson_dumps_single_long_zero(false);
    return ssrjson_dumps_single_long_from_u32_nonzero(v, sign, false);
}

static force_noinline PyObject *ssrjson_dumps_to_bytes_single_long_from_u32(u32 v, bool sign) {
    if (v == 0) return ssrjson_dumps_single_long_zero(true);
    return ssrjson_dumps_single_long_from_u32_nonzero(v, sign, true);
}

static force_noinline PyObject *ssrjson_dumps_to_str_single_long_from_u16(u16 v, bool sign) {
    if (v == 0) return ssrjson_dumps_single_long_zero(false);
    return ssrjson_dumps_single_long_from_u16_nonzero(v, sign, false);
}

static force_noinline PyObject *ssrjson_dumps_to_bytes_single_long_from_u16(u16 v, bool sign) {
    if (v == 0) return ssrjson_dumps_single_long_zero(true);
    return ssrjson_dumps_single_long_from_u16_nonzero(v, sign, true);
}

static force_noinline PyObject *ssrjson_dumps_to_str_single_long_from_u8(u8 v, bool sign) {
    if (v == 0) return ssrjson_dumps_single_long_zero(false);
    return ssrjson_dumps_single_long_from_u8_nonzero(v, sign, false);
}

static force_noinline PyObject *ssrjson_dumps_to_bytes_single_long_from_u8(u8 v, bool sign) {
    if (v == 0) return ssrjson_dumps_single_long_zero(true);
    return ssrjson_dumps_single_long_from_u8_nonzero(v, sign, true);
}

force_inline PyObject *ssrjson_dumps_single_long(PyObject *val, ssrjson_compiletime bool to_bytes_obj) {
    PyObject *ret;
    u64 v;
    int sign;

    return_if_unlikely(!pylong_to_clong(val, &v, &sign));

    if (ssrjson_consteval(sign == -1)) return ssrjson_dumps_single_long_zero(to_bytes_obj);
    return ssrjson_dumps_single_long_from_u64_nonzero(v, sign, to_bytes_obj);
}

force_inline PyObject *ssrjson_dumps_single_constant(EncodePyTypes py_type, PyObject *obj, ssrjson_compiletime bool to_bytes_obj) {
    if (py_type == T_Bool) {
        const bool is_false = (obj == Py_False);
        return create_obj_from_literal(is_false ? "false" : "true", to_bytes_obj);
    }
    return create_obj_from_literal("null", to_bytes_obj);
}

force_inline void invalid_arg_warning(void) {
    fprintf(stderr, "Warning: some options are not supported in this version of ssrjson\n");
    _InvalidArgChecked = 1;
}

force_inline PyObject *ssrjson_dumps_single_ndarray(PyObject *obj, int indent_int, ssrjson_compiletime bool to_bytes_obj) {
    EncodeUnicodeBufferInfo _unicode_buffer_info;
    usize write_offset;
    if (ssrjson_consteval(to_bytes_obj)) {
        write_offset = PYBYTES_START_OFFSET;
    } else {
        write_offset = sizeof(PyASCIIObject);
    }
    _unicode_buffer_info.head = PyObject_Malloc(SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    return_if_no_memory(_unicode_buffer_info.head);
    u8 *writer = ssrjson_cast(u8 *, _unicode_buffer_info.head) + write_offset;
    _unicode_buffer_info.end = ssrjson_cast(u8 *, _unicode_buffer_info.head) + SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE;

    u8 *new_writer;
    switch (indent_int) {
        case 0:
            new_writer = u8_buffer_append_ndarray_indent0(writer, &_unicode_buffer_info, 0, obj, true);
            break;
        case 2:
            new_writer = u8_buffer_append_ndarray_indent2(writer, &_unicode_buffer_info, 0, obj, true);
            break;
        case 4:
            new_writer = u8_buffer_append_ndarray_indent4(writer, &_unicode_buffer_info, 0, obj, true);
            break;
        default:
            ssrjson_unreachable();
    }
    if (unlikely(!new_writer)) {
        PyObject_Free(_unicode_buffer_info.head);
        return NULL;
    }
    // remove trailing comma
    new_writer--;
    usize written_len = new_writer - ssrjson_cast(u8 *, _unicode_buffer_info.head) - write_offset;

    bool resize_success;
    if (ssrjson_consteval(to_bytes_obj)) {
        resize_success = resize_to_fit_pybytes(&_unicode_buffer_info, written_len);
    } else {
        resize_success = resize_to_fit_pyunicode(&_unicode_buffer_info, written_len, 0);
    }
    if (unlikely(!resize_success)) {
        PyObject_Free(_unicode_buffer_info.head);
        return NULL;
    }
    if (ssrjson_consteval(to_bytes_obj)) {
        init_pybytes(_unicode_buffer_info.head, written_len);
    } else {
        init_pyunicode_noinline(_unicode_buffer_info.head, written_len, 0);
    }
    return (PyObject *)_unicode_buffer_info.head;
}

force_inline bool encode_argparse_with_kw(PyObject *const *args, usize npargs, PyObject *kwnames, PyObject **obj_out, PyObject **indent_out) {
    assert(kwnames);
    PyObject *obj, *indent;
    //
    const bool nonstrict_argparse = _NonstrictArgparse;
    bool invalid_arg_checked = _InvalidArgChecked;
    //
    usize nkwargs = PyTuple_GET_SIZE(kwnames);
    usize nargs = npargs + nkwargs;
    assert(nkwargs <= nargs);
    //
    obj = npargs ? args[0] : NULL;
    indent = NULL;
    //
    const char *func_name = "dumps";
    const char *_indent_str = "indent";
    const char *_obj_str = "obj";
    const usize _indent_str_len = strlen(_indent_str);
    const usize _obj_str_len = strlen(_obj_str);
    //
    if (unlikely(npargs > 1)) {
        PyErr_Format(PyExc_TypeError, "%s() takes 1 positional argument but %d were given", func_name, (int)npargs);
        return false;
    }
    for (usize i = 0; i < nkwargs; i++) {
        PyObject *kwname = PyTuple_GET_ITEM(kwnames, i);
        assert(PyUnicode_Check(kwname));
        bool is_ascii;
        const u8 *char_data;
        usize char_count;
        parse_ascii(kwname, &is_ascii, &char_data, &char_count);
        if (likely(is_ascii)) {
            if (char_count == _indent_str_len && memcmp(char_data, _indent_str, _indent_str_len) == 0) {
                assert(!indent);
                indent = args[npargs + i];
                continue;
            } else if (char_count == _obj_str_len && memcmp(char_data, _obj_str, _obj_str_len) == 0) {
                if (unlikely(obj)) {
                    // repeated arg
                    PyErr_Format(PyExc_TypeError, "%s() got multiple values for argument '%s'", func_name, _obj_str);
                    return false;
                }
                obj = args[npargs + i];
                continue;
            }
        }
        // unknown argument
        if (!nonstrict_argparse) {
            handle_unexpected_kw(func_name, kwname);
            return false;
        }
        if (!invalid_arg_checked) {
            invalid_arg_warning();
            invalid_arg_checked = true;
        }
    }
    //
    if (unlikely(!obj)) {
        PyErr_Format(PyExc_TypeError, "%s() missing 1 required positional argument: '%s'", func_name, _obj_str);
        return false;
    }
    *obj_out = obj;
    *indent_out = indent;
    return true;
}

PyObject *SIMD_NAME_MODIFIER(ssrjson_Dumps)(PyObject *self,
                                            PyObject *const *args,
                                            Py_ssize_t nargsf,
                                            PyObject *kwnames) {
    PyObject *ret;
    //
    usize npargs = PyVectorcall_NARGS(nargsf);
    //
    PyObject *obj, *indent;
    if (!kwnames) {
        // positional args except `obj' are not allowed even in nonstrict mode
        indent = NULL;
        if (unlikely(npargs != 1)) {
            if (npargs > 1) {
                PyErr_Format(PyExc_TypeError, "dumps() takes 1 positional argument but %d were given", (int)npargs);
            } else {
                PyErr_SetString(PyExc_TypeError, "dumps() missing 1 required positional argument: 'obj'");
            }
            return NULL;
        }
        obj = args[0];
    } else if (!encode_argparse_with_kw(args, npargs, kwnames, &obj, &indent)) {
        return NULL;
    }
    //
    int indent_int = 0;
    //

    if (indent && indent != Py_None) {
        if (!PyLong_Check(indent)) {
            PyErr_SetString(PyExc_TypeError, "indent must be an integer or None");
            return NULL;
        }
        int _indent = PyLong_AsLong(indent);
        if (_indent != 2 && _indent != 4) {
            PyErr_SetString(PyExc_ValueError, "integer indent must be 2 or 4");
            return NULL;
        }
        indent_int = _indent;
    }

    assert(obj);

    EncodePyTypes obj_type = ssrjson_type_check(obj);

    switch (obj_type) {
        case T_List:
        case T_Dict:
        case T_Tuple: {
            goto dumps_container;
        }
        case T_Unicode: {
            goto dumps_unicode;
        }
        case T_Long: {
            goto dumps_long;
        }
        case T_Bool:
        case T_None: {
            goto dumps_constant;
        }
        case T_Float: {
            goto dumps_float;
        }
        case T_NumpyFloat16:
            return ssrjson_dumps_single_float_from_f64(f16_to_f64(PYOBJ_SCALAR_VALUE(obj, u16)), false);
        case T_NumpyFloat32:
            return ssrjson_dumps_single_float_from_f64((double)PYOBJ_SCALAR_VALUE(obj, float), false);
        case T_NumpyInt8: {
            i16 v = (i16)PYOBJ_SCALAR_VALUE(obj, i8);
            bool sign = v < 0;
            return ssrjson_dumps_to_str_single_long_from_u8(sign ? (u8)(-v) : (u8)v, sign);
        }
        case T_NumpyInt16: {
            i32 v = (i32)PYOBJ_SCALAR_VALUE(obj, i16);
            bool sign = v < 0;
            return ssrjson_dumps_to_str_single_long_from_u16(sign ? (u16)(-v) : (u16)v, sign);
        }
        case T_NumpyInt32: {
            i64 v = (i64)PYOBJ_SCALAR_VALUE(obj, i32);
            bool sign = v < 0;
            return ssrjson_dumps_to_str_single_long_from_u32(sign ? (u32)(-v) : (u32)v, sign);
        }
        case T_NumpyInt64: {
            i64 v = PYOBJ_SCALAR_VALUE(obj, i64);
            bool sign = v < 0;
            return ssrjson_dumps_to_str_single_long_from_u64(sign ? (u64)(-(i64)v) : (u64)v, sign);
        }
        case T_NumpyUint8:
            return ssrjson_dumps_to_str_single_long_from_u8(PYOBJ_SCALAR_VALUE(obj, u8), 0);
        case T_NumpyUint16:
            return ssrjson_dumps_to_str_single_long_from_u16(PYOBJ_SCALAR_VALUE(obj, u16), 0);
        case T_NumpyUint32:
            return ssrjson_dumps_to_str_single_long_from_u32(PYOBJ_SCALAR_VALUE(obj, u32), 0);
        case T_NumpyUint64:
            return ssrjson_dumps_to_str_single_long_from_u64(PYOBJ_SCALAR_VALUE(obj, u64), 0);
        case T_NumpyBool: {
            // Convert numpy bool to Python bool
            int is_true = PyObject_IsTrue(obj);
            if (is_true == -1) return NULL;
            obj = is_true ? Py_True : Py_False;
            obj_type = T_Bool;
            goto dumps_constant;
        }
        case T_NumpyArray:
            return ssrjson_dumps_single_ndarray(obj, indent_int, false);
        default: {
            PyErr_SetString(JSONEncodeError, "Unsupported type to encode");
            return NULL;
        }
    }

dumps_container:;

    switch (indent_int) {
        case 0: {
            ret = _ssrjson_dumps_obj_ascii_indent0(obj);
            break;
        }
        case 2: {
            ret = _ssrjson_dumps_obj_ascii_indent2(obj);
            break;
        }
        case 4: {
            ret = _ssrjson_dumps_obj_ascii_indent4(obj);
            break;
        }
        default: {
            ssrjson_unreachable();
        }
    }

    if (unlikely(!ret)) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(JSONEncodeError, "Failed to decode JSON: unknown error");
        }
    }
#if SSRJSON_GIL_ENABLED
    assert(!ret || ret->ob_refcnt == 1);
#endif
    return ret;

dumps_unicode:;
    return ssrjson_dumps_single_unicode_to_str(obj);
dumps_long:;
    return ssrjson_dumps_single_long(obj, false);
dumps_constant:;
    return ssrjson_dumps_single_constant(obj_type, obj, false);
dumps_float:;
    return ssrjson_dumps_single_float(obj, false);
}

force_inline bool encode_to_bytes_argparse_with_kw(PyObject *const *args, usize npargs, PyObject *kwnames, PyObject **obj_out, PyObject **indent_out, bool *write_cache_out) {
    assert(kwnames);
    PyObject *obj, *indent;
    bool is_write_cache = *write_cache_out;
    //
    usize nkwargs = PyTuple_GET_SIZE(kwnames);
    usize nargs = npargs + nkwargs;
    assert(nkwargs <= nargs);
    //
    obj = npargs ? args[0] : NULL;
    indent = NULL;
    //
    const char *func_name = "dumps_to_bytes";
    const char *_indent_str = "indent";
    const char *_is_write_cache_str = "is_write_cache";
    const char *_obj_str = "obj";
    const usize _indent_str_len = strlen(_indent_str);
    const usize _is_write_cache_str_len = strlen(_is_write_cache_str);
    const usize _obj_str_len = strlen(_obj_str);
    //
    if (unlikely(npargs > 1)) {
        PyErr_Format(PyExc_TypeError, "%s() takes 1 positional argument but %d were given", func_name, (int)npargs);
        return false;
    }
    for (usize i = 0; i < nkwargs; i++) {
        PyObject *kwname = PyTuple_GET_ITEM(kwnames, i);
        assert(PyUnicode_Check(kwname));
        bool is_ascii;
        const u8 *char_data;
        usize char_count;
        parse_ascii(kwname, &is_ascii, &char_data, &char_count);
        if (likely(is_ascii)) {
            if (char_count == _indent_str_len && memcmp(char_data, _indent_str, _indent_str_len) == 0) {
                assert(!indent);
                indent = args[npargs + i];
                continue;
            } else if (char_count == _is_write_cache_str_len && memcmp(char_data, _is_write_cache_str, _is_write_cache_str_len) == 0) {
                PyObject *arg = args[npargs + i];
                bool value_is_true = arg == Py_True;
                bool value_is_false = arg == Py_False;
                if (unlikely(!value_is_true && !value_is_false)) {
                    PyErr_Format(PyExc_TypeError, "%s argument must be True or False", _is_write_cache_str);
                    return false;
                }
                is_write_cache = value_is_true;
                continue;
            } else if (char_count == _obj_str_len && memcmp(char_data, _obj_str, _obj_str_len) == 0) {
                if (unlikely(obj)) {
                    // repeated arg
                    PyErr_Format(PyExc_TypeError, "%s() got multiple values for argument '%s'", func_name, _obj_str);
                    return false;
                }
                obj = args[npargs + i];
                continue;
            }
        }
        // unknown argument
        handle_unexpected_kw(func_name, kwname);
        return false;
    }
    //
    if (unlikely(!obj)) {
        PyErr_Format(PyExc_TypeError, "%s() missing 1 required positional argument: '%s'", func_name, _obj_str);
        return false;
    }
    *obj_out = obj;
    *indent_out = indent;
    *write_cache_out = is_write_cache;
    return true;
}

PyObject *SIMD_NAME_MODIFIER(ssrjson_DumpsToBytes)(PyObject *self,
                                                   PyObject *const *args,
                                                   Py_ssize_t nargsf,
                                                   PyObject *kwnames) {
    PyObject *ret;
    //
    usize npargs = PyVectorcall_NARGS(nargsf);
    //
    PyObject *obj, *indent;
    bool is_write_cache = _WriteUTF8CacheValue;
    if (!kwnames) {
        if (unlikely(npargs != 1)) {
            if (npargs > 1) {
                PyErr_Format(PyExc_TypeError, "dumps_to_bytes() takes 1 positional argument but %d were given", (int)npargs);
            } else {
                PyErr_SetString(PyExc_TypeError, "dumps_to_bytes() missing 1 required positional argument: 'obj'");
            }
            return NULL;
        }
        obj = npargs > 0 ? args[0] : NULL;
        indent = NULL;
    } else if (!encode_to_bytes_argparse_with_kw(args, npargs, kwnames, &obj, &indent, &is_write_cache)) {
        return NULL;
    }
    //
    int indent_int = 0;

    if (indent && indent != Py_None) {
        if (!PyLong_Check(indent)) {
            PyErr_SetString(PyExc_TypeError, "indent must be an integer or None");
            return NULL;
        }
        int _indent = PyLong_AsLong(indent);
        if (_indent != 2 && _indent != 4) {
            PyErr_SetString(PyExc_ValueError, "integer indent must be 2 or 4");
            return NULL;
        }
        indent_int = _indent;
    }

    assert(obj);

    EncodePyTypes obj_type = ssrjson_type_check(obj);

    switch (obj_type) {
        case T_List:
        case T_Dict:
        case T_Tuple: {
            goto dumps_container;
        }
        case T_Unicode: {
            goto dumps_unicode;
        }
        case T_Long: {
            goto dumps_long;
        }
        case T_Bool:
        case T_None: {
            goto dumps_constant;
        }
        case T_Float: {
            goto dumps_float;
        }
        case T_NumpyFloat16:
            return ssrjson_dumps_single_float_from_f64(f16_to_f64(PYOBJ_SCALAR_VALUE(obj, u16)), true);
        case T_NumpyFloat32:
            return ssrjson_dumps_single_float_from_f64((double)PYOBJ_SCALAR_VALUE(obj, float), true);
        case T_NumpyInt8: {
            i16 v = (i16)PYOBJ_SCALAR_VALUE(obj, i8);
            bool sign = v < 0;
            return ssrjson_dumps_to_bytes_single_long_from_u8(sign ? (u8)(-v) : (u8)v, sign);
        }
        case T_NumpyInt16: {
            i32 v = (i32)PYOBJ_SCALAR_VALUE(obj, i16);
            bool sign = v < 0;
            return ssrjson_dumps_to_bytes_single_long_from_u16(sign ? (u16)(-v) : (u16)v, sign);
        }
        case T_NumpyInt32: {
            i64 v = (i64)PYOBJ_SCALAR_VALUE(obj, i32);
            bool sign = v < 0;
            return ssrjson_dumps_to_bytes_single_long_from_u32(sign ? (u32)(-v) : (u32)v, sign);
        }
        case T_NumpyInt64: {
            i64 v = PYOBJ_SCALAR_VALUE(obj, i64);
            bool sign = v < 0;
            return ssrjson_dumps_to_bytes_single_long_from_u64(sign ? (u64)(-(i64)v) : (u64)v, sign);
        }
        case T_NumpyUint8:
            return ssrjson_dumps_to_bytes_single_long_from_u8(PYOBJ_SCALAR_VALUE(obj, u8), 0);
        case T_NumpyUint16:
            return ssrjson_dumps_to_bytes_single_long_from_u16(PYOBJ_SCALAR_VALUE(obj, u16), 0);
        case T_NumpyUint32:
            return ssrjson_dumps_to_bytes_single_long_from_u32(PYOBJ_SCALAR_VALUE(obj, u32), 0);
        case T_NumpyUint64:
            return ssrjson_dumps_to_bytes_single_long_from_u64(PYOBJ_SCALAR_VALUE(obj, u64), 0);
        case T_NumpyBool: {
            // Convert numpy bool to Python bool
            int is_true = PyObject_IsTrue(obj);
            if (is_true == -1) return NULL;
            obj = is_true ? Py_True : Py_False;
            obj_type = T_Bool;
            goto dumps_constant;
        }
        case T_NumpyArray:
            return ssrjson_dumps_single_ndarray(obj, indent_int, true);
        default: {
            PyErr_SetString(JSONEncodeError, "Unsupported type to encode");
            return NULL;
        }
    }

dumps_container:;

    switch (indent_int) {
        case 0: {
            ret = ssrjson_dumps_to_bytes_obj_indent0(obj, is_write_cache);
            break;
        }
        case 2: {
            ret = ssrjson_dumps_to_bytes_obj_indent2(obj, is_write_cache);
            break;
        }
        case 4: {
            ret = ssrjson_dumps_to_bytes_obj_indent4(obj, is_write_cache);
            break;
        }
        default: {
            ssrjson_unreachable();
        }
    }

    if (unlikely(!ret)) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(JSONEncodeError, "Failed to decode JSON: unknown error");
        }
    }

#if SSRJSON_GIL_ENABLED
    assert(!ret || ret->ob_refcnt == 1);
#endif

    return ret;

dumps_unicode:;
    return ssrjson_dumps_single_unicode_to_bytes(obj, is_write_cache);
dumps_long:;
    return ssrjson_dumps_single_long(obj, true);
dumps_constant:;
    return ssrjson_dumps_single_constant(obj_type, obj, true);
dumps_float:;
    return ssrjson_dumps_single_float(obj, true);
}

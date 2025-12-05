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
force_inline PyObject *_ssrjson_dumps_single_unicode(PyObject *unicode, bool to_bytes_obj, bool is_write_cache) {
    EncodeUnicodeWriter writer;
    EncodeUnicodeBufferInfo _unicode_buffer_info; //, new_unicode_buffer_info;
    _unicode_buffer_info.head = PyObject_Malloc(SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    RETURN_ON_UNLIKELY_ERR(!_unicode_buffer_info.head);
    //
    bool compact = SSRJSON_CAST(PyASCIIObject *, unicode)->state.compact;
    assert(compact);
    usize len;
    int unicode_kind;
    bool is_ascii;
    //
    usize write_offset;
    if (to_bytes_obj) {
        write_offset = PYBYTES_START_OFFSET;
    } else {
        len = (usize)PyUnicode_GET_LENGTH(unicode);
        unicode_kind = PyUnicode_KIND(unicode);
        is_ascii = PyUnicode_IS_ASCII(unicode);
        if (is_ascii) {
            write_offset = sizeof(PyASCIIObject);
        } else {
            write_offset = sizeof(PyCompactUnicodeObject);
        }
    }
    WRITER_AS_U8(writer) = SSRJSON_CAST(u8 *, _unicode_buffer_info.head) + write_offset;
    _unicode_buffer_info.end = SSRJSON_CAST(u8 *, _unicode_buffer_info.head) + SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE;
    //
    bool success;
    if (to_bytes_obj) {
        success = bytes_buffer_append_str_indent0(unicode, &WRITER_AS_U8(writer), &_unicode_buffer_info, 0, true, is_write_cache);
        WRITER_AS_U8(writer)
        --;
    } else {
        switch (unicode_kind) {
            // pass `is_in_obj = true` to avoid unwanted indent check
            case 1: {
                const u8 *src = is_ascii ? PYUNICODE_ASCII_START(unicode) : PYUNICODE_UCS1_START(unicode);
                success = STR_WRITER_NOINDENT_IMPL(u8, u8)(src, len, &WRITER_AS_U8(writer), &_unicode_buffer_info, 0, true);
                WRITER_AS_U8(writer)
                --;
                break;
            }
            case 2: {
                const u16 *src = PYUNICODE_UCS2_START(unicode);
                success = STR_WRITER_NOINDENT_IMPL(u16, u16)(src, len, &WRITER_AS_U16(writer), &_unicode_buffer_info, 0, true);
                WRITER_AS_U16(writer)
                --;
                break;
            }
            case 4: {
                const u32 *src = PYUNICODE_UCS4_START(unicode);
                success = STR_WRITER_NOINDENT_IMPL(u32, u32)(src, len, &WRITER_AS_U32(writer), &_unicode_buffer_info, 0, true);
                WRITER_AS_U32(writer)
                --;
                break;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
    }
    if (unlikely(!success)) {
        // realloc failed when encoding, the original buffer is still valid
        PyObject_Free(_unicode_buffer_info.head);
        return NULL;
    }
    usize written_len = (uintptr_t)writer - (uintptr_t)_unicode_buffer_info.head - write_offset;
    if (!to_bytes_obj) {
        written_len /= unicode_kind;
    }
    assert(written_len >= 2);
    bool resize_success;
    if (to_bytes_obj) {
        resize_success = resize_to_fit_pybytes(&_unicode_buffer_info, written_len);
    } else {
        resize_success = resize_to_fit_pyunicode(&_unicode_buffer_info, written_len, is_ascii ? 0 : unicode_kind);
    }
    if (unlikely(!resize_success)) {
        PyObject_Free(_unicode_buffer_info.head);
        return NULL;
    }
    if (to_bytes_obj) {
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
#undef COMPILE_SIMD_BITS

force_inline PyObject *ssrjson_dumps_single_long(PyObject *val, bool to_bytes_obj) {
    PyObject *ret;
    if (pylong_is_zero(val)) {
        if (to_bytes_obj) {
            ret = PyObject_Malloc(PYBYTES_START_OFFSET + 1 + 1);
            RETURN_ON_UNLIKELY_ERR(!ret);
            init_pybytes(ret, 1);
            PyBytesObject *b = SSRJSON_CAST(PyBytesObject *, ret);
            b->ob_sval[0] = '0';
            b->ob_sval[1] = 0;
        } else {
            ret = create_empty_unicode(1, 0);
            RETURN_ON_UNLIKELY_ERR(!ret);
            u8 *writer = (u8 *)(((PyASCIIObject *)ret) + 1);
            writer[0] = '0';
            writer[1] = 0;
        }
    } else {
        u64 v;
        usize sign;
        if (pylong_is_unsigned(val)) {
            bool _c = pylong_value_unsigned(val, &v);
            if (unlikely(!_c)) {
                PyErr_SetString(JSONEncodeError, "convert value to unsigned long long failed");
                return NULL;
            }
            sign = 0;
        } else {
            i64 v2;
            bool _c = pylong_value_signed(val, &v2);
            if (unlikely(!_c)) {
                PyErr_SetString(JSONEncodeError, "convert value to long long failed");
                return NULL;
            }
            assert(v2 <= 0);
            v = -v2;
            sign = 1;
        }
        u8 buffer[64];
        if (sign) *buffer = '-';
        u8 *buffer_end = write_u64(v, buffer + sign);
        usize string_size = buffer_end - buffer;
        u8 *writer;
        if (to_bytes_obj) {
            ret = PyObject_Malloc(PYBYTES_START_OFFSET + string_size + 1);
            RETURN_ON_UNLIKELY_ERR(!ret);
            init_pybytes(ret, string_size);
            writer = SSRJSON_CAST(u8 *, SSRJSON_CAST(PyBytesObject *, ret)->ob_sval);
        } else {
            ret = create_empty_unicode(string_size, 0);
            RETURN_ON_UNLIKELY_ERR(!ret);
            writer = (u8 *)(((PyASCIIObject *)ret) + 1);
        }
        ssrjson_memcpy(writer, buffer, string_size);
        writer[string_size] = 0;
    }
    return ret;
}

force_inline PyObject *ssrjson_dumps_single_float(PyObject *val, bool to_bytes_obj) {
    u8 buffer[32];
    double v = PyFloat_AS_DOUBLE(val);
    u64 *raw = (u64 *)&v;
    u8 *buffer_end = dragonbox_to_chars_n(f64_from_raw(*raw), buffer);
    usize size = buffer_end - buffer;
    assert(size < 64);
    PyObject *unicode;
    if (to_bytes_obj) {
        unicode = PyObject_Malloc(PYBYTES_START_OFFSET + size + 1);
    } else {
        unicode = create_empty_unicode(size, 0);
    }
    if (unlikely(!unicode)) return NULL;
    if (to_bytes_obj) {
        init_pybytes(unicode, size);
    }
    char *write_pos;
    if (to_bytes_obj) {
        write_pos = SSRJSON_CAST(PyBytesObject *, unicode)->ob_sval;
    } else {
        write_pos = (char *)(((PyASCIIObject *)unicode) + 1);
    }
    ssrjson_memcpy((void *)write_pos, buffer, size);
    write_pos[size] = 0;
    return unicode;
}

force_inline PyObject *ssrjson_dumps_single_constant(ssrjson_py_types py_type, PyObject *obj, bool to_bytes_obj) {
    PyObject *ret;
    switch (py_type) {
        case T_Bool: {
            if (obj == Py_False) {
                u8 *writer;
                if (to_bytes_obj) {
                    ret = PyObject_Malloc(PYBYTES_START_OFFSET + 5 + 1);
                    RETURN_ON_UNLIKELY_ERR(!ret);
                    init_pybytes(ret, 5);
                    writer = SSRJSON_CAST(u8 *, SSRJSON_CAST(PyBytesObject *, ret)->ob_sval);
                } else {
                    ret = create_empty_unicode(5, 0);
                    RETURN_ON_UNLIKELY_ERR(!ret);
                    writer = (u8 *)(((PyASCIIObject *)ret) + 1);
                }
                strcpy((char *)writer, "false");
            } else {
                u8 *writer;
                if (to_bytes_obj) {
                    ret = PyObject_Malloc(PYBYTES_START_OFFSET + 4 + 1);
                    RETURN_ON_UNLIKELY_ERR(!ret);
                    init_pybytes(ret, 4);
                    writer = SSRJSON_CAST(u8 *, SSRJSON_CAST(PyBytesObject *, ret)->ob_sval);
                } else {
                    ret = create_empty_unicode(4, 0);
                    RETURN_ON_UNLIKELY_ERR(!ret);
                    writer = (u8 *)(((PyASCIIObject *)ret) + 1);
                }
                strcpy((char *)writer, "true");
            }
            break;
        }
        case T_None: {
            u8 *writer;
            if (to_bytes_obj) {
                ret = PyObject_Malloc(PYBYTES_START_OFFSET + 4 + 1);
                RETURN_ON_UNLIKELY_ERR(!ret);
                init_pybytes(ret, 4);
                writer = SSRJSON_CAST(u8 *, SSRJSON_CAST(PyBytesObject *, ret)->ob_sval);
            } else {
                ret = create_empty_unicode(4, 0);
                RETURN_ON_UNLIKELY_ERR(!ret);
                writer = (u8 *)(((PyASCIIObject *)ret) + 1);
            }
            strcpy((char *)writer, "null");
            break;
        }
        default: {
            ret = NULL;
            SSRJSON_UNREACHABLE();
            break;
        }
    }
    return ret;
}

extern int ssrjson_invalid_arg_checked;
extern int ssrjson_nonstrict_argparse;
extern int ssrjson_write_utf8_cache_value;

force_inline void invalid_arg_warning(void) {
    fprintf(stderr, "Warning: some options are not supported in this version of ssrjson\n");
    ssrjson_invalid_arg_checked = 1;
}

force_inline bool encode_argparse_with_kw(PyObject *const *args, usize npargs, PyObject *kwnames, PyObject **obj_out, PyObject **indent_out) {
    assert(kwnames);
    PyObject *obj, *indent;
    //
    const bool nonstrict_argparse = ssrjson_nonstrict_argparse;
    bool invalid_arg_checked = ssrjson_invalid_arg_checked;
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

/* Entrance for python code. */
// PyObject *SIMD_NAME_MODIFIER(ssrjson_Encode)(PyObject *self, PyObject *args, PyObject *kwargs) {
PyObject *SIMD_NAME_MODIFIER(ssrjson_Encode)(PyObject *self,
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

    ssrjson_py_types obj_type = ssrjson_type_check(obj);

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
            SSRJSON_UNREACHABLE();
        }
    }

    if (unlikely(!ret)) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(JSONEncodeError, "Failed to decode JSON: unknown error");
        }
    }

    assert(!ret || ret->ob_refcnt == 1);

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

PyObject *SIMD_NAME_MODIFIER(ssrjson_EncodeToBytes)(PyObject *self,
                                                    PyObject *const *args,
                                                    Py_ssize_t nargsf,
                                                    PyObject *kwnames) {
    PyObject *ret;
    //
    usize npargs = PyVectorcall_NARGS(nargsf);
    //
    PyObject *obj, *indent;
    bool is_write_cache = ssrjson_write_utf8_cache_value;
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

    ssrjson_py_types obj_type = ssrjson_type_check(obj);

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
            SSRJSON_UNREACHABLE();
        }
    }

    if (unlikely(!ret)) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(JSONEncodeError, "Failed to decode JSON: unknown error");
        }
    }

    assert(!ret || ret->ob_refcnt == 1);

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

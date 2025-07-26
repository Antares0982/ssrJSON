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
force_inline PyObject *ssrjson_dumps_single_unicode(PyObject *unicode, bool to_bytes_obj) {
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
    writer.writer_u8 = SSRJSON_CAST(u8 *, _unicode_buffer_info.head) + write_offset;
    _unicode_buffer_info.end = SSRJSON_CAST(u8 *, _unicode_buffer_info.head) + SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE;
    //
    bool success;
    if (to_bytes_obj) {
        success = bytes_buffer_append_str_indent0(unicode, &writer, &_unicode_buffer_info, 0, true);
        writer.writer_u8--;
    } else {
        switch (unicode_kind) {
            // pass `is_in_obj = true` to avoid unwanted indent check
            case 1: {
                const u8 *src = is_ascii ? PYUNICODE_ASCII_START(unicode) : PYUNICODE_UCS1_START(unicode);
                success = STR_WRITER_NOINDENT_IMPL(u8, u8)(src, len, &writer.writer_u8, &_unicode_buffer_info, 0, true);
                writer.writer_u8--;
                break;
            }
            case 2: {
                const u16 *src = PYUNICODE_UCS2_START(unicode);
                success = STR_WRITER_NOINDENT_IMPL(u16, u16)(src, len, &writer.writer_u16, &_unicode_buffer_info, 0, true);
                writer.writer_u16--;
                break;
            }
            case 4: {
                const u32 *src = PYUNICODE_UCS4_START(unicode);
                success = STR_WRITER_NOINDENT_IMPL(u32, u32)(src, len, &writer.writer_u32, &_unicode_buffer_info, 0, true);
                writer.writer_u32--;
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
    usize written_len = (uintptr_t)writer.writer_u8 - (uintptr_t)_unicode_buffer_info.head - write_offset;
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
            Py_UNREACHABLE();
            break;
        }
    }
    return ret;
}

static int invalid_arg_checked = 0;

/* Entrance for python code. */
PyObject *SIMD_NAME_MODIFIER(ssrjson_Encode)(PyObject *self, PyObject *args, PyObject *kwargs) {
    PyObject *obj;
    PyObject *ret;
    //
    PyObject *indent = NULL, *skipkeys = NULL, *ensure_ascii = NULL, *check_circular = NULL, *allow_nan = NULL, *cls = NULL, *separators = NULL, *default_ = NULL, *sort_keys = NULL;
    static const char *kwlist[] = {"obj", "indent", "skipkeys", "ensure_ascii", "check_circular", "allow_nan", "cls", "separators", "default", "sort_keys", NULL};
    //
    int indent_int = 0;
    //
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOOOOOOOO", (char **)kwlist, &obj, &indent, &skipkeys, &ensure_ascii, &check_circular, &allow_nan, &cls, &separators, &default_, &sort_keys)) {
        goto fail;
    }

    if (!invalid_arg_checked && (skipkeys || ensure_ascii || check_circular || allow_nan || cls || separators || default_ || sort_keys)) {
        fprintf(stderr, "Warning: some options are not supported in this version of ssrjson\n");
        invalid_arg_checked = 1;
    }

    if (indent) {
        if (indent != Py_None && !PyLong_Check(indent)) {
            PyErr_SetString(PyExc_TypeError, "indent must be an integer");
            goto fail;
        }
        if (indent != Py_None) {
            int _indent = PyLong_AsLong(indent);
            if (_indent < 0 || _indent > 4 || (_indent / 2) * 2 != _indent) {
                PyErr_SetString(PyExc_ValueError, "indent must be 0, 2, or 4");
                goto fail;
            }
            indent_int = _indent;
        }
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
        // case T_False:
        // case T_True:
        case T_None: {
            goto dumps_constant;
        }
        case T_Float: {
            goto dumps_float;
        }
        default: {
            PyErr_SetString(JSONEncodeError, "Unsupported type to encode");
            goto fail;
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

    goto success;

dumps_unicode:;
    return ssrjson_dumps_single_unicode(obj, false);
dumps_long:;
    return ssrjson_dumps_single_long(obj, false);
dumps_constant:;
    return ssrjson_dumps_single_constant(obj_type, obj, false);
dumps_float:;
    return ssrjson_dumps_single_float(obj, false);
success:;
    return ret;
fail:;
    return NULL;
}

PyObject *SIMD_NAME_MODIFIER(ssrjson_EncodeToBytes)(PyObject *self, PyObject *args, PyObject *kwargs) {
    PyObject *obj;
    PyObject *ret;
    //
    PyObject *indent = NULL;
    static const char *kwlist[] = {"obj", "indent", NULL};
    //
    int indent_int = 0;
    //
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", (char **)kwlist, &obj, &indent)) {
        goto fail;
    }

    if (indent) {
        if (indent != Py_None && !PyLong_Check(indent)) {
            PyErr_SetString(PyExc_TypeError, "indent must be an integer");
            goto fail;
        }
        if (indent != Py_None) {
            int _indent = PyLong_AsLong(indent);
            if (_indent < 0 || _indent > 4 || (_indent / 2) * 2 != _indent) {
                PyErr_SetString(PyExc_ValueError, "indent must be 0, 2, or 4");
                goto fail;
            }
            indent_int = _indent;
        }
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
            goto fail;
        }
    }

dumps_container:;

    switch (indent_int) {
        case 0: {
            ret = ssrjson_dumps_to_bytes_obj_indent0(obj);
            break;
        }
        case 2: {
            ret = ssrjson_dumps_to_bytes_obj_indent2(obj);
            break;
        }
        case 4: {
            ret = ssrjson_dumps_to_bytes_obj_indent4(obj);
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

    goto success;

dumps_unicode:;
    return ssrjson_dumps_single_unicode(obj, true);
dumps_long:;
    return ssrjson_dumps_single_long(obj, true);
dumps_constant:;
    return ssrjson_dumps_single_constant(obj_type, obj, true);
dumps_float:;
    return ssrjson_dumps_single_float(obj, true);
success:;
    return ret;
fail:;
    return NULL;
}

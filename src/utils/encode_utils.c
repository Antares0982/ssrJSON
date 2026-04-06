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
#define ENCODE_RESERVE_DEBUG 0
#include "encode/encode_shared.h"


#define RESERVE_MAX ((~(usize)PY_SSIZE_T_MAX) >> 1)
static_assert((ssrjson_cast(usize, RESERVE_MAX) & (ssrjson_cast(usize, RESERVE_MAX) - 1)) == 0, "");

EncodeUnicodeBufferInfo _unicode_buffer_reserve(EncodeUnicodeBufferInfo unicode_buffer_info, usize target_size) {
#if ENCODE_RESERVE_DEBUG
    void *new_ptr = PyObject_Realloc(unicode_buffer_info.head, target_size);
    if (unlikely(!new_ptr)) {
        PyErr_NoMemory();
        unicode_buffer_info.head = unicode_buffer_info.end = NULL;
        return unicode_buffer_info;
    }
    unicode_buffer_info.head = new_ptr;
    unicode_buffer_info.end = ssrjson_cast(u8 *, unicode_buffer_info.head) + target_size;
    return unicode_buffer_info;
#else
    usize u8len = ssrjson_cast(uintptr_t, unicode_buffer_info.end) - ssrjson_cast(uintptr_t, unicode_buffer_info.head);
    assert((u8len & (u8len - 1)) == 0);
    while (target_size > u8len) {
        if (u8len & RESERVE_MAX) {
            PyErr_NoMemory();
            unicode_buffer_info.head = unicode_buffer_info.end = NULL;
            return unicode_buffer_info;
        }
        u8len = (u8len << 1);
    }
    void *new_ptr = PyObject_Realloc(unicode_buffer_info.head, u8len);
    if (unlikely(!new_ptr)) {
        PyErr_NoMemory();
        unicode_buffer_info.head = unicode_buffer_info.end = NULL;
        return unicode_buffer_info;
    }
    unicode_buffer_info.head = new_ptr;
    unicode_buffer_info.end = ssrjson_cast(u8 *, unicode_buffer_info.head) + u8len;
    return unicode_buffer_info;
#endif
}

bool resize_to_fit_pyunicode(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t len, int ucs_type) {
    Py_ssize_t char_size = ucs_type ? ucs_type : 1;
    Py_ssize_t struct_size = ucs_type ? sizeof(PyCompactUnicodeObject) : sizeof(PyASCIIObject);
    assert(len <= ((PY_SSIZE_T_MAX - struct_size) / char_size - 1));
    // Resizes to a smaller size. It *should* always be successful
    void *new_ptr = PyObject_Realloc(unicode_buffer_info->head, struct_size + (len + 1) * char_size);
    if (unlikely(!new_ptr)) {
        PyErr_NoMemory();
        return false;
    }
    unicode_buffer_info->head = new_ptr;
    return true;
}

EncodePyTypes slow_type_check(PyTypeObject *type) {
    // Standard Python type checks first
    if (PyType_FastSubclass(type, Py_TPFLAGS_DICT_SUBCLASS)) {
        return T_Dict;
    } else if (PyType_FastSubclass(type, Py_TPFLAGS_LIST_SUBCLASS)) {
        return T_List;
    } else if (PyType_FastSubclass(type, Py_TPFLAGS_TUPLE_SUBCLASS)) {
        return T_Tuple;
    } else if (PyType_FastSubclass(type, Py_TPFLAGS_UNICODE_SUBCLASS)) {
        return T_UnicodeNonCompact;
    } else if (PyType_FastSubclass(type, Py_TPFLAGS_LONG_SUBCLASS)) {
        return T_Long;
    } else if (PyType_IsSubtype(type, &PyFloat_Type)) {
        return T_Float;
    }

    if (_NumpyTypes.ndarray) {
        if (PyType_IsSubtype(type, _NumpyTypes.ndarray)) return T_NumpyArray;
        /* np.float64 is not checked here: it is a subclass of Python float,
         * already caught by PyType_IsSubtype(type, &PyFloat_Type) above. */
        if (type == _NumpyTypes.float32) return T_NumpyFloat32;
        if (type == _NumpyTypes.int64) return T_NumpyInt64;
        if (type == _NumpyTypes.int32) return T_NumpyInt32;
        if (type == _NumpyTypes.uint64) return T_NumpyUint64;
        if (type == _NumpyTypes.uint32) return T_NumpyUint32;
        if (type == _NumpyTypes.bool_) return T_NumpyBool;
        if (type == _NumpyTypes.float16) return T_NumpyFloat16;
        if (type == _NumpyTypes.int16) return T_NumpyInt16;
        if (type == _NumpyTypes.int8) return T_NumpyInt8;
        if (type == _NumpyTypes.uint16) return T_NumpyUint16;
        if (type == _NumpyTypes.uint8) return T_NumpyUint8;
    }

    return T_Unknown;
}

/*==============================================================================
 * Global Vars
 *============================================================================*/
#if SSRJSON_GIL_ENABLED
EncodeCtnWithIndex _EncodeCtnBuffer[SSRJSON_ENCODE_MAX_RECURSION];
#endif

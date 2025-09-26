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

#include "encode/encode_shared.h"


#define RESERVE_MAX ((~(usize)PY_SSIZE_T_MAX) >> 1)
static_assert((SSRJSON_CAST(usize, RESERVE_MAX) & (SSRJSON_CAST(usize, RESERVE_MAX) - 1)) == 0, "");

bool _unicode_buffer_reserve(EncodeUnicodeBufferInfo *unicode_buffer_info, usize target_size) {
    usize u8len = SSRJSON_CAST(uintptr_t, unicode_buffer_info->end) - SSRJSON_CAST(uintptr_t, unicode_buffer_info->head);
    assert((u8len & (u8len - 1)) == 0);
    while (target_size > u8len) {
        if (u8len & RESERVE_MAX) {
            PyErr_NoMemory();
            return false;
        }
        u8len = (u8len << 1);
    }
    void *new_ptr = PyObject_Realloc(unicode_buffer_info->head, u8len);
    if (unlikely(!new_ptr)) {
        assert(PyErr_Occurred());
        return false;
    }
    unicode_buffer_info->head = new_ptr;
    unicode_buffer_info->end = SSRJSON_CAST(u8 *, unicode_buffer_info->head) + u8len;
    return true;
}

bool resize_to_fit_pyunicode(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t len, int ucs_type) {
    Py_ssize_t char_size = ucs_type ? ucs_type : 1;
    Py_ssize_t struct_size = ucs_type ? sizeof(PyCompactUnicodeObject) : sizeof(PyASCIIObject);
    assert(len <= ((PY_SSIZE_T_MAX - struct_size) / char_size - 1));
    // Resizes to a smaller size. It *should* always be successful
    void *new_ptr = PyObject_Realloc(unicode_buffer_info->head, struct_size + (len + 1) * char_size);
    if (unlikely(!new_ptr)) {
        return false;
    }
    unicode_buffer_info->head = new_ptr;
    return true;
}

ssrjson_py_types slow_type_check(PyTypeObject *type) {
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
    return T_Unknown;
}

#if !defined(Py_GIL_DISABLED)
EncodeCtnWithIndex _EncodeCtnBuffer[SSRJSON_ENCODE_MAX_RECURSION];
#endif

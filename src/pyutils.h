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

#ifndef SSRJSON_PYUTILS_H
#define SSRJSON_PYUTILS_H

#include "ssrjson.h"
#include "utils/unicode.h"
#if !SSRJSON_GIL_ENABLED
#    include <stdatomic.h>
#endif

#define ASCII_OFFSET sizeof(PyASCIIObject)
#define UNICODE_OFFSET sizeof(PyCompactUnicodeObject)

// _PyUnicode_CheckConsistency is hidden in Python 3.13
#if PY_MINOR_VERSION >= 13
extern int _PyUnicode_CheckConsistency(PyObject *op, int check_content);
#endif

#if PY_MINOR_VERSION >= 13
// these are hidden in Python 3.13
#    if PY_MINOR_VERSION == 13
extern Py_hash_t _Py_HashBytes(const void *, Py_ssize_t);
#    endif // PY_MINOR_VERSION == 13
extern int _PyDict_SetItem_KnownHash_LockHeld(PyObject *mp, PyObject *key, PyObject *item, Py_hash_t hash);
#    define _PyDict_SetItem_KnownHash _PyDict_SetItem_KnownHash_LockHeld
#endif // PY_MINOR_VERSION >= 13

#if PY_MINOR_VERSION >= 13
// _PyNone_Type is hidden in Python 3.13
extern PyTypeObject *PyNone_Type;
#else
#    define PyNone_Type &_PyNone_Type
#endif
#if PY_MINOR_VERSION >= 13
extern PyTypeObject *PyNone_Type;
#endif

// Initialize a PyUnicode object with the given size and kind.
force_inline void init_pyunicode(void *head, Py_ssize_t size, int kind) {
    u8 *const u8head = ssrjson_cast(u8 *, head);
    PyCompactUnicodeObject *unicode = ssrjson_pycompactunicode_cast(head);
    PyASCIIObject *ascii = ssrjson_pyascii_cast(head);
    PyObject_Init(ssrjson_pyobj_cast(head), &PyUnicode_Type);
    void *data = ssrjson_cast(void *, u8head + (kind ? UNICODE_OFFSET : ASCII_OFFSET));
    //
    ascii->length = size;
    ascii->hash = -1;
    ascii->state.interned = 0;
    ascii->state.kind = kind ? kind : 1;
    ascii->state.compact = 1;
    ascii->state.ascii = kind ? 0 : 1;

#if PY_MINOR_VERSION >= 12
    // statically_allocated appears in 3.12
    ascii->state.statically_allocated = 0;
#else
    bool is_sharing = false;
    // `ready` is dropped in 3.12
    ascii->state.ready = 1;
#endif

    if (kind <= 1) {
        ((u8 *)data)[size] = 0;
    } else if (kind == 2) {
        ((u16 *)data)[size] = 0;
#if PY_MINOR_VERSION < 12
        is_sharing = sizeof(wchar_t) == 2;
#endif
    } else {
        assert(kind == 4);
        ((u32 *)data)[size] = 0;
#if PY_MINOR_VERSION < 12
        is_sharing = sizeof(wchar_t) == 4;
#endif
    }
    if (kind) {
        unicode->utf8 = NULL;
        unicode->utf8_length = 0;
    }
#if PY_MINOR_VERSION < 12
    if (kind > 1) {
        if (is_sharing) {
            unicode->wstr_length = size;
            ascii->wstr = (wchar_t *)data;
        } else {
            unicode->wstr_length = 0;
            ascii->wstr = NULL;
        }
    } else {
        ascii->wstr = NULL;
        if (kind) unicode->wstr_length = 0;
    }
#endif
    assert(_PyUnicode_CheckConsistency((PyObject *)unicode, 0));
#if SSRJSON_GIL_ENABLED
    assert(ascii->ob_base.ob_refcnt == 1);
#endif
}

// Create an empty unicode object with the given size and kind, like PyUnicode_New.
// This is a force_inline function to avoid the overhead of function calls in performance-critical paths.
force_inline PyObject *create_empty_unicode(usize size, int kind) {
    if (unlikely(!size)) return PyUnicode_New(0, 0);
    assert(kind == 0 || kind == 1 || kind == 2 || kind == 4);
    usize offset = kind ? sizeof(PyCompactUnicodeObject) : sizeof(PyASCIIObject);
    usize tpsize = kind ? kind : 1;
    PyObject *str = PyObject_Malloc(offset + (size + 1) * tpsize);
    return_if_no_memory(str);
    init_pyunicode(str, size, kind);
    return str;
}

// Calculate the hash for a PyUnicodeObject based on the given unicode string and its real length.
force_inline void make_hash(PyASCIIObject *ascii, const void *unicode_str, size_t real_len) {
#if PY_MINOR_VERSION >= 14
    ascii->hash = Py_HashBuffer(unicode_str, real_len);
#else
    ascii->hash = _Py_HashBytes(unicode_str, real_len);
#endif
}

force_noinline void init_pyunicode_noinline(void *head, Py_ssize_t size, int kind);

force_inline void *pymem_malloc_wrapped(usize size) {
    void *ptr = PyMem_Malloc(size);
    if (unlikely(!ptr)) {
        PyErr_NoMemory();
    }
    return ptr;
}

force_inline void *pymem_realloc_wrapped(void *ptr, usize size) {
    void *new_ptr = PyMem_Realloc(ptr, size);
    if (unlikely(!new_ptr)) {
        PyErr_NoMemory();
    }
    return new_ptr;
}

force_inline void pymem_free_wrapped(void *ptr) {
    PyMem_Free(ptr);
}

force_inline const u8 *pyunicode_get_utf8_cache(PyObject *unicode) {
#if !SSRJSON_GIL_ENABLED
    return atomic_load_explicit((const _Atomic(void *) *)&ssrjson_pycompactunicode_cast(unicode)->utf8, memory_order_acquire);
#else
    return (const u8 *)ssrjson_pycompactunicode_cast(unicode)->utf8;
#endif
}

force_inline void get_utf8_cache(PyObject *unicode, const u8 **utf8_cache_out, usize *utf8_length_out) {
    assert(!ssrjson_pyascii_cast(unicode)->state.ascii);
    *utf8_cache_out = (const u8 *)pyunicode_get_utf8_cache(unicode);
    *utf8_length_out = (usize)ssrjson_pycompactunicode_cast(unicode)->utf8_length;
}

force_inline void set_cache(PyObject *str, const u8 **utf8_cache_addr, usize utf8_length) {
#if !SSRJSON_GIL_ENABLED
    ssrjson_pycompactunicode_cast(str)->utf8_length = (Py_ssize_t)utf8_length;
    u8 *expected = NULL;
    if (!atomic_compare_exchange_strong_explicit(
                (_Atomic(u8 *) *)&ssrjson_pycompactunicode_cast(str)->utf8,
                &expected,
                (u8 *)(*utf8_cache_addr),
                memory_order_release,
                memory_order_relaxed)) {
        // already has an UTF-8 cache, free the allocated one by us
        pymem_free_wrapped((void *)*utf8_cache_addr);
        *utf8_cache_addr = expected;
    }
#else
    ssrjson_pycompactunicode_cast(str)->utf8 = (void *)*utf8_cache_addr;
    ssrjson_pycompactunicode_cast(str)->utf8_length = (Py_ssize_t)utf8_length;
#endif
}

PyObject *make_unicode_from_raw_ucs4(void *raw_buffer, usize u8size, usize u16size, usize totalsize, bool do_hash);
PyObject *make_unicode_from_raw_ucs2(void *raw_buffer, usize u8size, usize totalsize, bool do_hash);
PyObject *make_unicode_from_raw_ucs1(void *raw_buffer, usize size, bool do_hash);
PyObject *make_unicode_from_raw_ascii(void *raw_buffer, usize size, bool do_hash);
PyObject *make_unicode_down_ucs2_u8(void *raw_buffer, usize size, bool do_hash, bool is_ascii);
PyObject *make_unicode_down_ucs4_u8(void *raw_buffer, usize size, bool do_hash, bool is_ascii);
PyObject *make_unicode_down_ucs4_ucs2(void *raw_buffer, usize size, bool do_hash);

void handle_unexpected_kw(const char *func_name, PyObject *kwname);

/* Parse an ASCII PyUnicodeObject. 
 * If the object is not ASCII, `char_data_out` and `char_count_out` are undefined.
 * Otherwise, `char_data_out` points to the character data, and `char_count_out` is the length of the string.
 */
force_inline void parse_ascii(PyObject *unicode, bool *is_ascii_out, const u8 **char_data_out, usize *char_count_out) {
    assert(PyUnicode_Check(unicode));
    bool is_ascii, is_compact;
    const u8 *char_data;
    usize char_count;
    is_ascii = PyUnicode_IS_ASCII(unicode);
    if (likely(is_ascii)) {
        is_compact = ssrjson_pyascii_cast(unicode)->state.compact;
        char_count = (usize)PyUnicode_GET_LENGTH(unicode);
        if (likely(is_compact)) {
            char_data = ssrjson_pyunicode_ascii_start(unicode);
        } else {
            char_data = ssrjson_pyunicode_cast(unicode)->data.any;
        }
    }

    *is_ascii_out = is_ascii;
    *char_data_out = char_data;
    *char_count_out = char_count;
}

extern ssrjson_align(64) u64 _PyFastType[8];

/* np.float64 is omitted: it has always been a subclass of Python float,
 * so PyType_IsSubtype(type, &PyFloat_Type) in slow_type_check() already
 * handles it via the T_Float path. */
typedef struct {
    PyTypeObject *ndarray;
    PyTypeObject *float32;
    PyTypeObject *int64;
    PyTypeObject *int32;
    PyTypeObject *uint64;
    PyTypeObject *uint32;
    PyTypeObject *bool_;
    PyTypeObject *float16;
    PyTypeObject *int16;
    PyTypeObject *int8;
    PyTypeObject *uint16;
    PyTypeObject *uint8;
} ssrjson_align(64) NumpyTypes;

#define NUMPY_TYPES_COUNT 12

extern NumpyTypes _NumpyTypes;

#endif // SSRJSON_PYUTILS_H

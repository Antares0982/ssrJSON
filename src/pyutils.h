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
#if defined(Py_GIL_DISABLED)
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

// Initialize a PyUnicode object with the given size and kind.
force_inline void init_pyunicode(void *head, Py_ssize_t size, int kind) {
    u8 *const u8head = SSRJSON_CAST(u8 *, head);
    PyCompactUnicodeObject *unicode = SSRJSON_CAST(PyCompactUnicodeObject *, head);
    PyASCIIObject *ascii = SSRJSON_CAST(PyASCIIObject *, head);
    PyObject_Init(SSRJSON_CAST(PyObject *, head), &PyUnicode_Type);
    void *data = SSRJSON_CAST(void *, u8head + (kind ? UNICODE_OFFSET : ASCII_OFFSET));
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
    // ready is dropped in 3.12
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
    assert(ascii->ob_base.ob_refcnt == 1);
}

// Create an empty unicode object with the given size and kind, like PyUnicode_New.
// This is a force_inline function to avoid the overhead of function calls in performance-critical paths.
force_inline PyObject *create_empty_unicode(usize size, int kind) {
    if (unlikely(!size)) return PyUnicode_New(0, 0x7f);
    assert(kind == 0 || kind == 1 || kind == 2 || kind == 4);
    usize offset = kind ? sizeof(PyCompactUnicodeObject) : sizeof(PyASCIIObject);
    usize tpsize = kind ? kind : 1;
    PyObject *str = PyObject_Malloc(offset + (size + 1) * tpsize);
    if (likely(str)) {
        init_pyunicode(str, size, kind);
    }
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

force_inline const u8 *pyunicode_get_utf8_cache(PyObject *unicode) {
#if defined(Py_GIL_DISABLED)
    return __c11_atomic_load((const _Atomic(void *) *)&SSRJSON_CAST(PyCompactUnicodeObject *, unicode)->utf8, memory_order_acquire);
#else
    return (const u8 *)SSRJSON_PYCOMPACTUNICODE_CAST(unicode)->utf8;
#endif
}

force_inline void get_utf8_cache(PyObject *unicode, const u8 **utf8_cache_out, usize *utf8_length_out) {
    assert(SSRJSON_CAST(PyASCIIObject *, unicode)->state.compact);
    assert(!SSRJSON_CAST(PyASCIIObject *, unicode)->state.ascii);
    *utf8_cache_out = (const u8 *)pyunicode_get_utf8_cache(unicode);
    *utf8_length_out = (usize)SSRJSON_PYCOMPACTUNICODE_CAST(unicode)->utf8_length;
}

force_inline void set_cache(PyObject *str, const u8 **utf8_cache_addr, usize *utf8_length_addr) {
#if defined(Py_GIL_DISABLED)
    // TODO
    const u8 *expected = NULL;
    if (!atomic_compare_exchange_strong(&SSRJSON_PYCOMPACTUNICODE_CAST(str)->utf8, &expected, *utf8_cache_addr)) {
        // already has an UTF-8 cache, free the allocated one
        PyMem_Free((void *)*utf8_cache_addr);
        *utf8_cache_addr = expected;
        assert(*utf8_length_addr == (usize)SSRJSON_PYCOMPACTUNICODE_CAST(str)->utf8_length);
    }
#else
    SSRJSON_PYCOMPACTUNICODE_CAST(str)->utf8 = (void *)*utf8_cache_addr;
    SSRJSON_PYCOMPACTUNICODE_CAST(str)->utf8_length = (Py_ssize_t)*utf8_length_addr;
#endif
}

force_inline void *pymem_malloc_wrapped(usize size) {
    void *ptr = PyMem_Malloc(size);
    if (unlikely(!ptr)) {
        PyErr_NoMemory();
    }
    return ptr;
}

force_inline void pymem_free_wrapped(void *ptr) {
    PyMem_Free(ptr);
}

force_inline void *pymem_realloc_wrapped(void *ptr, usize size) {
    void *new_ptr = PyMem_Realloc(ptr, size);
    if (unlikely(!new_ptr)) {
        PyErr_NoMemory();
    }
    return new_ptr;
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
        is_compact = SSRJSON_CAST(PyASCIIObject *, unicode)->state.compact;
        char_count = (usize)PyUnicode_GET_LENGTH(unicode);
        if (likely(is_compact)) {
            char_data = PYUNICODE_ASCII_START(unicode);
        } else {
            char_data = SSRJSON_CAST(PyUnicodeObject *, unicode)->data.any;
        }
    }

    *is_ascii_out = is_ascii;
    *char_data_out = char_data;
    *char_count_out = char_count;
}

#endif // SSRJSON_PYUTILS_H

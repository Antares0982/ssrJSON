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
    } else if (kind == 4) {
        ((u32 *)data)[size] = 0;
#if PY_MINOR_VERSION < 12
        is_sharing = sizeof(wchar_t) == 4;
#endif
    } else {
        SSRJSON_UNREACHABLE();
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
    PyObject *str = PyObject_Malloc(offset + size * (tpsize + 1));
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


PyObject *make_unicode_from_raw_ucs4(void *raw_buffer, usize u8size, usize u16size, usize totalsize, bool do_hash);
PyObject *make_unicode_from_raw_ucs2(void *raw_buffer, usize u8size, usize totalsize, bool do_hash);
PyObject *make_unicode_from_raw_ucs1(void *raw_buffer, usize size, bool do_hash);
PyObject *make_unicode_from_raw_ascii(void *raw_buffer, usize size, bool do_hash);
PyObject *make_unicode_down_ucs2_u8(void *raw_buffer, usize size, bool do_hash, bool is_ascii);
PyObject *make_unicode_down_ucs4_u8(void *raw_buffer, usize size, bool do_hash, bool is_ascii);
PyObject *make_unicode_down_ucs4_ucs2(void *raw_buffer, usize size, bool do_hash);
#endif // SSRJSON_PYUTILS_H

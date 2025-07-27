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

#include "pyutils.h"
#include "pythonlib.h"
#include "simd/cvt.h"
#include "simd/memcpy.h"
#include "utils/unicode.h"

force_noinline void init_pyunicode_noinline(void *head, Py_ssize_t size, int kind) {
    init_pyunicode(head, size, kind);
}

PyObject *make_unicode_from_raw_ucs4(void *raw_buffer, usize u8size, usize u16size, usize totalsize, bool do_hash) {
    PyObject *unicode = create_empty_unicode(totalsize, 4);
    if (!unicode) return NULL;
    usize u32size = totalsize - u16size - u8size;
    u32 *writer = PYUNICODE_UCS4_START(unicode);
    // write u32 part
    if (u32size) {
        memcpy(writer + u8size + u16size, SSRJSON_CAST(u32 *, raw_buffer) + u8size + u16size, u32size * sizeof(u32));
    }
    // write u16 part
    if (u16size) {
        long_cvt_noinline_u16_u32_interface(writer + u8size, SSRJSON_CAST(u16 *, raw_buffer) + u8size, u16size);
    }
    // write u8 part
    if (u8size) {
        long_cvt_noinline_u8_u32_interface(writer, SSRJSON_CAST(u8 *, raw_buffer), u8size);
    }
    if (do_hash && totalsize) {
        make_hash(SSRJSON_CAST(PyASCIIObject *, unicode), writer, totalsize * 4);
    }
    return unicode;
}

PyObject *make_unicode_from_raw_ucs2(void *raw_buffer, usize u8size, usize totalsize, bool do_hash) {
    PyObject *unicode = create_empty_unicode(totalsize, 2);
    if (!unicode) return NULL;
    usize u16size = totalsize - u8size;
    u16 *writer = PYUNICODE_UCS2_START(unicode);
    // write u16 part
    if (u16size) {
        memcpy(writer + u8size, SSRJSON_CAST(u16 *, raw_buffer) + u8size, u16size * sizeof(u16));
    }
    // write u8 part
    if (u8size) {
        long_cvt_noinline_u8_u16_interface(writer, SSRJSON_CAST(u8 *, raw_buffer), u8size);
    }
    if (do_hash && totalsize) {
        make_hash(SSRJSON_CAST(PyASCIIObject *, unicode), writer, totalsize * 2);
    }
    return unicode;
}

PyObject *make_unicode_from_raw_ucs1(void *raw_buffer, usize size, bool do_hash) {
    PyObject *unicode = create_empty_unicode(size, 1);
    if (!unicode) return NULL;
    u8 *writer = PYUNICODE_UCS1_START(unicode);
    // write u8 part
    if (size) {
        memcpy(writer, raw_buffer, size);
    }
    if (do_hash && size) {
        make_hash(SSRJSON_CAST(PyASCIIObject *, unicode), writer, size);
    }
    return unicode;
}

PyObject *make_unicode_from_raw_ascii(void *raw_buffer, usize size, bool do_hash) {
    PyObject *unicode = create_empty_unicode(size, 0);
    if (!unicode) return NULL;
    u8 *writer = PYUNICODE_ASCII_START(unicode);
    // write u8 part
    if (size) {
        memcpy(writer, raw_buffer, size);
    }
    if (do_hash && size) {
        make_hash(SSRJSON_CAST(PyASCIIObject *, unicode), writer, size);
    }
    return unicode;
}

PyObject *make_unicode_down_ucs2_u8(void *raw_buffer, usize size, bool do_hash, bool is_ascii) {
    PyObject *unicode = create_empty_unicode(size, is_ascii ? 0 : 1);
    if (!unicode) return NULL;
    u8 *writer = is_ascii ? PYUNICODE_ASCII_START(unicode) : PYUNICODE_UCS1_START(unicode);
    if (size) {
        long_cvt_noinline_u16_u8_interface(writer, SSRJSON_CAST(u16 *, raw_buffer), size);
    }
    if (do_hash && size) {
        make_hash(SSRJSON_CAST(PyASCIIObject *, unicode), writer, size);
    }
    return unicode;
}

PyObject *make_unicode_down_ucs4_u8(void *raw_buffer, usize size, bool do_hash, bool is_ascii) {
    PyObject *unicode = create_empty_unicode(size, is_ascii ? 0 : 1);
    if (!unicode) return NULL;
    u8 *writer = is_ascii ? PYUNICODE_ASCII_START(unicode) : PYUNICODE_UCS1_START(unicode);
    if (size) {
        long_cvt_noinline_u32_u8_interface(writer, SSRJSON_CAST(u32 *, raw_buffer), size);
    }
    if (do_hash && size) {
        make_hash(SSRJSON_CAST(PyASCIIObject *, unicode), writer, size);
    }
    return unicode;
}

PyObject *make_unicode_down_ucs4_ucs2(void *raw_buffer, usize size, bool do_hash) {
    PyObject *unicode = create_empty_unicode(size, 2);
    if (!unicode) return NULL;
    u16 *writer = PYUNICODE_UCS2_START(unicode);
    if (size) {
        long_cvt_noinline_u32_u16_interface(writer, SSRJSON_CAST(u32 *, raw_buffer), size);
    }
    if (do_hash && size) {
        make_hash(SSRJSON_CAST(PyASCIIObject *, unicode), writer, size * 2);
    }
    return unicode;
}

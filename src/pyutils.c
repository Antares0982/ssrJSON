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

void init_pyunicode_noinline(void *head, Py_ssize_t size, int kind) { init_pyunicode(head, size, kind); }

PyObject *make_unicode_from_raw_ucs4(void *raw_buffer, usize u8size, usize u16size, usize totalsize, bool do_hash) {
    PyObject *unicode = create_empty_unicode(totalsize, 4);
    if (!unicode) return NULL;
    usize u32size = totalsize - u16size - u8size;
    u32 *writer = ssrjson_pyunicode_ucs4_start(unicode);
    // write u32 part
    if (u32size) {
        memcpy(writer + u8size + u16size, ssrjson_cast(u32 *, raw_buffer) + u8size + u16size, u32size * sizeof(u32));
    }
    // write u16 part
    if (u16size) {
        long_cvt_noinline_u16_u32_interface(writer + u8size, ssrjson_cast(u16 *, raw_buffer) + u8size, u16size);
    }
    // write u8 part
    if (u8size) { long_cvt_noinline_u8_u32_interface(writer, ssrjson_cast(u8 *, raw_buffer), u8size); }
    if (do_hash && totalsize) { make_hash(ssrjson_pyascii_cast(unicode), writer, totalsize * 4); }
    return unicode;
}

PyObject *make_unicode_from_raw_ucs2(void *raw_buffer, usize u8size, usize totalsize, bool do_hash) {
    PyObject *unicode = create_empty_unicode(totalsize, 2);
    if (!unicode) return NULL;
    usize u16size = totalsize - u8size;
    u16 *writer = ssrjson_pyunicode_ucs2_start(unicode);
    // write u16 part
    if (u16size) { memcpy(writer + u8size, ssrjson_cast(u16 *, raw_buffer) + u8size, u16size * sizeof(u16)); }
    // write u8 part
    if (u8size) { long_cvt_noinline_u8_u16_interface(writer, ssrjson_cast(u8 *, raw_buffer), u8size); }
    if (do_hash && totalsize) { make_hash(ssrjson_pyascii_cast(unicode), writer, totalsize * 2); }
    return unicode;
}

PyObject *make_unicode_from_raw_ucs1(void *raw_buffer, usize size, bool do_hash) {
    PyObject *unicode = create_empty_unicode(size, 1);
    if (!unicode) return NULL;
    u8 *writer = ssrjson_pyunicode_ucs1_start(unicode);
    // write u8 part
    if (size) { memcpy(writer, raw_buffer, size); }
    if (do_hash && size) { make_hash(ssrjson_pyascii_cast(unicode), writer, size); }
    return unicode;
}

PyObject *make_unicode_from_raw_ascii(void *raw_buffer, usize size, bool do_hash) {
    PyObject *unicode = create_empty_unicode(size, 0);
    if (!unicode) return NULL;
    u8 *writer = ssrjson_pyunicode_ascii_start(unicode);
    // write u8 part
    if (size) { memcpy(writer, raw_buffer, size); }
    if (do_hash && size) { make_hash(ssrjson_pyascii_cast(unicode), writer, size); }
    return unicode;
}

PyObject *make_unicode_down_ucs2_u8(void *raw_buffer, usize size, bool do_hash, bool is_ascii) {
    PyObject *unicode = create_empty_unicode(size, is_ascii ? 0 : 1);
    if (!unicode) return NULL;
    u8 *writer = is_ascii ? ssrjson_pyunicode_ascii_start(unicode) : ssrjson_pyunicode_ucs1_start(unicode);
    if (size) { long_cvt_noinline_u16_u8_interface(writer, ssrjson_cast(u16 *, raw_buffer), size); }
    if (do_hash && size) { make_hash(ssrjson_pyascii_cast(unicode), writer, size); }
    return unicode;
}

PyObject *make_unicode_down_ucs4_u8(void *raw_buffer, usize size, bool do_hash, bool is_ascii) {
    PyObject *unicode = create_empty_unicode(size, is_ascii ? 0 : 1);
    if (!unicode) return NULL;
    u8 *writer = is_ascii ? ssrjson_pyunicode_ascii_start(unicode) : ssrjson_pyunicode_ucs1_start(unicode);
    if (size) { long_cvt_noinline_u32_u8_interface(writer, ssrjson_cast(u32 *, raw_buffer), size); }
    if (do_hash && size) { make_hash(ssrjson_pyascii_cast(unicode), writer, size); }
    return unicode;
}

PyObject *make_unicode_down_ucs4_ucs2(void *raw_buffer, usize size, bool do_hash) {
    PyObject *unicode = create_empty_unicode(size, 2);
    if (!unicode) return NULL;
    u16 *writer = ssrjson_pyunicode_ucs2_start(unicode);
    if (size) { long_cvt_noinline_u32_u16_interface(writer, ssrjson_cast(u32 *, raw_buffer), size); }
    if (do_hash && size) { make_hash(ssrjson_pyascii_cast(unicode), writer, size * 2); }
    return unicode;
}

ssrjson_align(64) u64 _PyFastType[8];

// Numpy type pointers
NumpyTypes _NumpyTypes = {0};

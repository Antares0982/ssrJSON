#include "pyutils.h"
#include "pythonlib.h"
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

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
#include "encode/encode_utf8_shared.h"

#define RESERVE_MAX ((~(usize)PY_SSIZE_T_MAX) >> 1)
static_assert((ssrjson_cast(usize, RESERVE_MAX) & (ssrjson_cast(usize, RESERVE_MAX) - 1)) == 0, "");

EncodeUBufInfo _u_buf_reserve(EncodeUBufInfo u_buf_info, usize target_size) {
#if ENCODE_RESERVE_DEBUG
    void *new_ptr = PyObject_Realloc(u_buf_info.head, target_size);
    if (unlikely(!new_ptr)) {
        PyErr_NoMemory();
        u_buf_info.head = u_buf_info.end = NULL;
        return u_buf_info;
    }
    u_buf_info.head = new_ptr;
    u_buf_info.end = ssrjson_cast(u8 *, u_buf_info.head) + target_size;
    return u_buf_info;
#else
    usize u8len = ssrjson_cast(uintptr_t, u_buf_info.end) - ssrjson_cast(uintptr_t, u_buf_info.head);
    assert((u8len & (u8len - 1)) == 0);
    while (target_size > u8len) {
        if (u8len & RESERVE_MAX) {
            PyErr_NoMemory();
            u_buf_info.head = u_buf_info.end = NULL;
            return u_buf_info;
        }
        u8len = (u8len << 1);
    }
    void *new_ptr = PyObject_Realloc(u_buf_info.head, u8len);
    if (unlikely(!new_ptr)) {
        PyErr_NoMemory();
        u_buf_info.head = u_buf_info.end = NULL;
        return u_buf_info;
    }
    u_buf_info.head = new_ptr;
    u_buf_info.end = ssrjson_cast(u8 *, u_buf_info.head) + u8len;
    return u_buf_info;
#endif
}

bool resize_to_fit_pyunicode(EncodeUBufInfo *u_buf_info, Py_ssize_t len, int ucs_type) {
    Py_ssize_t char_size = ucs_type ? ucs_type : 1;
    Py_ssize_t struct_size = ucs_type ? sizeof(PyCompactUnicodeObject) : sizeof(PyASCIIObject);
    assert(len <= ((PY_SSIZE_T_MAX - struct_size) / char_size - 1));
    // Resizes to a smaller size. It *should* always be successful
    void *new_ptr = PyObject_Realloc(u_buf_info->head, struct_size + (len + 1) * char_size);
    if (unlikely(!new_ptr)) {
        PyErr_NoMemory();
        return false;
    }
    u_buf_info->head = new_ptr;
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

extern const u8 ControlEscapeTable_u8[256 * 16];
extern const u16 ControlEscapeTable_u16[256 * 16];
extern const u32 ControlEscapeTable_u32[256 * 8];


#define scalar_encoder(_src_t_, _dst_t_, _stride_, _count_off_)                                                     \
    const _src_t_ *const src_end = src + len;                                                                       \
    while (src < src_end) {                                                                                         \
        const _src_t_ unicode = *src++;                                                                             \
        if (unicode > 255) {                                                                                        \
            *writer++ = unicode;                                                                                    \
        } else {                                                                                                    \
            memcpy(writer, ssrjson_concat2(ControlEscapeTable, _dst_t_) + _stride_ * unicode, 8 * sizeof(_dst_t_)); \
            writer += *ssrjson_cast(                                                                                \
                    const u64 *, ssrjson_concat2(ControlEscapeTable, _dst_t_) + _stride_ * unicode + _count_off_);  \
        }                                                                                                           \
    }

u8 *ssrjson_nofail encode_scalar_u8_u8(u8 *restrict writer, const u8 *restrict src, usize len) {
    assume(len < 16);
    scalar_encoder(u8, u8, 16, 8);
    return writer;
}

u16 *ssrjson_nofail encode_scalar_u8_u16(u16 *restrict writer, const u8 *restrict src, usize len) {
    assume(len < 16);
    scalar_encoder(u8, u16, 16, 8);
    return writer;
}

u32 *ssrjson_nofail encode_scalar_u8_u32(u32 *restrict writer, const u8 *restrict src, usize len) {
    assume(len < 16);
    scalar_encoder(u8, u32, 8, 6);
    return writer;
}

u16 *ssrjson_nofail encode_scalar_u16_u16(u16 *restrict writer, const u16 *restrict src, usize len) {
    assume(len < 8);
    scalar_encoder(u16, u16, 16, 8);
    return writer;
}

u32 *ssrjson_nofail encode_scalar_u16_u32(u32 *restrict writer, const u16 *restrict src, usize len) {
    assume(len < 8);
    scalar_encoder(u16, u32, 8, 6);
    return writer;
}

u32 *ssrjson_nofail encode_scalar_u32_u32(u32 *restrict writer, const u32 *restrict src, usize len) {
    assume(len < 4);
    scalar_encoder(u32, u32, 8, 6);
    return writer;
}

u8 *ssrjson_nofail encode_bytes_ucs1_scalar(u8 *writer, const u8 *src, usize len) {
    assume(len < 16);
    const u8 *const src_end = src + len;
    while (src < src_end) {
        const u8 unicode = *src++;
        writer = encode_one_ucs1(writer, unicode);
    }
    return writer;
}

u8 *encode_bytes_ucs2_scalar(u8 *writer, const u16 *src, usize len) {
    // encode ucs2 may fail.
    assume(len < 8);
    const u16 *const src_end = src + len;
    while (src < src_end) {
        const u16 unicode = *src++;
        writer = encode_one_ucs2(writer, unicode);
        return_if_unlikely(!writer);
    }
    return writer;
}

u8 *encode_bytes_ucs4_scalar(u8 *writer, const u32 *src, usize len) {
    // encode ucs4 may fail.
    assume(len < 4);
    const u32 *const src_end = src + len;
    while (src < src_end) {
        const u32 unicode = *src++;
        writer = encode_one_ucs4(writer, unicode);
        return_if_unlikely(!writer);
    }
    return writer;
}

u8 *ssrjson_nofail encode_bytes_ucs1_raw_utf8_scalar(u8 *writer, const u8 *src, usize len) {
    assume(len < 16);
    const u8 *const src_end = src + len;
    while (src < src_end) { writer = encode_one_ucs1_noescape(writer, *src++); }
    return writer;
}

u8 *encode_bytes_ucs2_raw_utf8_scalar(u8 *writer, const u16 *src, usize len) {
    // encode ucs2 may fail.
    assume(len < 8);
    const u16 *const src_end = src + len;
    while (src < src_end) {
        writer = encode_one_ucs2_noescape(writer, *src++);
        return_if_unlikely(!writer);
    }
    return writer;
}

u8 *encode_bytes_ucs4_raw_utf8_scalar(u8 *writer, const u32 *src, usize len) {
    // encode ucs4 may fail.
    assume(len < 4);
    const u32 *const src_end = src + len;
    while (src < src_end) {
        writer = encode_one_ucs4_noescape(writer, *src++);
        return_if_unlikely(!writer);
    }
    return writer;
}

/*==============================================================================
 * Global Vars
 *============================================================================*/
#if SSRJSON_GIL_ENABLED
EncodeCtnWithIndex _EncodeCtnBuffer[SSRJSON_ENCODE_MAX_RECURSION];
#endif

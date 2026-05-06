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


#ifndef ENCODE_SCALAR_H
#define ENCODE_SCALAR_H

#include "encode/encode_shared.h"
#include "ssrjson.h"
//
#include <simd/compile_feature_check.h>

#if _CompileVectorBits == 128

/* ============================================================================
 * STR-output (UCS) cold-path helpers. Each `encode_unicode_noncompact_<r>_<w>`
 * dispatches: if cnt < READ_BATCH_COUNT (= 16/sizeof(src_t)) take the pure
 * scalar loop, else call into the SSE-128 SIMD impl. Used by str leaves.
 * ==========================================================================*/

force_inline u8 *encode_one_scalar_u8_u8(u8 *dst, u8 unicode) {
    memcpy(dst, &ControlEscapeTable_u8[unicode * 8], 8);
    return dst + _ControlJump[unicode];
}

force_inline u8 *encode_unicode_noncompact_u8_u8(u8 *dst, const u8 *src, usize cnt) {
    if (cnt < 16) {
        // cold path for scalar.
        for (usize i = 0; i < cnt; ++i) {
            dst = encode_one_scalar_u8_u8(dst, *src++);
        }
        return dst;
    }
    // call 128bit SIMD path
    dst = encode_unicode_loop_u8_u8_128(dst, &src, &cnt);
    if (cnt) dst = encode_trailing_copy_with_cvt_u8_u8_128(dst, src, cnt);
    return dst;
}

force_inline u16 *encode_one_scalar_u16_u16(u16 *dst, u16 unicode) {
    if (unicode < 256) {
        memcpy(dst, &ControlEscapeTable_u16[unicode * 8], 16);
        return dst + _ControlJump[unicode];
    } else {
        *dst = unicode;
        return dst + 1;
    }
}

force_inline u16 *encode_unicode_noncompact_u16_u16(u16 *dst, const u16 *src, usize cnt) {
    if (cnt < 8) {
        // cold path for scalar.
        for (usize i = 0; i < cnt; ++i) {
            dst = encode_one_scalar_u16_u16(dst, *src++);
        }
        return dst;
    }
    // call 128bit SIMD path
    dst = encode_unicode_loop_u16_u16_128(dst, &src, &cnt);
    if (cnt) dst = encode_trailing_copy_with_cvt_u16_u16_128(dst, src, cnt);
    return dst;
}

force_inline u32 *encode_one_scalar_u32_u32(u32 *dst, u32 unicode) {
    if (unicode < 256) {
        memcpy(dst, &ControlEscapeTable_u32[unicode * 8], 32);
        return dst + _ControlJump[unicode];
    } else {
        *dst = unicode;
        return dst + 1;
    }
}

force_inline u32 *encode_unicode_noncompact_u32_u32(u32 *dst, const u32 *src, usize cnt) {
    if (cnt < 4) {
        // cold path for scalar.
        for (usize i = 0; i < cnt; ++i) {
            dst = encode_one_scalar_u32_u32(dst, *src++);
        }
        return dst;
    }
    // call 128bit SIMD path
    dst = encode_unicode_loop_u32_u32_128(dst, &src, &cnt);
    if (cnt) dst = encode_trailing_copy_with_cvt_u32_u32_128(dst, src, cnt);
    return dst;
}

force_inline u16 *encode_one_scalar_u8_u16(u16 *dst, u8 unicode) {
    return encode_one_scalar_u16_u16(dst, unicode);
}

force_inline u16 *encode_unicode_noncompact_u8_u16(u16 *dst, const u8 *src, usize cnt) {
    if (cnt < 16) {
        // cold path for scalar.
        for (usize i = 0; i < cnt; ++i) {
            dst = encode_one_scalar_u8_u16(dst, *src++);
        }
        return dst;
    }
    // call 128bit SIMD path
    dst = encode_unicode_loop_u8_u16_128(dst, &src, &cnt);
    if (cnt) dst = encode_trailing_copy_with_cvt_u8_u16_128(dst, src, cnt);
    return dst;
}

force_inline u32 *encode_one_scalar_u8_u32(u32 *dst, u8 unicode) {
    return encode_one_scalar_u32_u32(dst, unicode);
}

force_inline u32 *encode_unicode_noncompact_u8_u32(u32 *dst, const u8 *src, usize cnt) {
    if (cnt < 16) {
        // cold path for scalar.
        for (usize i = 0; i < cnt; ++i) {
            dst = encode_one_scalar_u8_u32(dst, *src++);
        }
        return dst;
    }
    // call 128bit SIMD path
    dst = encode_unicode_loop_u8_u32_128(dst, &src, &cnt);
    if (cnt) dst = encode_trailing_copy_with_cvt_u8_u32_128(dst, src, cnt);
    return dst;
}

force_inline u32 *encode_one_scalar_u16_u32(u32 *dst, u16 unicode) {
    return encode_one_scalar_u32_u32(dst, unicode);
}

force_inline u32 *encode_unicode_noncompact_u16_u32(u32 *dst, const u16 *src, usize cnt) {
    if (cnt < 8) {
        // cold path for scalar.
        for (usize i = 0; i < cnt; ++i) {
            dst = encode_one_scalar_u16_u32(dst, *src++);
        }
        return dst;
    }
    // call 128bit SIMD path
    dst = encode_unicode_loop_u16_u32_128(dst, &src, &cnt);
    if (cnt) dst = encode_trailing_copy_with_cvt_u16_u32_128(dst, src, cnt);
    return dst;
}

/* ============================================================================
 * BYTES-output cold-path helpers. Threshold for scalar vs SIMD is
 * `len * sizeof(src_t) < 16` (one 128-bit reg). Scalar uses encode_one_ucs<k>;
 * SIMD reuses the SSE-128 force_inline `bytes_write_ucs<k>` (which is safe to
 * over-read backwards by 16 bytes only when total source bytes >= 16).
 * ==========================================================================*/

force_inline u8 *encode_bytes_noncompact_ucs1(u8 *dst, const u8 *src, usize cnt) {
    if (cnt < 16) {
        for (usize i = 0; i < cnt; ++i) dst = encode_one_ucs1(dst, src[i]);
        return dst;
    }
    /* is_key=true to skip loop4 (cold path; user-stipulated). */
    return bytes_write_ucs1(dst, src, cnt, true);
}

force_inline u8 *encode_bytes_noncompact_ucs2(u8 *dst, const u16 *src, usize cnt) {
    if (cnt < 8) {
        for (usize i = 0; i < cnt; ++i) {
            dst = encode_one_ucs2(dst, src[i]);
            return_if_unlikely(!dst);
        }
        return dst;
    }
    return bytes_write_ucs2(dst, src, cnt, true);
}

force_inline u8 *encode_bytes_noncompact_ucs4(u8 *dst, const u32 *src, usize cnt) {
    if (cnt < 4) {
        for (usize i = 0; i < cnt; ++i) {
            dst = encode_one_ucs4(dst, src[i]);
            return_if_unlikely(!dst);
        }
        return dst;
    }
    return bytes_write_ucs4(dst, src, cnt, true);
}

/* Raw UTF-8 (no JSON escapes), used by the non-compact write_cache impl. */

force_inline u8 *encode_bytes_noncompact_raw_utf8_ucs1(u8 *dst, const u8 *src, usize cnt) {
    if (cnt < 16) {
        for (usize i = 0; i < cnt; ++i) dst = encode_one_ucs1_noescape(dst, src[i]);
        return dst;
    }
    return bytes_write_ucs1_raw_utf8(dst, src, cnt, true);
}

force_inline u8 *encode_bytes_noncompact_raw_utf8_ucs2(u8 *dst, const u16 *src, usize cnt) {
    if (cnt < 8) {
        for (usize i = 0; i < cnt; ++i) {
            dst = encode_one_ucs2_noescape(dst, src[i]);
            return_if_unlikely(!dst);
        }
        return dst;
    }
    return bytes_write_ucs2_raw_utf8(dst, src, cnt, true);
}

force_inline u8 *encode_bytes_noncompact_raw_utf8_ucs4(u8 *dst, const u32 *src, usize cnt) {
    if (cnt < 4) {
        for (usize i = 0; i < cnt; ++i) {
            dst = encode_one_ucs4_noescape(dst, src[i]);
            return_if_unlikely(!dst);
        }
        return dst;
    }
    return bytes_write_ucs4_raw_utf8(dst, src, cnt, true);
}

/* Non-compact-safe write_cache. Mirrors write_cache_impl but uses the
 * encode_bytes_noncompact_raw_utf8_* helpers above so that short non-compact
 * sources do not over-read. The destination buffer is freshly malloc'd, so
 * destination over-write is bounded by max_utf8_bytes_per_ucs<k> * len. */
force_inline bool nc_write_cache_impl(int src_pykind, const void *src_voidp, usize len,
                                      const u8 **utf8_cache_out, usize *utf8_length_out) {
    void *new_buffer;
    u8 *writer;
    switch (src_pykind) {
        case 1: {
            new_buffer = pymem_malloc_wrapped(max_utf8_bytes_per_ucs1 * len + __excess_bytes_write_ucs1_raw_utf8_trailing_128);
            return_if_unlikely(!new_buffer);
            writer = encode_bytes_noncompact_raw_utf8_ucs1(ssrjson_cast(u8 *, new_buffer), src_voidp, len);
            break;
        }
        case 2: {
            new_buffer = pymem_malloc_wrapped(max_utf8_bytes_per_ucs2 * len + __excess_bytes_write_ucs2_raw_utf8_trailing_128);
            return_if_unlikely(!new_buffer);
            writer = encode_bytes_noncompact_raw_utf8_ucs2(ssrjson_cast(u8 *, new_buffer), src_voidp, len);
            if (unlikely(!writer)) goto fail;
            break;
        }
        case 4: {
            new_buffer = pymem_malloc_wrapped(max_utf8_bytes_per_ucs4 * len + __excess_bytes_write_ucs4_raw_utf8_trailing_128);
            return_if_unlikely(!new_buffer);
            writer = encode_bytes_noncompact_raw_utf8_ucs4(ssrjson_cast(u8 *, new_buffer), src_voidp, len);
            if (unlikely(!writer)) goto fail;
            break;
        }
        default: {
            ssrjson_unreachable();
        }
    }
    usize utf8_length = (usize)(writer - ssrjson_cast(u8 *, new_buffer));
    *utf8_length_out = utf8_length;
    void *resized_buffer = pymem_realloc_wrapped(new_buffer, utf8_length + 1);
    if (unlikely(!resized_buffer)) goto fail;
    u8 *final_buffer = ssrjson_cast(u8 *, resized_buffer);
    final_buffer[utf8_length] = 0;
    *utf8_cache_out = final_buffer;
    return true;
fail:;
    pymem_free_wrapped(new_buffer);
    return false;
}

/* ============================================================================
 * Container value/key leaves for str output (B/C series). Generated via
 * multi-include of `_encode_non_compact_impl.inl.h`.
 * ==========================================================================*/
#    include "encode_non_compact_wrap.h"

/* ============================================================================
 * Container value/key leaves for bytes output (D/E series). Generated via
 * multi-include of `bytes/encode_bytes_non_compact_wrap.h`.
 * ==========================================================================*/
#    include "bytes/encode_bytes_non_compact_wrap.h"

/* ============================================================================
 * Top-level single-string entries (A1/A2). Called from ssrjson_Dumps and
 * ssrjson_DumpsToBytes when the input is a non-compact unicode (i.e., a str
 * subclass).
 * ==========================================================================*/

static force_noinline PyObject *_dumps_single_unicode_nc_str_kind1(PyObject *unicode, const u8 *src, usize len) {
    EncodeUnicodeBufferInfo unicode_buffer_info;
    unicode_buffer_info.head = PyObject_Malloc(SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    return_if_no_memory(unicode_buffer_info.head);
    unicode_buffer_info.end = ssrjson_cast(u8 *, unicode_buffer_info.head) + SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE;
    /* Output is a compact UCS1 (or ASCII) Python str; we don't know which until
     * we know whether the source has any non-ASCII byte. For simplicity, always
     * emit as UCS1 (compact) — even pure-ASCII inputs become UCS1 here. The
     * cold-path allocation cost dwarfs the per-char wide-write difference. */
    u8 *writer = ssrjson_cast(u8 *, unicode_buffer_info.head) + sizeof(PyCompactUnicodeObject);
    writer = _unicode_buffer_append_str_value_non_compact_in_obj_u8_u8_indent0(src, len, writer, &unicode_buffer_info, 0);
    if (unlikely(!writer)) {
        PyObject_Free(unicode_buffer_info.head);
        return NULL;
    }
    /* drop trailing ',' written by the leaf */
    writer--;
    usize written_len = (usize)(writer - ssrjson_cast(u8 *, unicode_buffer_info.head) - sizeof(PyCompactUnicodeObject));
    assert(written_len >= 2);
    if (unlikely(!resize_to_fit_pyunicode(&unicode_buffer_info, (Py_ssize_t)written_len, 1))) {
        PyObject_Free(unicode_buffer_info.head);
        return NULL;
    }
    init_pyunicode_noinline(unicode_buffer_info.head, (Py_ssize_t)written_len, 1);
    return (PyObject *)unicode_buffer_info.head;
}

static force_noinline PyObject *_dumps_single_unicode_nc_str_kind2(PyObject *unicode, const u16 *src, usize len) {
    EncodeUnicodeBufferInfo unicode_buffer_info;
    unicode_buffer_info.head = PyObject_Malloc(SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    return_if_no_memory(unicode_buffer_info.head);
    unicode_buffer_info.end = ssrjson_cast(u8 *, unicode_buffer_info.head) + SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE;
    u16 *writer = ssrjson_cast(u16 *, ssrjson_cast(u8 *, unicode_buffer_info.head) + sizeof(PyCompactUnicodeObject));
    writer = _unicode_buffer_append_str_value_non_compact_in_obj_u16_u16_indent0(src, len, writer, &unicode_buffer_info, 0);
    if (unlikely(!writer)) {
        PyObject_Free(unicode_buffer_info.head);
        return NULL;
    }
    writer--;
    usize written_bytes = (usize)((u8 *)writer - ssrjson_cast(u8 *, unicode_buffer_info.head) - sizeof(PyCompactUnicodeObject));
    usize written_len = written_bytes / sizeof(u16);
    assert(written_len >= 2);
    if (unlikely(!resize_to_fit_pyunicode(&unicode_buffer_info, (Py_ssize_t)written_len, 2))) {
        PyObject_Free(unicode_buffer_info.head);
        return NULL;
    }
    init_pyunicode_noinline(unicode_buffer_info.head, (Py_ssize_t)written_len, 2);
    return (PyObject *)unicode_buffer_info.head;
}

static force_noinline PyObject *_dumps_single_unicode_nc_str_kind4(PyObject *unicode, const u32 *src, usize len) {
    EncodeUnicodeBufferInfo unicode_buffer_info;
    unicode_buffer_info.head = PyObject_Malloc(SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    return_if_no_memory(unicode_buffer_info.head);
    unicode_buffer_info.end = ssrjson_cast(u8 *, unicode_buffer_info.head) + SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE;
    u32 *writer = ssrjson_cast(u32 *, ssrjson_cast(u8 *, unicode_buffer_info.head) + sizeof(PyCompactUnicodeObject));
    writer = _unicode_buffer_append_str_value_non_compact_in_obj_u32_u32_indent0(src, len, writer, &unicode_buffer_info, 0);
    if (unlikely(!writer)) {
        PyObject_Free(unicode_buffer_info.head);
        return NULL;
    }
    writer--;
    usize written_bytes = (usize)((u8 *)writer - ssrjson_cast(u8 *, unicode_buffer_info.head) - sizeof(PyCompactUnicodeObject));
    usize written_len = written_bytes / sizeof(u32);
    assert(written_len >= 2);
    if (unlikely(!resize_to_fit_pyunicode(&unicode_buffer_info, (Py_ssize_t)written_len, 4))) {
        PyObject_Free(unicode_buffer_info.head);
        return NULL;
    }
    init_pyunicode_noinline(unicode_buffer_info.head, (Py_ssize_t)written_len, 4);
    return (PyObject *)unicode_buffer_info.head;
}

PyObject *ssrjson_dumps_single_unicode_non_compact_to_str(PyObject *unicode) {
    int kind = PyUnicode_KIND(unicode);
    usize len = (usize)PyUnicode_GET_LENGTH(unicode);
    const void *data = ssrjson_pyunicode_cast(unicode)->data.any;
    switch (kind) {
        case 1:
            return _dumps_single_unicode_nc_str_kind1(unicode, data, len);
        case 2:
            return _dumps_single_unicode_nc_str_kind2(unicode, data, len);
        case 4:
            return _dumps_single_unicode_nc_str_kind4(unicode, data, len);
        default:
            ssrjson_unreachable();
    }
    return NULL;
}

static force_noinline PyObject *_dumps_single_unicode_nc_bytes_kind(PyObject *unicode, int kind, const void *src, usize len, bool is_write_cache) {
    EncodeUnicodeBufferInfo unicode_buffer_info;
    unicode_buffer_info.head = PyObject_Malloc(SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    return_if_no_memory(unicode_buffer_info.head);
    unicode_buffer_info.end = ssrjson_cast(u8 *, unicode_buffer_info.head) + SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE;
    u8 *writer = ssrjson_cast(u8 *, unicode_buffer_info.head) + PYBYTES_START_OFFSET;
    switch (kind) {
        case 1:
            writer = _bytes_buffer_append_str_value_non_compact_in_obj_u8_indent0(writer, src, len, unicode, &unicode_buffer_info, 0, is_write_cache);
            break;
        case 2:
            writer = _bytes_buffer_append_str_value_non_compact_in_obj_u16_indent0(writer, src, len, unicode, &unicode_buffer_info, 0, is_write_cache);
            break;
        case 4:
            writer = _bytes_buffer_append_str_value_non_compact_in_obj_u32_indent0(writer, src, len, unicode, &unicode_buffer_info, 0, is_write_cache);
            break;
        default:
            ssrjson_unreachable();
    }
    if (unlikely(!writer)) {
        PyObject_Free(unicode_buffer_info.head);
        return NULL;
    }
    /* drop trailing ',' written by the leaf */
    writer--;
    usize written_len = (usize)(writer - ssrjson_cast(u8 *, unicode_buffer_info.head) - PYBYTES_START_OFFSET);
    assert(written_len >= 2);
    if (unlikely(!resize_to_fit_pybytes(&unicode_buffer_info, written_len))) {
        PyObject_Free(unicode_buffer_info.head);
        return NULL;
    }
    init_pybytes(unicode_buffer_info.head, written_len);
    return (PyObject *)unicode_buffer_info.head;
}

PyObject *ssrjson_dumps_single_unicode_non_compact_to_bytes(PyObject *unicode, bool is_write_cache) {
    int kind = PyUnicode_KIND(unicode);
    usize len = (usize)PyUnicode_GET_LENGTH(unicode);
    const void *data = ssrjson_pyunicode_cast(unicode)->data.any;
    return _dumps_single_unicode_nc_bytes_kind(unicode, kind, data, len, is_write_cache);
}

#endif // _CompileVectorBits == 128

#undef _CompileVectorBits

#endif // ENCODE_SCALAR_H

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

#ifndef SSRJSON_DECODE_DECODE_BYTES_H
#define SSRJSON_DECODE_DECODE_BYTES_H

// must come first: defines the `loads_bytes` that the root templates inline
#include "decode_bytes_str_wrap.h"
//
#include "decode_bytes_root_wrap.h"
#include "decode_float_wrap.h"
#include "decode_shared.h"
#include "simd/memcpy.h"
#include "ssrjson.h"
#include "str/tools.h"
//
#include "simd/compile_feature_check.h"

/** Read single value JSON document. */
internal_simd_noinline PyObject *loads_root_single_bytes(DecoderBuffers *decoder_context, const u8 *dat,
                                                         usize len DECODER_TLS_KEYCACHE_ADDITIONAL_ARGDEF) {
#define return_err(_pos, _type, _msg)                                                             \
    do {                                                                                          \
        if (_type == JSONDecodeError) {                                                           \
            PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, ((u8 *)_pos) - (u8 *)dat); \
        } else {                                                                                  \
            PyErr_SetString(_type, _msg);                                                         \
        }                                                                                         \
        goto fail_cleanup;                                                                        \
    } while (0)

    const u8 *cur = (const u8 *)dat;
    const u8 *const end = cur + len;

    PyObject *ret = NULL;

    if (char_is_number(*cur)) {
        ret = loads_number_u8(&cur, end);
        if (likely(ret)) goto single_end;
        goto fail_number;
    }
    if (*cur == '"') {
        u8 *write_buffer;
        bool need_dealloc = false;
        if (unlikely(!check_and_reserve_bytes_str_buffer(
                    decoder_context, (Py_ssize_t)len, &write_buffer, &need_dealloc))) {
            goto fail_alloc;
        }
        ret = loads_bytes_not_key(&cur, write_buffer, end DECODER_TLS_KEYCACHE_ADDITIONAL_ARG);
        free_bytes_str_buffer(write_buffer, need_dealloc);
        if (likely(ret)) goto single_end;
        goto fail_string;
    }
    if (*cur == 't') {
        if (likely(_read_true_u8(&cur, end))) {
            Py_Immortal_IncRef(Py_True);
            ret = Py_True;
            goto single_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_read_false_u8(&cur, end))) {
            Py_Immortal_IncRef(Py_False);
            ret = Py_False;
            goto single_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_read_null_u8(&cur, end))) {
            Py_Immortal_IncRef(Py_None);
            ret = Py_None;
            goto single_end;
        }
        if (_read_nan_u8(&cur, end)) {
            ret = PyFloat_FromDouble(fabs(Py_NAN));
            if (likely(ret)) goto single_end;
        }
        goto fail_literal_null;
    }
    {
        ret = loads_inf_or_nan_u8(false, &cur, end);
        if (likely(ret)) goto single_end;
    }
    goto fail_character;

single_end:
    assert(ret);
    if (unlikely(cur < end)) {
        while (char_is_space(*cur)) cur++;
        if (unlikely(cur < end)) goto fail_garbage;
    }
    return ret;

fail_string:
    return_err(cur, JSONDecodeError, "invalid string");
fail_number:
    return_err(cur, JSONDecodeError, "invalid number");
fail_alloc:
    return_err(cur, PyExc_MemoryError, "memory allocation failed");
fail_literal_true:
    return_err(cur, JSONDecodeError, "invalid literal, expected a valid literal such as 'true'");
fail_literal_false:
    return_err(cur, JSONDecodeError, "invalid literal, expected a valid literal such as 'false'");
fail_literal_null:
    return_err(cur, JSONDecodeError, "invalid literal, expected a valid literal such as 'null'");
fail_character:
    return_err(cur, JSONDecodeError, "unexpected character, expected a valid root value");
fail_garbage:
    return_err(cur, JSONDecodeError, "unexpected content after document");
fail_cleanup:
    Py_XDECREF(ret);
    return NULL;
#undef return_err
}

force_inline bool _skip_starting_space(char **buffer_addr, Py_ssize_t *len_addr) {
    /* skip empty contents before json document */
    if (unlikely(char_is_space_or_comment(**buffer_addr))) {
        if (likely(char_is_space(**buffer_addr))) {
            do {
                *buffer_addr = *buffer_addr + 1;
                *len_addr = (*len_addr) - 1;
            } while (char_is_space(**buffer_addr));
        }
        if (unlikely(*len_addr <= 0)) {
            PyErr_Format(JSONDecodeError, "input data is empty");
            return false;
        }
    }
    return true;
}

/*
 * Copy destination for the document itself. The copy starts at an offset of up
 * to SSRJSON_MEMCPY_SIMD_SIZE - 1 inside the allocation, reproducing the
 * alignment of the input, and the reserve leaves a whole register readable
 * past the terminating NUL.
 */
force_inline void _alloc_aligned_b_buf(DecoderBuffers *decoder_context, Py_ssize_t len, bool *dynamic, u8 **buffer) {
    if (unlikely(len > (Py_ssize_t)PY_SSIZE_T_MAX - 3 * SSRJSON_MEMCPY_SIMD_SIZE - 4)) {
        PyErr_NoMemory();
        *buffer = NULL;
        return;
    }
    Py_ssize_t required_size = size_align_up(len + 2 * SSRJSON_MEMCPY_SIMD_SIZE + 4, SSRJSON_MEMCPY_SIMD_SIZE);
    if (unlikely(required_size > SSRJSON_STRING_BUFFER_SIZE)) {
        *buffer = ssrjson_aligned_alloc(SSRJSON_MEMCPY_SIMD_SIZE, required_size);
        if (unlikely(!*buffer)) {
            PyErr_NoMemory();
            return;
        }
        *dynamic = true;
    } else {
        *buffer = decoder_context->decoder_ctx_bytes_src_buffer;
        *dynamic = false;
    }
}

force_inline bool should_loads_bytes_pretty(const u8 *buffer, Py_ssize_t len) {
    if (len > 3) {
        // check if can use pretty read
        u8 second, third;
        second = buffer[1];
        third = buffer[2];
        if (second == '\n' || third == '\n') {
            // likely to hit
            return true;
        }
        if (char_is_space(second) && char_is_space(third)) { return true; }
    }
    return false;
}

internal_simd_noinline PyObject *ssrjson_decode_bytes(DecoderBuffers *decoder_context, char *_buffer, Py_ssize_t len,
                                                      PyObject *object_hook DECODER_TLS_KEYCACHE_ADDITIONAL_ARGDEF) {
    if (unlikely(!len)) {
        PyErr_Format(JSONDecodeError, "input data is empty");
        return NULL;
    }

    assert(_buffer);
    assert(len > 0);

    if (!_skip_starting_space(&_buffer, &len)) { return NULL; }

    u8 *_new_buffer;
    bool is_dynamic;
    _alloc_aligned_b_buf(decoder_context, len, &is_dynamic, &_new_buffer);
    if (!_new_buffer) {
        PyErr_NoMemory();
        return NULL;
    }
    u8 *buffer;
    {
        uintptr_t _buffer_int = (uintptr_t)_buffer;
        usize align_offset = (_buffer_int & (SSRJSON_MEMCPY_SIMD_SIZE - 1));
        buffer = _new_buffer + align_offset;
        ssrjson_memcpy_prealigned((void *)buffer, (const void *)_buffer, (usize)len);
    }

    u8 *const end = buffer + len;
    *end = 0;
    PyObject *ret;

    /* read json document */
    if (likely(char_is_container(*buffer))) {
        if (should_loads_bytes_pretty(buffer, len)) {
            ret = loads_bytes_root_pretty(
                    decoder_context, buffer, len, object_hook DECODER_TLS_KEYCACHE_ADDITIONAL_ARG);
        } else {
            ret = loads_bytes_root_minify(
                    decoder_context, buffer, len, object_hook DECODER_TLS_KEYCACHE_ADDITIONAL_ARG);
        }
    } else {
        ret = loads_root_single_bytes(decoder_context, buffer, len DECODER_TLS_KEYCACHE_ADDITIONAL_ARG);
    }

    if (is_dynamic) ssrjson_aligned_free(_new_buffer);
    return ret;
}

#undef _CompileVectorBits

#endif // SSRJSON_DECODE_DECODE_BYTES_H

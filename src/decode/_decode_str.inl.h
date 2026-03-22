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

#ifdef SSRJSON_CLANGD_CHECKING
#    ifndef _CompileVectorBits
#        define COMPILE_CONTEXT_DECODE
#        include "decode_str_root_wrap.h"
//
#        include "simd/compile_feature_check.h"
#        define COMPILE_UCS_LEVEL 0
#    endif
#endif


#if COMPILE_UCS_LEVEL == 0
#    define COMPILE_READ_UCS_LEVEL 1
#else
#    define COMPILE_READ_UCS_LEVEL COMPILE_UCS_LEVEL
#endif
//
#include "compile_context/sr_in.inl.h"

force_inline bool check_and_reserve_str_buffer(DecoderBuffers *decoder_context, Py_ssize_t len, src_t **buffer_head_addr, bool *need_dealloc) {
    // consider the max length of the buffer we need
    // assume that each string has an escape of ucs4, we need buffer with size
    // sizeof(ucs4) * len == 4 * len
    // reserve additional _TailPadding bytes before and after the buffer for convenience,
    // i.e. additional 2 * _TailPadding bytes
    if (len > ((Py_ssize_t)PY_SSIZE_T_MAX - _TailPadding * 2) / 4) {
        return false;
    }
    static_assert(((Py_ssize_t)SSRJSON_STRING_BUFFER_SIZE - _TailPadding * 2) > 4, "((Py_ssize_t)SSRJSON_STRING_BUFFER_SIZE - 128) > 4");
    Py_ssize_t new_buffer_size = 4 * len + 2 * _TailPadding;
    if (new_buffer_size > SSRJSON_STRING_BUFFER_SIZE) {
        u8 *new_buffer = (u8 *)SSRJSON_MALLOC(new_buffer_size);
        if (!new_buffer) return false;
        *buffer_head_addr = (src_t *)(new_buffer + _TailPadding);
        *need_dealloc = true;
    } else {
        *buffer_head_addr = (src_t *)(decoder_context->decoder_ctx_temp_buffer + _TailPadding);
        *need_dealloc = false;
    }
    return true;
}

/** Read single value JSON document. */
internal_simd_noinline PyObject *loads_root_single(DecoderBuffers *decoder_context, const src_t *dat, Py_ssize_t len DECODER_TLS_KEYCACHE_ADDITIONAL_ARGDEF) {
#define return_err(_pos, _type, _msg)                                                             \
    do {                                                                                          \
        if (_type == JSONDecodeError) {                                                           \
            PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, ((u8 *)_pos) - (u8 *)dat); \
        } else {                                                                                  \
            PyErr_SetString(_type, _msg);                                                         \
        }                                                                                         \
        goto fail_cleanup;                                                                        \
    } while (0)

    assert(len > 0);
    //
    const src_t *cur = dat;
    const src_t *const end = cur + len;

    PyObject *ret = NULL;

    if (*cur <= U8MAX && char_is_number(*cur)) {
        ret = loads_number(&cur, end);
        if (likely(ret)) goto single_end;
        goto fail_number;
    }
    if (*cur == '"') {
        // u8 *write_buffer;
        src_t *string_buffer_head;
        bool need_dealloc = false;
        check_and_reserve_str_buffer(decoder_context, len, &string_buffer_head, &need_dealloc);
        cur++;
        ret = decode_str(&cur, end, string_buffer_head, false DECODER_TLS_KEYCACHE_ADDITIONAL_ARG);
        if (need_dealloc) {
            SSRJSON_FREE((void *)((u8 *)string_buffer_head - _TailPadding));
        }
        if (likely(ret)) goto single_end;
        goto fail_string;
    }
    if (*cur == 't') {
        if (likely(_read_true(&cur, end))) {
            Py_Immortal_IncRef(Py_True);
            ret = Py_True;
            goto single_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_read_false(&cur, end))) {
            Py_Immortal_IncRef(Py_False);
            ret = Py_False;
            goto single_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_read_null(&cur, end))) {
            Py_Immortal_IncRef(Py_None);
            ret = Py_None;
            goto single_end;
        }
        if (_read_nan(&cur, end)) {
            ret = PyFloat_FromDouble(fabs(Py_NAN));
            if (likely(ret)) goto single_end;
        }
        goto fail_literal_null;
    }
    {
        ret = loads_inf_or_nan(false, &cur, end);
        if (likely(ret)) goto single_end;
    }
    goto fail_character;

single_end:
    assert(ret);
    if (unlikely(cur < end)) {
        if (*cur == ' ') fast_skip_spaces(&cur, end);
        if (*cur <= U8MAX && char_is_space(*cur)) {
            do {
                cur++;
            } while (*cur <= U8MAX && char_is_space(*cur));
        }
        if (unlikely(cur < end)) goto fail_garbage;
    }
    return ret;

fail_string:
    return_err(cur, JSONDecodeError, "invalid string");
fail_number:
    return_err(cur, JSONDecodeError, "invalid number");
fail_alloc:
    return_err(cur, PyExc_MemoryError,
               "memory allocation failed");
fail_literal_true:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'true'");
fail_literal_false:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'false'");
fail_literal_null:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'null'");
fail_character:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a valid root value");
fail_garbage:
    return_err(cur, JSONDecodeError,
               "unexpected content after document");
fail_cleanup:
    Py_XDECREF(ret);
    return NULL;
#undef return_err
}

force_inline bool should_loads_pretty(const src_t *buffer, const src_t *end) {
    if (end - buffer > 3) {
        // check if can use pretty read
        src_t second, third;
        second = buffer[1];
        third = buffer[2];
        if (second == '\n' || third == '\n') {
            // likely to hit
            return true;
        }
        if (second <= U8MAX && third <= U8MAX && char_is_space(second) && char_is_space(third)) {
            return true;
        }
    }
    return false;
}

internal_simd_noinline PyObject *decode(DecoderBuffers *decoder_context, PyUnicodeObject *in_unicode, PyObject *object_hook DECODER_TLS_KEYCACHE_ADDITIONAL_ARGDEF) {
    // some checks
    assert(in_unicode);
    PyASCIIObject *ascii_head = ssrjson_pyascii_cast(in_unicode);
    assert((ascii_head->state.ascii ? 0 : ascii_head->state.kind) == COMPILE_UCS_LEVEL);
    if (unlikely(!ascii_head->length)) {
        PyErr_Format(JSONDecodeError, "input data is empty");
        return NULL;
    }
#if COMPILE_UCS_LEVEL > 0
    const src_t *buffer = ssrjson_cast(src_t *, ssrjson_pycompactunicode_cast(in_unicode) + 1);
#else
    const src_t *buffer = ssrjson_cast(src_t *, ascii_head + 1);
#endif
    assert(buffer);
    assert(ascii_head->length > 0);

    const src_t *const end = buffer + ascii_head->length;
    PyObject *ret;
    assert(*end == 0);

    /* skip empty contents before json document */
    if (unlikely(*buffer <= U8MAX && char_is_space_or_comment(*buffer))) {
        if (likely(*buffer <= U8MAX && char_is_space(*buffer))) {
            while ((*++buffer) <= U8MAX && char_is_space(*buffer));
        }
        if (unlikely(buffer >= end)) {
            PyErr_Format(JSONDecodeError, "input data is empty");
            return NULL;
        }
    }

    /* read json document */
    if (likely(*buffer <= U8MAX && char_is_container(*buffer))) {
        if (should_loads_pretty(buffer, end)) {
            ret = loads_root_pretty(decoder_context, buffer, end - buffer, object_hook DECODER_TLS_KEYCACHE_ADDITIONAL_ARG);
        } else {
            ret = loads_root_minify(decoder_context, buffer, end - buffer, object_hook DECODER_TLS_KEYCACHE_ADDITIONAL_ARG);
        }
    } else {
        ret = loads_root_single(decoder_context, buffer, end - buffer DECODER_TLS_KEYCACHE_ADDITIONAL_ARG);
    }

    return ret;
}

#undef COMPILE_READ_UCS_LEVEL
#include "compile_context/sr_out.inl.h"

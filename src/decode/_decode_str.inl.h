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

#ifdef SSRJSON_CLANGD_DUMMY
#    ifndef COMPILE_SIMD_BITS
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

force_inline bool check_and_reserve_str_buffer(Py_ssize_t len, _src_t **buffer_head_addr, bool *need_dealloc) {
    // consider the max length of the buffer we need
    // assume that each string has an escape of ucs4, we need buffer with size
    // sizeof(ucs4) * len == 4 * len
    // reserve additional TAIL_PADDING bytes before and after the buffer for convenience,
    // i.e. additional 2 * TAIL_PADDING bytes
    if (len > ((Py_ssize_t)PY_SSIZE_T_MAX - TAIL_PADDING * 2) / 4) {
        return false;
    }
    static_assert(((Py_ssize_t)SSRJSON_STRING_BUFFER_SIZE - TAIL_PADDING * 2) > 4, "((Py_ssize_t)SSRJSON_STRING_BUFFER_SIZE - 128) > 4");
    Py_ssize_t new_buffer_size = 4 * len + 2 * TAIL_PADDING;
    if (new_buffer_size > SSRJSON_STRING_BUFFER_SIZE) {
        // malloc new buffer
        u8 *new_buffer = (u8 *)malloc(new_buffer_size);
        if (!new_buffer) return false;
        *buffer_head_addr = (_src_t *)(new_buffer + TAIL_PADDING);
        *need_dealloc = true;
    } else {
        *buffer_head_addr = (_src_t *)(_DecodeTempBuffer + TAIL_PADDING);
        *need_dealloc = false;
    }
    return true;
}

/** Read single value JSON document. */
internal_simd_noinline PyObject *decode_root_single(const _src_t *dat, Py_ssize_t len) {
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
    const _src_t *cur = dat;
    const _src_t *const end = cur + len;

    PyObject *ret = NULL;

    if (*cur <= U8MAX && char_is_number(*cur)) {
        ret = read_number(&cur, end);
        if (likely(ret)) goto single_end;
        goto fail_number;
    }
    if (*cur == '"') {
        // u8 *write_buffer;
        _src_t *string_buffer_head;
        bool need_dealloc = false;
        check_and_reserve_str_buffer(len, &string_buffer_head, &need_dealloc);
        cur++;
        ret = decode_str(&cur, end, string_buffer_head, false);
        if (need_dealloc) {
            free((void *)((u8 *)string_buffer_head - TAIL_PADDING));
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
        ret = read_inf_or_nan(false, &cur, end);
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

force_inline bool should_read_pretty(const _src_t *buffer, const _src_t *end) {
    if (end - buffer > 3) {
        // check if can use pretty read
        _src_t second, third;
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

internal_simd_noinline PyObject *decode(PyUnicodeObject *in_unicode) {
    // some checks
    assert(in_unicode);
    PyASCIIObject *ascii_head = SSRJSON_CAST(PyASCIIObject *, in_unicode);
    assert((ascii_head->state.ascii ? 0 : ascii_head->state.kind) == COMPILE_UCS_LEVEL);
    if (unlikely(!ascii_head->length)) {
        PyErr_Format(JSONDecodeError, "input data is empty");
        return NULL;
    }
#if COMPILE_UCS_LEVEL > 0
    const _src_t *buffer = SSRJSON_CAST(_src_t *, SSRJSON_CAST(PyCompactUnicodeObject *, in_unicode) + 1);
#else
    const _src_t *buffer = SSRJSON_CAST(_src_t *, ascii_head + 1);
#endif
    assert(buffer);
    assert(ascii_head->length > 0);

    const _src_t *const end = buffer + ascii_head->length;
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
        if (should_read_pretty(buffer, end)) {
            ret = decode_root_pretty(buffer, end - buffer);
        } else {
            ret = decode_root_minify(buffer, end - buffer);
        }
    } else {
        ret = decode_root_single(buffer, end - buffer);
    }

    return ret;
}

#undef COMPILE_READ_UCS_LEVEL
#include "compile_context/sr_out.inl.h"

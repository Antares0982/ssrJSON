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

/*
 * Required macros:
 *   READ_ROOT_IMPL, points to the function name
 *   DECODE_READ_PRETTY, true/false
 */
#ifdef SSRJSON_CLANGD_DUMMY
#    ifndef DECODE_READ_PRETTY
#        define COMPILE_CONTEXT_DECODE 1
#        include "decode/decode_float_wrap.h"
#        include "decode/decode_shared.h"
#        include "decode/str/tools.h"
#        include "simd/simd_impl.h"
#        define DECODE_READ_PRETTY 1
#        include "simd/compile_feature_check.h"
#    endif
#endif

//
#include "compile_context/s_in.inl.h"

#define SKIP_CONSECUTIVE_SPACES(_u8ptr)   \
    do {                                  \
        do {                              \
            _u8ptr++;                     \
        } while (char_is_space(*_u8ptr)); \
    } while (0)

internal_simd_noinline PyObject *read_bytes_not_key(const u8 **ptr, u8 *write_buffer);

internal_simd_noinline PyObject *READ_ROOT_IMPL(const u8 *dat, usize len) {
    // container stack info
    DecodeCtnWithSize *ctn = NULL;
    DecodeCtnWithSize *ctn_start = NULL;
    DecodeCtnWithSize *ctn_end = NULL;
    // object stack info
    decode_obj_stack_ptr_t decode_obj_writer = NULL;
    decode_obj_stack_ptr_t decode_obj_stack = NULL;
    decode_obj_stack_ptr_t decode_obj_stack_end = NULL;
    // init
    if (!init_decode_ctn_stack_info(&ctn_start, &ctn, &ctn_end) || !init_decode_obj_stack_info(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end)) goto failed_cleanup;
    u8 *string_buffer_head = (u8 *)_DecodeTempBuffer;

    //
    if (unlikely(len > ((size_t)(-1)) / 4)) {
        goto fail_alloc;
    }
    if (unlikely(4 * len > SSRJSON_STRING_BUFFER_SIZE)) {
        string_buffer_head = malloc(4 * len);
        if (!string_buffer_head) goto fail_alloc;
    }
    //
    const u8 *cur = (const u8 *)dat;
    const u8 *const end = (const u8 *)dat + len;

    if (*cur++ == '{') {
        set_decode_ctn(ctn, 0, false);
        if (DECODE_READ_PRETTY && *cur == '\n') cur++;
        goto obj_key_begin;
    } else {
        set_decode_ctn(ctn, 0, true);
        if (DECODE_READ_PRETTY && *cur == '\n') cur++;
        goto arr_val_begin;
    }

arr_begin:
    /* save current container */
    /* create a new array value, save parent container offset */
    if (unlikely(!ctn_grow_check(&ctn, ctn_end))) goto fail_ctn_grow;
    set_decode_ctn(ctn, 0, true);

    /* push the new array value as current container */
    if (DECODE_READ_PRETTY && *cur == '\n') cur++;

arr_val_begin:
#if DECODE_READ_PRETTY
    if (*cur == ' ') {
        fast_skip_spaces_u8(&cur, end);
    }

#endif
    if (*cur == '{') {
        cur++;
        goto obj_begin;
    }
    if (*cur == '[') {
        cur++;
        goto arr_begin;
    }
    if (char_is_number(*cur)) {
        PyObject *number_obj = read_number_u8(&cur, end);
        if (likely(number_obj && push_obj(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end, number_obj))) {
            incr_decode_ctn_size(ctn);
            goto arr_val_end;
        }
        goto fail_number;
    }
    if (*cur == '"') {
        PyObject *str_obj = read_bytes_not_key(&cur, string_buffer_head);
        if (likely(str_obj && push_obj(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end, str_obj))) {
            incr_decode_ctn_size(ctn);
            goto arr_val_end;
        }
        goto fail_string;
    }
    if (*cur == 't') {
        if (likely(_read_true_u8(&cur, end) && decode_true(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end))) {
            incr_decode_ctn_size(ctn);
            goto arr_val_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_read_false_u8(&cur, end) && decode_false(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end))) {
            incr_decode_ctn_size(ctn);
            goto arr_val_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_read_null_u8(&cur, end) && decode_null(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end))) {
            incr_decode_ctn_size(ctn);
            goto arr_val_end;
        }
        if (likely(_read_nan_u8(&cur, end) && decode_nan(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end, false))) {
            incr_decode_ctn_size(ctn);
            goto arr_val_end;
        }
        goto fail_literal_null;
    }
    if (*cur == ']') {
        cur++;
        if (likely(get_decode_ctn_len(ctn) == 0)) goto arr_end;
        while (*cur != ',') cur--;
        goto fail_trailing_comma;
    }
    if (char_is_space(*cur)) {
        // read pretty:
        //   the ",\n" and white spaces after them are all read out,
        //   this case is unlikely.
        // read minify:
        //   the ", " or "," is read out, this case is unlikely
        // guess it occurs when the document is using some `CHAR_TYPE_SPACE` characters
        // other than space itself as indent, like, tabs.
        SKIP_CONSECUTIVE_SPACES(cur);
        goto arr_val_begin;
    }
    if ((*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        PyObject *number_obj = read_inf_or_nan_u8(false, &cur, end);
        if (likely(number_obj && push_obj(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end, number_obj))) {
            incr_decode_ctn_size(ctn);
            goto arr_val_end;
        }
        goto fail_character_val;
    }

    goto fail_character_val;

arr_val_end:
#if DECODE_READ_PRETTY
    if (byte_match_2((void *)cur, ",\n")) {
#else
    if (byte_match_2((void *)cur, ", ")) {
#endif
        cur += 2;
        goto arr_val_begin;
    }
    if (*cur == ',') {
        cur++;
        goto arr_val_begin;
    }
    if (*cur == ']') {
        cur++;
        goto arr_end;
    }
    if (char_is_space(*cur)) {
        // unlikely case, we expect a "," or "]" but not found right after the value
        cur++;
        if (*cur == ' ') fast_skip_spaces_u8(&cur, end);
        if (char_is_space(*cur)) {
            SKIP_CONSECUTIVE_SPACES(cur);
        }
        goto arr_val_end;
    }

    goto fail_character_arr_end;

arr_end:
    assert(decode_ctn_is_arr(ctn));
    if (!decode_arr(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end, get_decode_ctn_len(ctn))) goto failed_cleanup;
    /* pop parent as current container */
    if (unlikely(ctn-- == ctn_start)) {
        goto doc_end;
    }

    incr_decode_ctn_size(ctn);
    if (DECODE_READ_PRETTY && *cur == '\n') cur++;
    if (!decode_ctn_is_arr(ctn)) {
        goto obj_val_end;
    } else {
        goto arr_val_end;
    }

obj_begin:
    /* push container */
    if (unlikely(!ctn_grow_check(&ctn, ctn_end))) goto fail_ctn_grow;
    set_decode_ctn(ctn, 0, false);
    if (DECODE_READ_PRETTY && *cur == '\n') cur++;

obj_key_begin:
#if DECODE_READ_PRETTY
    if (*cur == ' ') {
        fast_skip_spaces_u8(&cur, end);
    }
#endif

    if (likely(*cur == '"')) {
        PyObject *str_obj = read_bytes(&cur, string_buffer_head, true);
        if (likely(str_obj && push_obj(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end, str_obj))) {
            goto obj_key_end;
        }
        goto fail_string;
    }
    if (likely(*cur == '}')) {
        cur++;
        if (likely(get_decode_ctn_len(ctn) == 0)) goto obj_end;
        goto fail_trailing_comma;
    }
    if (char_is_space(*cur)) {
        // for both read pretty and minify:
        //   likely occurs when the document is using some `CHAR_TYPE_SPACE` characters
        //   other than space as indent.
        //   see the comment in `arr_val_begin` for more details.
        SKIP_CONSECUTIVE_SPACES(cur);
        goto obj_key_begin;
    }
    goto fail_character_obj_key;

obj_key_end:
    if (byte_match_2((void *)cur, ": ")) {
        cur += 2;
        goto obj_val_begin;
    }
    if (*cur == ':') {
        cur++;
        goto obj_val_begin;
    }
    if (char_is_space(*cur)) {
        // unlikely case, we expect a colon here
        cur++;
        if (*cur == ' ') fast_skip_spaces_u8(&cur, end);
        if (char_is_space(*cur)) {
            SKIP_CONSECUTIVE_SPACES(cur);
        }
        goto obj_key_end;
    }
    goto fail_character_obj_sep;

obj_val_begin:
    if (*cur == '"') {
        PyObject *str_obj = read_bytes_not_key(&cur, string_buffer_head);
        if (likely(str_obj && push_obj(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end, str_obj))) {
            incr_decode_ctn_size(ctn);
            goto obj_val_end;
        }
        goto fail_string;
    }
    if (char_is_number(*cur)) {
        PyObject *number_obj = read_number_u8(&cur, end);
        if (likely(number_obj && push_obj(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end, number_obj))) {
            incr_decode_ctn_size(ctn);
            goto obj_val_end;
        }
        goto fail_number;
    }
    if (*cur == '{') {
        cur++;
        goto obj_begin;
    }
    if (*cur == '[') {
        cur++;
        goto arr_begin;
    }
    if (*cur == 't') {
        if (likely(_read_true_u8(&cur, end) && decode_true(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end))) {
            incr_decode_ctn_size(ctn);
            goto obj_val_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_read_false_u8(&cur, end) && decode_false(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end))) {
            incr_decode_ctn_size(ctn);
            goto obj_val_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_read_null_u8(&cur, end) && decode_null(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end))) {
            incr_decode_ctn_size(ctn);
            goto obj_val_end;
        }
        if (likely(_read_nan_u8(&cur, end) && decode_nan(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end, false))) {
            incr_decode_ctn_size(ctn);
            goto obj_val_end;
        }
        goto fail_literal_null;
    }
    if (char_is_space(*cur)) {
        // read pretty:
        //   the ": " is read out, this character is likely to be "\n", then we should skip spaces before new line
        // read minify:
        //   the ": " or ":" is read out, this is an unlikely case
        cur++;
#if DECODE_READ_PRETTY
        if (*cur == ' ') fast_skip_spaces_u8(&cur, end);
#endif
        if (char_is_space(*cur)) {
            // handle unlikely cases
            SKIP_CONSECUTIVE_SPACES(cur);
        }
        goto obj_val_begin;
    }
    if ((*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        PyObject *number_obj = read_inf_or_nan_u8(false, &cur, end);
        if (likely(number_obj && push_obj(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end, number_obj))) {
            incr_decode_ctn_size(ctn);
            goto obj_val_end;
        }
    }

    goto fail_character_val;

obj_val_end:
#if DECODE_READ_PRETTY
    if (byte_match_2((void *)cur, ",\n")) {
#else
    if (byte_match_2((void *)cur, ", ")) {
#endif
        cur += 2;
        goto obj_key_begin;
    }

    if (likely(*cur == ',')) {
        cur++;
        goto obj_key_begin;
    }
    if (likely(*cur == '}')) {
        cur++;
        goto obj_end;
    }
    if (char_is_space(*cur)) {
        // unlikely case
        cur++;
        if (*cur == ' ') fast_skip_spaces_u8(&cur, end);
        if (char_is_space(*cur)) {
            SKIP_CONSECUTIVE_SPACES(cur);
        }
        goto obj_val_end;
    }

    goto fail_character_obj_end;

obj_end:
    assert(!decode_ctn_is_arr(ctn));
    if (unlikely(!decode_obj(&decode_obj_writer, &decode_obj_stack, &decode_obj_stack_end, get_decode_ctn_len(ctn)))) goto failed_cleanup;
    /* pop container */
    /* point to the next value */
    if (unlikely(ctn-- == ctn_start)) {
        goto doc_end;
    }
    incr_decode_ctn_size(ctn);
    if (DECODE_READ_PRETTY && *cur == '\n') cur++;
    if (decode_ctn_is_arr(ctn)) {
        goto arr_val_end;
    } else {
        goto obj_val_end;
    }

doc_end:
    /* check invalid contents after json document */
    if (unlikely(cur < end)) {
        if (*cur == ' ') fast_skip_spaces_u8(&cur, end);
        if (char_is_space(*cur)) {
            SKIP_CONSECUTIVE_SPACES(cur);
        }
        // while (char_is_space(*cur)) cur++;
        if (unlikely(cur < end)) goto fail_garbage;
    }

success:;
    PyObject *obj = *decode_obj_stack;
    assert(ctn == ctn_start - 1);
    assert(decode_obj_writer == decode_obj_stack + 1);
    assert(obj && !PyErr_Occurred());
    assert(obj->ob_refcnt == 1);
    // free string buffer
    if (unlikely(string_buffer_head != _DecodeTempBuffer)) {
        free(string_buffer_head);
    }
    // free obj stack buffer if allocated dynamically
    if (unlikely(decode_obj_stack_end - decode_obj_stack > SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE)) {
        free(decode_obj_stack);
    }

    return obj;

#define return_err(_pos, _type, _msg)                                                             \
    do {                                                                                          \
        if (_type == JSONDecodeError) {                                                           \
            PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, ((u8 *)_pos) - (u8 *)dat); \
        } else {                                                                                  \
            PyErr_SetString(_type, _msg);                                                         \
        }                                                                                         \
        goto failed_cleanup;                                                                      \
    } while (0)

fail_string:
    return_err(cur, JSONDecodeError, "invalid string");
fail_number:
    return_err(cur, JSONDecodeError, "invalid number");
fail_alloc:
    return_err(cur, PyExc_MemoryError,
               "memory allocation failed");
fail_trailing_comma:
    return_err(cur, JSONDecodeError,
               "trailing comma is not allowed");
fail_literal_true:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'true'");
fail_literal_false:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'false'");
fail_literal_null:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'null'");
fail_character_val:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a valid JSON value");
fail_character_arr_end:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a comma or a closing bracket");
fail_character_obj_key:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a string for object key");
fail_character_obj_sep:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a colon after object key");
fail_character_obj_end:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a comma or a closing brace");
fail_garbage:
    return_err(cur, JSONDecodeError,
               "unexpected content after document");
fail_ctn_grow:
    return_err(cur, JSONDecodeError,
               "max recursion exceeded");

failed_cleanup:
    for (decode_obj_stack_ptr_t obj_ptr = decode_obj_stack; obj_ptr < decode_obj_writer; obj_ptr++) {
        Py_XDECREF(*obj_ptr);
    }
    // free string buffer
    if (unlikely(string_buffer_head != _DecodeTempBuffer)) {
        free(string_buffer_head);
    }
    // free obj stack buffer if allocated dynamically
    if (unlikely(decode_obj_stack_end - decode_obj_stack > SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE)) {
        free(decode_obj_stack);
    }
    return NULL;
#undef return_err
}

#undef SKIP_CONSECUTIVE_SPACES
#include "compile_context/s_out.inl.h"

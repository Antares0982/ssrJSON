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

#include "decode_bytes_root_wrap.h"
#include "decode_float_wrap.h"
#include "decode_shared.h"
#include "simd/memcpy.h"
#include "ssrjson.h"
#include "str/tools.h"
//
#include "simd/compile_feature_check.h"

/**
 Read a JSON string.
 @param ptr The head pointer of string before '"' prefix (inout).
 @param lst JSON last position.
 @param inv Allow invalid unicode.
 @param val The string value to be written.
 @param msg The error message pointer.
 @return Whether success.
 */
force_inline PyObject *read_bytes(const u8 **ptr, u8 *write_buffer, bool is_key) {
    /*
     Each unicode code point is encoded as 1 to 4 bytes in UTF-8 encoding,
     we use 4-byte mask and pattern value to validate UTF-8 byte sequence,
     this requires the input data to have 4-byte zero padding.
     ---------------------------------------------------
     1 byte
     unicode range [U+0000, U+007F]
     unicode min   [.......0]
     unicode max   [.1111111]
     bit pattern   [0.......]
     ---------------------------------------------------
     2 byte
     unicode range [U+0080, U+07FF]
     unicode min   [......10 ..000000]
     unicode max   [...11111 ..111111]
     bit require   [...xxxx. ........] (1E 00)
     bit mask      [xxx..... xx......] (E0 C0)
     bit pattern   [110..... 10......] (C0 80)
     ---------------------------------------------------
     3 byte
     unicode range [U+0800, U+FFFF]
     unicode min   [........ ..100000 ..000000]
     unicode max   [....1111 ..111111 ..111111]
     bit require   [....xxxx ..x..... ........] (0F 20 00)
     bit mask      [xxxx.... xx...... xx......] (F0 C0 C0)
     bit pattern   [1110.... 10...... 10......] (E0 80 80)
     ---------------------------------------------------
     3 byte invalid (reserved for surrogate halves)
     unicode range [U+D800, U+DFFF]
     unicode min   [....1101 ..100000 ..000000]
     unicode max   [....1101 ..111111 ..111111]
     bit mask      [....xxxx ..x..... ........] (0F 20 00)
     bit pattern   [....1101 ..1..... ........] (0D 20 00)
     ---------------------------------------------------
     4 byte
     unicode range [U+10000, U+10FFFF]
     unicode min   [........ ...10000 ..000000 ..000000]
     unicode max   [.....100 ..001111 ..111111 ..111111]
     bit require   [.....xxx ..xx.... ........ ........] (07 30 00 00)
     bit mask      [xxxxx... xx...... xx...... xx......] (F8 C0 C0 C0)
     bit pattern   [11110... 10...... 10...... 10......] (F0 80 80 80)
     ---------------------------------------------------
     */
#if PY_BIG_ENDIAN
    const u32 b1_mask = 0x80000000UL;
    const u32 b1_patt = 0x00000000UL;
    const u32 b2_mask = 0xE0C00000UL;
    const u32 b2_patt = 0xC0800000UL;
    const u32 b2_requ = 0x1E000000UL;
    const u32 b3_mask = 0xF0C0C000UL;
    const u32 b3_patt = 0xE0808000UL;
    const u32 b3_requ = 0x0F200000UL;
    const u32 b3_erro = 0x0D200000UL;
    const u32 b4_mask = 0xF8C0C0C0UL;
    const u32 b4_patt = 0xF0808080UL;
    const u32 b4_requ = 0x07300000UL;
    const u32 b4_err0 = 0x04000000UL;
    const u32 b4_err1 = 0x03300000UL;
#else
    const u32 b1_mask = 0x00000080UL;
    const u32 b1_patt = 0x00000000UL;
    const u32 b2_mask = 0x0000C0E0UL;
    const u32 b2_patt = 0x000080C0UL;
    const u32 b2_requ = 0x0000001EUL;
    const u32 b3_mask = 0x00C0C0F0UL;
    const u32 b3_patt = 0x008080E0UL;
    const u32 b3_requ = 0x0000200FUL;
    const u32 b3_erro = 0x0000200DUL;
    const u32 b4_mask = 0xC0C0C0F8UL;
    const u32 b4_patt = 0x808080F0UL;
    const u32 b4_requ = 0x00003007UL;
    const u32 b4_err0 = 0x00000004UL;
    const u32 b4_err1 = 0x00003003UL;
#endif

#define is_valid_seq_1(uni) ( \
        ((uni & b1_mask) == b1_patt))

#define is_valid_seq_2(uni) (           \
        ((uni & b2_mask) == b2_patt) && \
        ((uni & b2_requ)))

#define is_valid_seq_3(uni) (           \
        ((uni & b3_mask) == b3_patt) && \
        ((tmp = (uni & b3_requ))) &&    \
        ((tmp != b3_erro)))

#define is_valid_seq_4(uni) (           \
        ((uni & b4_mask) == b4_patt) && \
        ((tmp = (uni & b4_requ))) &&    \
        ((tmp & b4_err0) == 0 || (tmp & b4_err1) == 0))

#define return_err(_end, _msg)                                                        \
    do {                                                                              \
        PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, _end - src_start); \
        return NULL;                                                                  \
    } while (0)

    u8 *cur = (u8 *)*ptr;
    // u8 **end = (u8 **)ptr;

    // u8 *src = ++cur, *dst, *pos;
    u8 *src = ++cur, *pos;

    u16 hi, lo;
    u32 uni, tmp;

    u8 *const src_start = src;
    size_t len_ucs1 = 0, len_ucs2 = 0, len_ucs4 = 0;
    u8 *temp_string_buf = write_buffer; //(u8*)*buffer;
    u8 *dst = temp_string_buf;
    u8 cur_max_ucs_size = 1;
    u16 *dst_ucs2;
    u32 *dst_ucs4;
    Py_ssize_t final_string_length;
    int final_type_flag;
    bool is_ascii = true;


skip_ascii:
    /* Most strings have no escaped characters, so we can jump them quickly. */

skip_ascii_begin:
    /*
     We want to make loop unrolling, as shown in the following code. Some
     compiler may not generate instructions as expected, so we rewrite it with
     explicit goto statements. We hope the compiler can generate instructions
     like this: https://godbolt.org/z/8vjsYq
     
         while (true) repeat16({
            if (likely(!(char_is_ascii_stop(*src)))) src++;
            else break;
         })
     */
#define expr_jump(i)                           \
    if (likely(!char_is_ascii_stop(src[i]))) { \
    } else                                     \
        goto skip_ascii_stop##i;

#define expr_stop(i)               \
    skip_ascii_stop##i : src += i; \
    goto skip_ascii_end;

    REPEAT_INCR_16(expr_jump)
    src += 16;
    goto skip_ascii_begin;
    REPEAT_INCR_16(expr_stop)

#undef expr_jump
#undef expr_stop

skip_ascii_end:

    /*
     GCC may store src[i] in a register at each line of expr_jump(i) above.
     These instructions are useless and will degrade performance.
     This inline asm is a hint for gcc: "the memory has been modified,
     do not cache it".
     
     MSVC, Clang, ICC can generate expected instructions without this hint.
     */
#if SSRJSON_IS_REAL_GCC
    __asm__ volatile("" : "=m"(*src));
#endif
    if (likely(*src == '"')) {

        // this is a fast path for ascii strings. directly copy the buffer to pyobject
        *ptr = src + 1;
        return make_string(src_start, src - src_start, SSRJSON_STRING_TYPE_ASCII, is_key);
    } else if (src != src_start) {
        ssrjson_memcpy(temp_string_buf, src_start, src - src_start);
        len_ucs1 = src - src_start;
        dst += len_ucs1;
    }
    goto copy_utf8_ucs1;

copy_escape_ucs1:
    if (likely(*src == '\\')) {

        switch (*++src) {
            case '"':
                *dst++ = '"';
                src++;
                break;
            case '\\':
                *dst++ = '\\';
                src++;
                break;
            case '/':
                *dst++ = '/';
                src++;
                break;
            case 'b':
                *dst++ = '\b';
                src++;
                break;
            case 'f':
                *dst++ = '\f';
                src++;
                break;
            case 'n':
                *dst++ = '\n';
                src++;
                break;
            case 'r':
                *dst++ = '\r';
                src++;
                break;
            case 't':
                *dst++ = '\t';
                src++;
                break;
            case 'u':
                if (unlikely(!read_to_hex_u8(++src, &hi))) {
                    return_err(src - 2, "invalid escaped sequence in string");
                }
                src += 4;
                if (likely((hi & 0xF800) != 0xD800)) {
                    /* a BMP character */
                    if (hi >= 0x100) {
                        // ucs1 -> ucs2
                        assert(cur_max_ucs_size == 1);
                        len_ucs1 = dst - (u8 *)temp_string_buf;
                        dst_ucs2 = ((u16 *)temp_string_buf) + len_ucs1;
                        cur_max_ucs_size = 2;
                        *dst_ucs2++ = hi;
                        goto copy_ascii_ucs2;
                    } else {
                        if (hi >= 0x80) is_ascii = false; // latin1
                        *dst++ = (u8)hi;
                        goto copy_ascii_ucs1;
                    }
                } else {
                    /* a non-BMP character, represented as a surrogate pair */
                    if (unlikely((hi & 0xFC00) != 0xD800)) {
                        return_err(src - 6, "invalid high surrogate in string");
                    }
                    if (unlikely(!byte_match_2(src, "\\u"))) {
                        return_err(src, "no low surrogate in string");
                    }
                    if (unlikely(!read_to_hex_u8(src + 2, &lo))) {
                        return_err(src, "invalid escaped sequence in string");
                    }
                    if (unlikely((lo & 0xFC00) != 0xDC00)) {
                        return_err(src, "invalid low surrogate in string");
                    }
                    uni = ((((u32)hi - 0xD800) << 10) |
                           ((u32)lo - 0xDC00)) +
                          0x10000;
                    // ucs1 -> ucs4
                    assert(cur_max_ucs_size == 1);
                    len_ucs1 = dst - (u8 *)temp_string_buf;
                    dst_ucs4 = ((u32 *)temp_string_buf) + len_ucs1;
                    cur_max_ucs_size = 4;
                    *dst_ucs4++ = uni;
                    src += 6;
                    goto copy_ascii_ucs4;
                }
            default:
                return_err(src, "invalid escaped character in string");
        }
        goto copy_ascii_ucs1;
    } else if (likely(*src == '"')) {
        goto read_finalize;
    } else {
        return_err(src, "unexpected control character in string");
    }

copy_ascii_ucs1:
    /*
     Copy continuous ASCII, loop unrolling, same as the following code:
     
         while (true) repeat16({
            if (unlikely(char_is_ascii_stop(*src))) break;
            *dst++ = *src++;
         })
     */
#if SSRJSON_IS_REAL_GCC
#    define expr_jump(i)                             \
        if (likely(!(char_is_ascii_stop(src[i])))) { \
        } else {                                     \
            __asm__ volatile("" : "=m"(src[i]));     \
            goto copy_ascii_ucs1_stop_##i;           \
        }
#else
#    define expr_jump(i)                             \
        if (likely(!(char_is_ascii_stop(src[i])))) { \
        } else {                                     \
            goto copy_ascii_ucs1_stop_##i;           \
        }
#endif
    REPEAT_INCR_16(expr_jump)
#undef expr_jump

    memcpy(dst, src, 16);
    src += 16;
    dst += 16;
    goto copy_ascii_ucs1;

    /*
     The memory will be moved forward by at least 1 byte. So the `byte_move`
     can be one byte more than needed to reduce the number of instructions.
     */
copy_ascii_ucs1_stop_0:
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_1:
    byte_move_2(dst, src);
    src += 1;
    dst += 1;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_2:
    byte_move_2(dst, src);
    src += 2;
    dst += 2;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_3:
    byte_move_4(dst, src);
    src += 3;
    dst += 3;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_4:
    byte_move_4(dst, src);
    src += 4;
    dst += 4;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_5:
    byte_move_4(dst, src);
    byte_move_2(dst + 4, src + 4);
    src += 5;
    dst += 5;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_6:
    byte_move_4(dst, src);
    byte_move_2(dst + 4, src + 4);
    src += 6;
    dst += 6;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_7:
    byte_move_8(dst, src);
    src += 7;
    dst += 7;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_8:
    byte_move_8(dst, src);
    src += 8;
    dst += 8;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_9:
    byte_move_8(dst, src);
    byte_move_2(dst + 8, src + 8);
    src += 9;
    dst += 9;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_10:
    byte_move_8(dst, src);
    byte_move_2(dst + 8, src + 8);
    src += 10;
    dst += 10;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_11:
    byte_move_8(dst, src);
    byte_move_4(dst + 8, src + 8);
    src += 11;
    dst += 11;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_12:
    byte_move_8(dst, src);
    byte_move_4(dst + 8, src + 8);
    src += 12;
    dst += 12;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_13:
    byte_move_8(dst, src);
    byte_move_4(dst + 8, src + 8);
    byte_move_2(dst + 12, src + 12);
    src += 13;
    dst += 13;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_14:
    byte_move_8(dst, src);
    byte_move_4(dst + 8, src + 8);
    byte_move_2(dst + 12, src + 12);
    src += 14;
    dst += 14;
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_15:
    memcpy(dst, src, 16);
    src += 15;
    dst += 15;
    goto copy_utf8_ucs1;

copy_utf8_ucs1:
    assert(cur_max_ucs_size == 1);
    if (*src & 0x80) { /* non-ASCII character */
    copy_utf8_inner_ucs1:
        pos = src;
        uni = byte_load_4(src);
        if (is_valid_seq_1(uni)) goto copy_ascii_ucs1;
        if (is_valid_seq_3(uni)) {
            // code point: [U+0800, U+FFFF]
            // ucs1 -> ucs2
            assert(cur_max_ucs_size == 1);
            len_ucs1 = dst - (u8 *)temp_string_buf;
            dst_ucs2 = ((u16 *)temp_string_buf) + len_ucs1;
            cur_max_ucs_size = 2;
            *dst_ucs2++ = read_b3_unicode(uni);
            src += 3;
            goto copy_utf8_inner_ucs2;
        }
        while (is_valid_seq_2(uni)) {
            assert(cur_max_ucs_size == 1);
            u16 to_write = read_b2_unicode(uni);
            if (likely(to_write >= 0x100)) {
                // ucs1 -> ucs2
                assert(cur_max_ucs_size == 1);
                len_ucs1 = dst - (u8 *)temp_string_buf;
                dst_ucs2 = ((u16 *)temp_string_buf) + len_ucs1;
                cur_max_ucs_size = 2;
                *dst_ucs2++ = to_write;
                src += 2;
                goto copy_utf8_inner_ucs2;
            } else {
                is_ascii = false;
                *dst++ = (u8)to_write;
                src += 2;
                // still ascii, no need goto
                uni = byte_load_4(src);
            }
        }
        if (is_valid_seq_4(uni)) {
            // code point: [U+10000, U+10FFFF]
            // must be ucs4
            // BEGIN ucs1 -> ucs4
            assert(cur_max_ucs_size == 1);
            len_ucs1 = dst - (u8 *)temp_string_buf;
            dst_ucs4 = ((u32 *)temp_string_buf) + len_ucs1;
            cur_max_ucs_size = 4;
            *dst_ucs4++ = read_b4_unicode(uni);
            src += 4;
            goto copy_utf8_inner_ucs4;
        }

        if (unlikely(pos == src)) {
            return_err(src, "invalid UTF-8 encoding in string");
        }
        goto copy_ascii_ucs1;
    }
    goto copy_escape_ucs1;

copy_escape_ucs2:
    assert(cur_max_ucs_size == 2);
    if (likely(*src == '\\')) {
        switch (*++src) {
            case '"':
                *dst_ucs2++ = '"';
                src++;
                break;
            case '\\':
                *dst_ucs2++ = '\\';
                src++;
                break;
            case '/':
                *dst_ucs2++ = '/';
                src++;
                break;
            case 'b':
                *dst_ucs2++ = '\b';
                src++;
                break;
            case 'f':
                *dst_ucs2++ = '\f';
                src++;
                break;
            case 'n':
                *dst_ucs2++ = '\n';
                src++;
                break;
            case 'r':
                *dst_ucs2++ = '\r';
                src++;
                break;
            case 't':
                *dst_ucs2++ = '\t';
                src++;
                break;
            case 'u':
                if (unlikely(!read_to_hex_u8(++src, &hi))) {
                    return_err(src - 2, "invalid escaped sequence in string");
                }
                src += 4;
                if (likely((hi & 0xF800) != 0xD800)) {
                    /* a BMP character */
                    *dst_ucs2++ = hi;
                    goto copy_ascii_ucs2;
                } else {
                    /* a non-BMP character, represented as a surrogate pair */
                    if (unlikely((hi & 0xFC00) != 0xD800)) {
                        return_err(src - 6, "invalid high surrogate in string");
                    }
                    if (unlikely(!byte_match_2(src, "\\u"))) {
                        return_err(src, "no low surrogate in string");
                    }
                    if (unlikely(!read_to_hex_u8(src + 2, &lo))) {
                        return_err(src, "invalid escaped sequence in string");
                    }
                    if (unlikely((lo & 0xFC00) != 0xDC00)) {
                        return_err(src, "invalid low surrogate in string");
                    }
                    uni = ((((u32)hi - 0xD800) << 10) |
                           ((u32)lo - 0xDC00)) +
                          0x10000;
                    // ucs2 -> ucs4
                    assert(cur_max_ucs_size == 2);
                    len_ucs2 = dst_ucs2 - (u16 *)temp_string_buf - len_ucs1;
                    dst_ucs4 = ((u32 *)temp_string_buf) + len_ucs1 + len_ucs2;
                    cur_max_ucs_size = 4;
                    *dst_ucs4++ = uni;
                    src += 6;
                    goto copy_ascii_ucs4;
                }
            default:
                return_err(src, "invalid escaped character in string");
        }
        goto copy_ascii_ucs2;
    } else if (likely(*src == '"')) {
        goto read_finalize;
    } else {
        return_err(src, "unexpected control character in string");
    }

copy_ascii_ucs2:
    assert(cur_max_ucs_size == 2);
    while (true) REPEAT_CALL_16({
        if (unlikely(char_is_ascii_stop(*src))) break;
        *dst_ucs2++ = *src++;
    })

copy_utf8_ucs2:
    assert(cur_max_ucs_size == 2);

    if (*src & 0x80) { /* non-ASCII character */
    copy_utf8_inner_ucs2:
        pos = src;
        uni = byte_load_4(src);
        while (is_valid_seq_3(uni)) {
            // code point: [U+0800, U+FFFF]
            assert(cur_max_ucs_size == 2);
            *dst_ucs2++ = read_b3_unicode(uni);
            src += 3;
            uni = byte_load_4(src);
        }
        if (is_valid_seq_1(uni)) goto copy_ascii_ucs2;
        while (is_valid_seq_2(uni)) {
            assert(cur_max_ucs_size == 2);
            u16 to_write = read_b2_unicode(uni);
            *dst_ucs2++ = to_write;
            src += 2;
            uni = byte_load_4(src);
        }
        if (is_valid_seq_4(uni)) {
            // code point: [U+10000, U+10FFFF]
            // must be ucs4
            // ucs2 -> ucs4
            assert(cur_max_ucs_size == 2);
            len_ucs2 = dst_ucs2 - (u16 *)temp_string_buf - len_ucs1;
            dst_ucs4 = ((u32 *)temp_string_buf) + len_ucs1 + len_ucs2;
            cur_max_ucs_size = 4;
            *dst_ucs4++ = read_b4_unicode(uni);
            src += 4;
            goto copy_utf8_inner_ucs4;
        }


        if (unlikely(pos == src)) {
            return_err(src, "invalid UTF-8 encoding in string");
        }
        goto copy_ascii_ucs2;
    }
    goto copy_escape_ucs2;

copy_escape_ucs4:
    assert(cur_max_ucs_size == 4);
    if (likely(*src == '\\')) {
        switch (*++src) {
            case '"':
                *dst_ucs4++ = '"';
                src++;
                break;
            case '\\':
                *dst_ucs4++ = '\\';
                src++;
                break;
            case '/':
                *dst_ucs4++ = '/';
                src++;
                break;
            case 'b':
                *dst_ucs4++ = '\b';
                src++;
                break;
            case 'f':
                *dst_ucs4++ = '\f';
                src++;
                break;
            case 'n':
                *dst_ucs4++ = '\n';
                src++;
                break;
            case 'r':
                *dst_ucs4++ = '\r';
                src++;
                break;
            case 't':
                *dst_ucs4++ = '\t';
                src++;
                break;
            case 'u':
                if (unlikely(!read_to_hex_u8(++src, &hi))) {
                    return_err(src - 2, "invalid escaped sequence in string");
                }
                src += 4;
                if (likely((hi & 0xF800) != 0xD800)) {
                    /* a BMP character */
                    *dst_ucs4++ = hi;
                    goto copy_ascii_ucs4;
                } else {
                    /* a non-BMP character, represented as a surrogate pair */
                    if (unlikely((hi & 0xFC00) != 0xD800)) {
                        return_err(src - 6, "invalid high surrogate in string");
                    }
                    if (unlikely(!byte_match_2(src, "\\u"))) {
                        return_err(src, "no low surrogate in string");
                    }
                    if (unlikely(!read_to_hex_u8(src + 2, &lo))) {
                        return_err(src, "invalid escaped sequence in string");
                    }
                    if (unlikely((lo & 0xFC00) != 0xDC00)) {
                        return_err(src, "invalid low surrogate in string");
                    }
                    uni = ((((u32)hi - 0xD800) << 10) |
                           ((u32)lo - 0xDC00)) +
                          0x10000;
                    // ucs2 -> ucs4
                    *dst_ucs4++ = uni;
                    src += 6;
                    goto copy_ascii_ucs4;
                }
            default:
                return_err(src, "invalid escaped character in string");
        }
        goto copy_ascii_ucs4;
    } else if (likely(*src == '"')) {
        goto read_finalize;
    } else {
        return_err(src, "unexpected control character in string");
    }

copy_ascii_ucs4:
    assert(cur_max_ucs_size == 4);
    while (true) REPEAT_CALL_16({
        if (unlikely(char_is_ascii_stop(*src))) break;
        *dst_ucs4++ = *src++;
    })

copy_utf8_ucs4:
    assert(cur_max_ucs_size == 4);

    if (*src & 0x80) { /* non-ASCII character */
    copy_utf8_inner_ucs4:
        pos = src;
        uni = byte_load_4(src);
        while (is_valid_seq_3(uni)) {
            // code point: [U+0800, U+FFFF]
            assert(cur_max_ucs_size == 4);
            *dst_ucs4++ = read_b3_unicode(uni);
            src += 3;
            uni = byte_load_4(src);
        }
        if (is_valid_seq_1(uni)) goto copy_ascii_ucs4;
        while (is_valid_seq_2(uni)) {
            assert(cur_max_ucs_size == 4);
            *dst_ucs4++ = read_b2_unicode(uni);
            src += 2;
            uni = byte_load_4(src);
        }
        while (is_valid_seq_4(uni)) {
            // code point: [U+10000, U+10FFFF]
            // must be ucs4
            *dst_ucs4++ = read_b4_unicode(uni);
            src += 4;
            uni = byte_load_4(src);
        }
        if (unlikely(pos == src)) {
            return_err(src, "invalid UTF-8 encoding in string");
        }
        goto copy_ascii_ucs4;
    }
    goto copy_escape_ucs4;

read_finalize:
    *ptr = src + 1;
    if (unlikely(cur_max_ucs_size == 4)) {
        u32 *start = (u32 *)temp_string_buf + len_ucs1 + len_ucs2 - 1;
        u16 *ucs2_back = (u16 *)temp_string_buf + len_ucs1 + len_ucs2 - 1;
        u8 *ucs1_back = (u8 *)temp_string_buf + len_ucs1 - 1;
        while (len_ucs2) {
            *start-- = *ucs2_back--;
            len_ucs2--;
        }
        while (len_ucs1) {
            *start-- = *ucs1_back--;
            len_ucs1--;
        }
        final_string_length = dst_ucs4 - (u32 *)temp_string_buf;
        final_type_flag = SSRJSON_STRING_TYPE_UCS4;
    } else if (unlikely(cur_max_ucs_size == 2)) {
        u16 *start = (u16 *)temp_string_buf + len_ucs1 - 1;
        u8 *ucs1_back = (u8 *)temp_string_buf + len_ucs1 - 1;
        while (len_ucs1) {
            *start-- = *ucs1_back--;
            len_ucs1--;
        }
        final_string_length = dst_ucs2 - (u16 *)temp_string_buf;
        final_type_flag = SSRJSON_STRING_TYPE_UCS2;
    } else {
        final_string_length = dst - (u8 *)temp_string_buf;
        final_type_flag = is_ascii ? SSRJSON_STRING_TYPE_ASCII : SSRJSON_STRING_TYPE_LATIN1;
    }

    return make_string(temp_string_buf, final_string_length, final_type_flag, is_key);

#undef return_err
#undef is_valid_seq_1
#undef is_valid_seq_2
#undef is_valid_seq_3
#undef is_valid_seq_4
}

internal_simd_noinline PyObject *read_bytes_not_key(const u8 **ptr, u8 *write_buffer) {
    return read_bytes(ptr, write_buffer, false);
}

/** Read single value JSON document. */
internal_simd_noinline PyObject *read_root_single_bytes(const u8 *dat, usize len) {
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
        ret = read_number_u8(&cur, end);
        if (likely(ret)) goto single_end;
        goto fail_number;
    }
    if (*cur == '"') {
        u8 *write_buffer;
        bool dynamic = false;
        if (unlikely(4 * len > SSRJSON_STRING_BUFFER_SIZE)) {
            write_buffer = malloc(4 * len);
            if (unlikely(!write_buffer)) goto fail_alloc;
            dynamic = true;
        } else {
            write_buffer = _DecodeTempBuffer;
        }
        ret = read_bytes_not_key(&cur, write_buffer);
        if (dynamic) free(write_buffer);
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
        ret = read_inf_or_nan_u8(false, &cur, end);
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

extern ssrjson_align(64) u8 _DecodeBytesSrcBuffer[SSRJSON_STRING_BUFFER_SIZE];

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

force_inline void _alloc_aligned_bytes_buffer(Py_ssize_t len, bool *dynamic, u8 **buffer) {
    if (unlikely(len > (Py_ssize_t)PY_SSIZE_T_MAX - 2 * SSRJSON_MEMCPY_SIMD_SIZE - 4)) {
        PyErr_NoMemory();
        *buffer = NULL;
        return;
    }
    Py_ssize_t required_size = size_align_up(len + SSRJSON_MEMCPY_SIMD_SIZE + 4, SSRJSON_MEMCPY_SIMD_SIZE);
    if (unlikely(required_size > SSRJSON_STRING_BUFFER_SIZE)) {
        *buffer = SSRJSON_ALIGNED_ALLOC(SSRJSON_MEMCPY_SIMD_SIZE, required_size);
        if (unlikely(!*buffer)) {
            PyErr_NoMemory();
            return;
        }
        *dynamic = true;
    } else {
        *buffer = _DecodeBytesSrcBuffer;
        *dynamic = false;
    }
}

force_inline bool should_read_bytes_pretty(const u8 *buffer, Py_ssize_t len) {
    if (len > 3) {
        // check if can use pretty read
        u8 second, third;
        second = buffer[1];
        third = buffer[2];
        if (second == '\n' || third == '\n') {
            // likely to hit
            return true;
        }
        if (char_is_space(second) && char_is_space(third)) {
            return true;
        }
    }
    return false;
}

internal_simd_noinline PyObject *ssrjson_decode_bytes(char *_buffer, Py_ssize_t len) {
    if (unlikely(!len)) {
        PyErr_Format(JSONDecodeError, "input data is empty");
        return NULL;
    }

    assert(_buffer);
    assert(len > 0);

    if (!_skip_starting_space(&_buffer, &len)) {
        return NULL;
    }

    u8 *_new_buffer;
    bool is_dynamic;
    _alloc_aligned_bytes_buffer(len, &is_dynamic, &_new_buffer);
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
        if (should_read_bytes_pretty(buffer, len)) {
            ret = read_bytes_root_pretty(buffer, len);
        } else {
            ret = read_bytes_root_minify(buffer, len);
        }
    } else {
        ret = read_root_single_bytes(buffer, len);
    }

    if (is_dynamic) SSRJSON_ALIGNED_FREE(_new_buffer);
    return ret;
}

#undef COMPILE_SIMD_BITS

#endif // SSRJSON_DECODE_DECODE_BYTES_H

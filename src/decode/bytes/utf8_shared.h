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

#ifndef SSRJSON_DECODE_BYTES_UTF8_SHARED_H
#define SSRJSON_DECODE_BYTES_UTF8_SHARED_H

#include "decode/decode_shared.h"
#include "simd/memcpy.h"
#include "ssrjson.h"

/*
 * Padding on both sides of the decode destination buffer, as in the str path.
 * A block decoder stores whole registers however few lanes are valid, and one
 * block produces at most SSRJSON_MEMCPY_SIMD_SIZE units of at most 4 bytes.
 */
#define _BytesDstPadding (4 * SSRJSON_MEMCPY_SIMD_SIZE)

/* Mirrors `check_and_reserve_str_buffer` of the str path. Returns false on
 * allocation failure, without setting an exception. */
force_inline bool check_and_reserve_bytes_str_buffer(DecoderBuffers *decoder_context, Py_ssize_t len,
                                                     u8 **buffer_head_addr, bool *need_dealloc) {
    // Worst case each source byte decodes to one ucs4 unit, i.e. 4 * len bytes.
    if (unlikely(len > ((Py_ssize_t)PY_SSIZE_T_MAX - _BytesDstPadding * 2) / 4)) return false;
    static_assert(((Py_ssize_t)SSRJSON_STRING_BUFFER_SIZE - _BytesDstPadding * 2) > 4,
                  "((Py_ssize_t)SSRJSON_STRING_BUFFER_SIZE - _BytesDstPadding * 2) > 4");
    Py_ssize_t new_buffer_size = 4 * len + 2 * _BytesDstPadding;
    if (unlikely(new_buffer_size > SSRJSON_STRING_BUFFER_SIZE)) {
        u8 *new_buffer = (u8 *)SSRJSON_MALLOC(new_buffer_size);
        if (unlikely(!new_buffer)) return false;
        *buffer_head_addr = new_buffer + _BytesDstPadding;
        *need_dealloc = true;
    } else {
        *buffer_head_addr = decoder_context->decoder_ctx_temp_buffer + _BytesDstPadding;
        *need_dealloc = false;
    }
    return true;
}

force_inline void free_bytes_str_buffer(u8 *buffer_head, bool need_dealloc) {
    if (unlikely(need_dealloc)) { SSRJSON_FREE((void *)(buffer_head - _BytesDstPadding)); }
}

/*
 * Validate one sequence out of a four byte little endian load taken at the lead
 * byte, so byte 0 of the sequence sits in the low bits. Rejects continuation
 * bytes in lead position, overlong forms, encoded surrogate halves
 * [U+D800, U+DFFF] and anything above U+10FFFF.
 *
 * No bounds check: the source buffer is padded past `src_end` and `*src_end` is
 * a NUL, so a truncated sequence fails validation like any other malformed one.
 *
 * `_tmp_` must be a u32 lvalue; the caller supplies it so the tests stay usable
 * inside a condition.
 */
#define utf8_is_seq_2(_uni_) ((((_uni_) & 0x0000C0E0UL) == 0x000080C0UL) && ((_uni_) & 0x0000001EUL))
#define utf8_is_seq_3(_uni_, _tmp_) \
    ((((_uni_) & 0x00C0C0F0UL) == 0x008080E0UL) && ((_tmp_) = ((_uni_) & 0x0000200FUL)) && ((_tmp_) != 0x0000200DUL))
#define utf8_is_seq_4(_uni_, _tmp_)                                                        \
    ((((_uni_) & 0xC0C0C0F8UL) == 0x808080F0UL) && ((_tmp_) = ((_uni_) & 0x00003007UL)) && \
     (((_tmp_) & 0x00000004UL) == 0 || ((_tmp_) & 0x00003003UL) == 0))

/* Kept out of line so the hot path carries only the branch that lands here. */
internal_simd_noinline void utf8_set_seq_error(const u8 *src, const u8 *src_end) {
    u8 c = src[0];
    usize want = (c < 0xE0) ? 2 : ((c < 0xF0) ? 3 : 4);
    if ((usize)(src_end - src) < want) {
        PyErr_SetString(JSONDecodeError, "Unexpected end of string");
    } else {
        PyErr_SetString(JSONDecodeError, "Invalid UTF-8 encoding in string");
    }
}

/* The minimal ucs level holding the code point of lead byte `c`. Only
 * meaningful for a valid non-ASCII lead byte. */
force_inline int utf8_lead_min_ucs_level(u8 c) {
    assert(c >= 0x80);
    if (c < 0xC4) return 1;
    if (c < 0xF0) return 2;
    return 4;
}

/*
 * The destination buffer is laid out as [u8: u8size][u16: u16size][u32], as in
 * the str path, so an upgrade only records the size of the region being closed
 * and rebases the writer. `process_escape_ascii_*` in decode/str/ascii.h does
 * the same bookkeeping.
 */
force_inline void promote_buffer_u8_to_u16(u8 **u8writer_addr, u16 **u16writer_addr, usize *u8size_addr,
                                           void *temp_buffer) {
    usize u8size = (*u8writer_addr) - ssrjson_cast(u8 *, temp_buffer);
    *u8size_addr = u8size;
    *u8writer_addr = NULL;
    *u16writer_addr = ssrjson_cast(u16 *, temp_buffer) + u8size;
}

force_inline void promote_buffer_u8_to_u32(u8 **u8writer_addr, u32 **u32writer_addr, usize *u8size_addr,
                                           void *temp_buffer) {
    usize u8size = (*u8writer_addr) - ssrjson_cast(u8 *, temp_buffer);
    *u8size_addr = u8size;
    *u8writer_addr = NULL;
    *u32writer_addr = ssrjson_cast(u32 *, temp_buffer) + u8size;
}

force_inline void promote_buffer_u16_to_u32(u16 **u16writer_addr, u32 **u32writer_addr, usize *u16size_addr,
                                            usize u8size, void *temp_buffer) {
    usize total = (*u16writer_addr) - ssrjson_cast(u16 *, temp_buffer);
    assert(total >= u8size);
    *u16size_addr = total - u8size;
    *u16writer_addr = NULL;
    *u32writer_addr = ssrjson_cast(u32 *, temp_buffer) + total;
}

#endif // SSRJSON_DECODE_BYTES_UTF8_SHARED_H

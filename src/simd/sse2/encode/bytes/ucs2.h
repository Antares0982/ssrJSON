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

#ifndef SSRJSON_SIMD_SSE2_ENCODE_BYTES_UCS2_H
#define SSRJSON_SIMD_SSE2_ENCODE_BYTES_UCS2_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "encode/encode_utf8_shared.h"
#include "simd/sse2/checker.h"
#include "simd/sse2/common.h"
#include "simd/sse2/cvt.h"
//
#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 1
#define COMPILE_SIMD_BITS 128
#include "compile_context/srw_in.inl.h"

#if __SSSE3__
#    include "simd/ssse3/common.h"

#    define __reserve_ucs2_encode_3bytes_utf8_ssse3 (24)

force_inline void ucs2_encode_3bytes_utf8_ssse3(u8 *writer, vector_a x) {
    static const vector_a_u8_128 t1 = {
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            0x80, 0x80, 0x80, 0x80};
    static const vector_a_u8_128 m1 = {
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0xff, 0xff, 0xff};
    static const vector_a_u8_128 m2 = {
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0, 0, 0, 0};
    vector_a_u32_128 x1 = cvt_u16_to_u32_128(x);
    vector_a_u32_128 x2 = cvt_u16_to_u32_128(_mm_unpackhi_epi64(x, x));
    vector_a_u32_128 x3 = rshift_u32_128(x1, 6);
    vector_a_u32_128 x4 = rshift_u32_128(x1, 12);
    vector_a_u32_128 x5 = rshift_u32_128(x2, 6);
    vector_a_u32_128 x6 = rshift_u32_128(x2, 12);
    vector_a_u8_128 x7 = shuffle_128(x1, t1);
    vector_a_u8_128 x8 = shuffle_128(x3, t1);
    x8 = byte_rshift_128(x8, 1);
    vector_a_u8_128 x9 = shuffle_128(x4, t1);
    x9 = byte_rshift_128(x9, 2);
    vector_a_u8_128 x10 = shuffle_128(x2, t1);
    vector_a_u8_128 x11 = shuffle_128(x5, t1);
    x11 = byte_rshift_128(x11, 1);
    vector_a_u8_128 x12 = shuffle_128(x6, t1);
    x12 = byte_rshift_128(x12, 2);
    vector_a_u8_128 x13 = ((x7 | x8 | x9) & m1) | m2;
    vector_a_u8_128 x14 = ((x10 | x11 | x12) & m1) | m2;
    vector_a_u8_128 x15 = alignr_128(byte_lshift_128(x13, 4), x14, 4);
    vector_a_u8_128 x16 = byte_rshift_128(x14, 4);
    *(vector_u_u8_128 *)(writer + 0) = x15;
    memcpy(writer + 16, &x16, 8);
}
#endif

#define __reserve_ucs2_encode_2bytes_utf8_sse2 (16)

force_inline void ucs2_encode_2bytes_utf8_sse2(u8 *writer, vector_a x) {
    /* abcdefgh|12300000 -> gh123[mmm]|abcdef[mm] */
    /* x1 = gh123000|00000000 */
    vector_a x1 = rshift_u16(x, 6);
    /* x2 = ????????|abcdefgh */
    vector_a x2 = byte_lshift_128(x, 1);
    /* x2 = 00000000|abcdef00 */
    x2 = x2 & broadcast(0x3f00);
    /* y = gh123000|abcdef00 */
    x = x1 | x2;
    /* y = gh123[mmm]|abcdef[mm] */
    x = x | broadcast(0x80c0);
    *(vector_u *)writer = x;
}

/* See AVX2 code for more details. */
#define __readbefore_bytes_write_ucs2_trailing_128 (16)
#define __excess_bytes_write_ucs2_trailing_128 (24 - max_json_bytes_per_unicode)

/* 
 * Encode UCS2 trailing to UTF-8.
 * Consider 3 types of vector:
 *   vector in ASCII range
 *   vector in 2-bytes range
 *   vector in 3-bytes range
 */
force_inline bool bytes_write_ucs2_trailing_128(u8 **writer_addr, const u16 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    const u16 *const src_end = src + len;
    const u16 *const last_batch_start = src_end - READ_BATCH_COUNT;
    const vector_a vec = *(const vector_u *)last_batch_start;
    u8 *writer = *writer_addr;
    //
    vector_a m, tail_vec;
    usize shift;

restart:;
    if (len == 1) {
        if (unlikely(!encode_one_ucs2(&writer, *src))) return false;
        goto finished;
    }
    u16 cur_unicode = *src;
    bool is_escaped;
    int unicode_type = ucs2_get_type(cur_unicode, &is_escaped);
    switch (unicode_type) {
        case 1: {
            if (unlikely(is_escaped)) {
                memcpy(writer, &ControlEscapeTable_u8[cur_unicode * 8], 8);
                writer += _ControlJump[cur_unicode];
                src++;
                len--;
                if (len) goto restart;
                goto finished;
            }
            goto ascii;
        }
        case 2: {
            goto _2bytes;
        }
        case 3: {
#if __SSSE3__
            goto _3bytes;
#else
            if (unlikely(!encode_one_ucs2(&writer, cur_unicode))) return false;
            src++;
            len--;
            if (len) goto restart;
            goto finished;
#endif
        }
        default: {
            SSRJSON_UNREACHABLE();
        }
    }
    SSRJSON_UNREACHABLE();
ascii:;
    {
        const vector_a m_not_ascii = (vec == broadcast(_Quote)) | (vec == broadcast(_Slash)) | unsigned_saturate_minus(broadcast(ControlMax), vec) | unsigned_saturate_minus(vec, broadcast(0x7f));
        m = high_mask(m_not_ascii, len);
        shift = sizeof(u16) * (READ_BATCH_COUNT - len);
        tail_vec = runtime_byte_rshift_128(vec, shift);
        cvt_to_dst(writer, tail_vec);
        if (likely(testz(m))) {
            writer += len;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u16 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count;
            len = READ_BATCH_COUNT - done_count - 1;
            if (escape_unicode >= ControlMax && escape_unicode < 0x80 && escape_unicode != _Slash && escape_unicode != _Quote) {
                SSRJSON_UNREACHABLE();
            } else {
                if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
            }
            if (len) goto restart;
            goto finished;
        }
        SSRJSON_UNREACHABLE();
    }
_2bytes:;
    {
        const vector_a m_not_2bytes = unsigned_saturate_minus(broadcast(0x80), vec) | unsigned_saturate_minus(vec, broadcast(0x7ff));
        m = high_mask(m_not_2bytes, len);
        shift = sizeof(u16) * (READ_BATCH_COUNT - len);
        tail_vec = runtime_byte_rshift_128(vec, shift);
        ucs2_encode_2bytes_utf8_sse2(writer, tail_vec);
        if (likely(testz(m))) {
            writer += len * 2;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u16 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count * 2;
            len = READ_BATCH_COUNT - done_count - 1;
            if (escape_unicode >= 0x80 && escape_unicode <= 0x7ff) {
                SSRJSON_UNREACHABLE();
            } else {
                if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
            }
            if (len) goto restart;
            goto finished;
        }
        SSRJSON_UNREACHABLE();
    }
#if __SSSE3__
_3bytes:;
    {
        const vector_a m_not_3bytes = unsigned_saturate_minus(broadcast(0x800), vec) | (signed_cmpgt(vec, broadcast(0xd7ff)) & signed_cmpgt(broadcast(0xe000), vec));
        m = high_mask(m_not_3bytes, len);
        shift = sizeof(u16) * (READ_BATCH_COUNT - len);
        tail_vec = runtime_byte_rshift_128(vec, shift);
        ucs2_encode_3bytes_utf8_ssse3(writer, tail_vec);
        if (likely(testz(m))) {
            writer += len * 3;
            goto finished;
        } else {
            // cannot use no_eq0 version
            usize done_count = escape_mask_to_done_count(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u16 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count * 3;
            len = READ_BATCH_COUNT - done_count - 1;
            if (escape_unicode >= 0x800 && (escape_unicode <= 0xd7ff || escape_unicode >= 0xe000)) {
                SSRJSON_UNREACHABLE();
            } else {
                if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
            }
            if (len) goto restart;
            goto finished;
        }
        SSRJSON_UNREACHABLE();
    }
#endif
finished:;
    *writer_addr = writer;
    return true;
}

/* See AVX2 code for more details. */
#define __readbefore_bytes_write_ucs2_raw_utf8_trailing_128 (16)
#define __excess_bytes_write_ucs2_raw_utf8_trailing_128 (24 - max_utf8_bytes_per_ucs2)

force_inline bool bytes_write_ucs2_raw_utf8_trailing_128(u8 **writer_addr, const u16 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    const u16 *const src_end = src + len;
    const u16 *const last_batch_start = src_end - READ_BATCH_COUNT;
    const vector_a vec = *(const vector_u *)last_batch_start;
    u8 *writer = *writer_addr;
    //
    vector_a m, tail_vec;
    usize shift;

restart:;
    if (len == 1) {
        if (unlikely(!encode_one_ucs2_noescape(&writer, *src))) return false;
        goto finished;
    }
    u16 cur_unicode = *src;
    bool is_escaped_unused;
    int unicode_type = ucs2_get_type(cur_unicode, &is_escaped_unused);
    switch (unicode_type) {
        case 1: {
            goto ascii;
        }
        case 2: {
            goto _2bytes;
        }
        case 3: {
#if __SSSE3__
            goto _3bytes;
#else
            if (unlikely(!encode_one_ucs2_noescape(&writer, cur_unicode))) return false;
            src++;
            len--;
            if (len) goto restart;
            goto finished;
#endif
        }
        default: {
            SSRJSON_UNREACHABLE();
        }
    }
    // ---unreachable here---
ascii:;
    {
        const vector_a m_not_ascii = unsigned_saturate_minus(broadcast(0), vec) | unsigned_saturate_minus(vec, broadcast(0x7f));
        m = high_mask(m_not_ascii, len);
        shift = sizeof(u16) * (READ_BATCH_COUNT - len);
        tail_vec = runtime_byte_rshift_128(vec, shift);
        cvt_to_dst(writer, tail_vec);
        if (likely(testz(m))) {
            writer += len;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u16 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count;
            len = READ_BATCH_COUNT - done_count - 1;
            assume(escape_unicode >= 128);
            if (unlikely(!encode_one_ucs2_noescape(&writer, escape_unicode))) return false;
            if (len) goto restart;
            goto finished;
        }
        // ---unreachable here---
    }
_2bytes:;
    {
        const vector_a m_not_2bytes = unsigned_saturate_minus(broadcast(0x80), vec) | unsigned_saturate_minus(vec, broadcast(0x7ff));
        m = high_mask(m_not_2bytes, len);
        shift = sizeof(u16) * (READ_BATCH_COUNT - len);
        tail_vec = runtime_byte_rshift_128(vec, shift);
        ucs2_encode_2bytes_utf8_sse2(writer, tail_vec);
        if (likely(testz(m))) {
            writer += len * 2;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u16 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count * 2;
            len = READ_BATCH_COUNT - done_count - 1;
            assume(!(escape_unicode >= 0x80 && escape_unicode <= 0x7ff));
            if (unlikely(!encode_one_ucs2_noescape(&writer, escape_unicode))) return false;
            if (len) goto restart;
            goto finished;
        }
        // ---unreachable here---
    }
#if __SSSE3__
_3bytes:;
    {
        const vector_a m_not_3bytes = unsigned_saturate_minus(broadcast(0x800), vec) | (signed_cmpgt(vec, broadcast(0xd7ff)) & signed_cmpgt(broadcast(0xe000), vec));
        m = high_mask(m_not_3bytes, len);
        shift = sizeof(u16) * (READ_BATCH_COUNT - len);
        tail_vec = runtime_byte_rshift_128(vec, shift);
        ucs2_encode_3bytes_utf8_ssse3(writer, tail_vec);
        if (likely(testz(m))) {
            writer += len * 3;
            goto finished;
        } else {
            // cannot use no_eq0 version
            usize done_count = escape_mask_to_done_count(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u16 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count * 3;
            len = READ_BATCH_COUNT - done_count - 1;
            assume(!(escape_unicode >= 0x800 && (escape_unicode <= 0xd7ff || escape_unicode >= 0xe000)));
            if (unlikely(!encode_one_ucs2_noescape(&writer, escape_unicode))) return false;
            if (len) goto restart;
            goto finished;
        }
        // ---unreachable here---
    }
#endif
finished:;
    *writer_addr = writer;
    return true;
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_SIMD_BITS
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#endif // SSRJSON_SIMD_SSE2_ENCODE_BYTES_UCS2_H

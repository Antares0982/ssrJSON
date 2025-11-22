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

#ifndef SSRJSON_SIMD_AVX2_ENCODE_BYTES_UCS4_H
#define SSRJSON_SIMD_AVX2_ENCODE_BYTES_UCS4_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "encode/encode_utf8_shared.h"
#include "simd/avx2/checker.h"
#include "simd/avx2/common.h"
#include "simd/avx2/cvt.h"
#include "simd/ssse3/common.h"
//
#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 1
#define COMPILE_SIMD_BITS 256
#include "compile_context/srw_in.inl.h"

force_inline vector_a_u8_128 __ucs4_encode_2bytes_utf8_avx2_impl(vector_a y) {
    /* abcdefgh|12300000|00000000|00000000 -> gh123[mmm]|abcdef[mm] */
    /* x = abcdefgh|12300000 */
    vector_a_u16_128 x = cvt_u32_to_u16_256(y);
    vector_a_u8_128 t1 = {
            0x80, 0,
            0x80, 2,
            0x80, 4,
            0x80, 6,
            0x80, 8,
            0x80, 10,
            0x80, 12,
            0x80, 14};
    vector_a_u8_128 m1 = broadcast_u16_128(0x3fff);
    vector_a_u8_128 m2 = broadcast_u16_128(0x80c0);
    /*x1 = gh123000|00000000 */
    vector_a_u8_128 x1 = rshift_u16_128(x, 6);
    /*x2 = 00000000|abcdefgh */
    vector_a_u8_128 x2 = shuffle_128(x, t1);
    /*x3 = gh123000|abcdefgh */
    vector_a_u8_128 x3 = ((x1 | x2) & m1) | m2;
    return x3;
}

#define __reserve_ucs4_encode_2bytes_utf8_avx2 (16)

force_inline void ucs4_encode_2bytes_utf8_avx2(u8 *writer, vector_a y) {
    *(vector_u_u8_128 *)writer = __ucs4_encode_2bytes_utf8_avx2_impl(y);
}

#define __reserve_ucs4_encode_2bytes_utf8_avx2_trailing (16)

force_inline void ucs4_encode_2bytes_utf8_avx2_trailing(u8 *writer, vector_a y, usize len) {
    vector_a_u8_128 x = __ucs4_encode_2bytes_utf8_avx2_impl(y);
    x = runtime_byte_rshift_128(x, 16 - len * 2);
    *SSRJSON_CAST(vector_u_u8_128 *, writer) = x;
}

force_inline void __ucs4_encode_3bytes_utf8_avx2_impl(vector_a y, vector_a_u8_128 *out_x1, vector_a_u8_128 *out_x2) {
    /* abcdefgh|12345678|00000000|00000000 -> 5678[mmmm]|gh1234[mm]|abcdef[mm] */
    vector_a_u8_256 t1 = {
            0x80, 0x80, 0x80, 0x80,
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            //
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            0x80, 0x80, 0x80, 0x80};
    vector_a_u8_256 t2 = {
            0x80, 0x80, 0x80, 0x80,
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            //
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            0x80, 0x80, 0x80, 0x80};
    vector_a_u8_256 t3 = {
            0x80, 0x80, 0x80, 0x80,
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            //
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            0x80, 0x80, 0x80, 0x80};
    vector_a_u8_256 m1 = {
            0xff, 0xff, 0xff, 0xff,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            //
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0xff, 0xff, 0xff};
    vector_a_u8_256 m2 = {
            0, 0, 0, 0,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            //
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0, 0, 0, 0};
    /* y1 = gh123456|78000000|00000000|00000000 */
    vector_a_u32_256 y1 = rshift_u32_256(y, 6);
    /* y2 = 56780000|00000000|00000000|00000000 */
    vector_a_u32_256 y2 = rshift_u32_256(y, 12);
    /* y3 = 00000000|00000000|abcdefgh */
    vector_a_u8_256 y3 = shuffle_256(y, t1);
    /* y4 = 00000000|gh123456|00000000 */
    vector_a_u8_256 y4 = shuffle_256(y1, t2);
    /* y5 = 56780000|00000000|00000000 */
    vector_a_u8_256 y5 = shuffle_256(y2, t3);
    vector_a_u8_256 y6 = ((y3 | y4 | y5) & m1) | m2;
    //
    vector_a_u8_128 x1 = extract_128_from_256(y6, 0);
    vector_a_u8_128 x2 = extract_128_from_256(y6, 1);
    //
    vector_a_u8_128 x3 = _mm_alignr_epi8(x2, x1, 4);
    vector_a_u8_128 x4 = byte_rshift_128(x2, 4);
    *out_x1 = x3;
    *out_x2 = x4;
}

#define __reserve_ucs4_encode_3bytes_utf8_avx2 (24)

force_inline void ucs4_encode_3bytes_utf8_avx2(u8 *writer, vector_a_u32_256 y) {
    vector_a_u8_128 x1, x2;
    __ucs4_encode_3bytes_utf8_avx2_impl(y, &x1, &x2);
    *(vector_u_u8_128 *)(writer + 0) = x1;
    // optimized to vpshufd + vmovq
    memcpy(writer + 16, &x2, 8);
}

#define __reserve_ucs4_encode_3bytes_utf8_avx2_trailing (24)

force_inline void ucs4_encode_3bytes_utf8_avx2_trailing(const u32 *src, const u32 *src_end, u8 *dst) {
    const size_t half = 32 / 2 / sizeof(u32);
    const u32 *t1 = src_end - half;
    const bool t1_before_src = t1 < src;
    //
    vector_a_u8_128 s1, s2;
    s1 = *(vector_u_u8_128 *)(t1_before_src ? t1 : src);
    s2 = *(vector_u_u8_128 *)t1;
    int __shl1 = (src - t1);
    int __shl2 = (half - (t1 - src));
    int shl1 = t1_before_src ? __shl1 : 0;
    int shl2 = t1_before_src ? 0 : __shl2;
    s1 = runtime_byte_rshift_128(s1, shl1 * 4);
    s2 = runtime_byte_rshift_128(s2, shl2 * 4);
    vector_a y = _mm256_set_m128i(s2, s1);
    vector_a_u8_128 x1, x2;
    __ucs4_encode_3bytes_utf8_avx2_impl(y, &x1, &x2);
    *(vector_u_u8_128 *)(dst + 0) = x1;
    // optimized to vpshufd + vmovq
    memcpy(dst + 16, &x2, 8);
}

#define __readbefore_bytes_write_ucs4_trailing_256 (32)
#define __excess_bytes_write_ucs4_trailing_256 (24 - max_json_bytes_per_unicode)

/* 
 * Encode UCS4 trailing to UTF-8.
 * Consider 3 types of vector:
 *   vector in ASCII range
 *   vector in 2-bytes range
 *   vector in 3-bytes range
 * 4-bytes case is considered as *unlikely*.
 */
force_inline bool bytes_write_ucs4_trailing_256(u8 **writer_addr, const u32 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    const u32 *const src_end = src + len;
    const u32 *const last_batch_start = src_end - READ_BATCH_COUNT;
    const vector_a vec = *(const vector_u *)last_batch_start;
    u8 *writer = *writer_addr;
restart:;
    if (len == 1) {
        // excess bytes written: 8 - max_json_bytes_per_unicode = 2
        if (unlikely(!encode_one_ucs4(&writer, *src))) return false;
        goto finished;
    }
    u32 cur_unicode = *src;
    bool is_escaped;
    int unicode_type = ucs4_get_type(cur_unicode, &is_escaped);
    switch (unicode_type) {
        case 1: {
            if (unlikely(is_escaped)) {
                // excess bytes written: 8 - max_json_bytes_per_unicode = 2
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
            goto _3bytes;
        }
        case 4: {
            // excess bytes written: 8 - max_json_bytes_per_unicode = 2
            if (unlikely(!encode_one_ucs4(&writer, cur_unicode))) assert(false);
            src++;
            len--;
            if (len) goto restart;
            goto finished;
        }
        default: {
            SSRJSON_UNREACHABLE();
        }
    }
    SSRJSON_UNREACHABLE();
ascii:;
    {
        const vector_a m_not_ascii = (vec == broadcast(_Quote)) | (vec == broadcast(_Slash)) | signed_cmpgt(broadcast(ControlMax), vec) | signed_cmpgt(vec, broadcast(0x7f));
        vector_a m = high_mask(m_not_ascii, len);
        // excess bytes written: 8 - max_json_bytes_per_unicode = 2
        avx2_trailing_cvt(src, src_end, writer);
        if (likely(testz(m))) {
            writer += len;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count_no_eq0(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u32 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count;
            len = READ_BATCH_COUNT - done_count - 1;
            if (escape_unicode >= ControlMax && escape_unicode < 0x80 && escape_unicode != _Slash && escape_unicode != _Quote) {
                SSRJSON_UNREACHABLE();
            } else {
                // excess bytes written: 8 - max_json_bytes_per_unicode = 2
                if (unlikely(!encode_one_ucs4(&writer, escape_unicode))) return false;
            }
            if (len) goto restart;
            goto finished;
        }
        SSRJSON_UNREACHABLE();
    }
_2bytes:;
    {
        const vector_a m_not_2bytes = signed_cmpgt(broadcast(0x80), vec) | signed_cmpgt(vec, broadcast(0x7ff));
        vector_a m = high_mask(m_not_2bytes, len);
        // excess bytes written: 16 - max_json_bytes_per_unicode = 10
        ucs4_encode_2bytes_utf8_avx2_trailing(writer, vec, len);
        if (likely(testz(m))) {
            writer += len * 2;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u32 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count * 2;
            len = READ_BATCH_COUNT - done_count - 1;
            if (escape_unicode >= 0x80 && escape_unicode <= 0x7ff) {
                SSRJSON_UNREACHABLE();
            } else {
                // excess bytes written: 8 - max_json_bytes_per_unicode = 2
                if (unlikely(!encode_one_ucs4(&writer, escape_unicode))) return false;
            }
            if (len) goto restart;
            goto finished;
        }
        SSRJSON_UNREACHABLE();
    }
_3bytes:;
    {
        const vector_a m_not_3bytes = signed_cmpgt(broadcast(0x800), vec) | (signed_cmpgt(vec, broadcast(0xd7ff)) & signed_cmpgt(broadcast(0xe000), vec)) | signed_cmpgt(vec, broadcast(0xffff));
        vector_a m = high_mask(m_not_3bytes, len);
        // excess bytes written: 24 - max_json_bytes_per_unicode = 18
        ucs4_encode_3bytes_utf8_avx2_trailing(src, src_end, writer);
        if (likely(testz(m))) {
            writer += len * 3;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count_no_eq0(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u32 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count * 3;
            len = READ_BATCH_COUNT - done_count - 1;
            if (escape_unicode >= 0x800 && escape_unicode <= 0xffff && (escape_unicode <= 0xd7ff || escape_unicode >= 0xe000)) {
                SSRJSON_UNREACHABLE();
            } else {
                // excess bytes written: 8 - max_json_bytes_per_unicode = 2
                if (unlikely(!encode_one_ucs4(&writer, escape_unicode))) return false;
            }
            if (len) goto restart;
            goto finished;
        }
        SSRJSON_UNREACHABLE();
    }
finished:;
    *writer_addr = writer;
    return true;
}

#define __readbefore_bytes_write_ucs4_raw_utf8_trailing_256 (32)
#define __excess_bytes_write_ucs4_raw_utf8_trailing_256 (24 - max_utf8_bytes_per_ucs4)

force_inline bool bytes_write_ucs4_raw_utf8_trailing_256(u8 **writer_addr, const u32 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    const u32 *const src_end = src + len;
    const u32 *const last_batch_start = src_end - READ_BATCH_COUNT;
    const vector_a vec = *(const vector_u *)last_batch_start;
    u8 *writer = *writer_addr;
restart:;
    if (len == 1) {
        if (unlikely(!encode_one_ucs4_noescape(&writer, *src))) return false;
        goto finished;
    }
    u32 cur_unicode = *src;
    bool is_escaped_unused;
    int unicode_type = ucs4_get_type(cur_unicode, &is_escaped_unused);
    switch (unicode_type) {
        case 1: {
            goto ascii;
        }
        case 2: {
            goto _2bytes;
        }
        case 3: {
            goto _3bytes;
        }
        case 4: {
            if (unlikely(!encode_one_ucs4_noescape(&writer, cur_unicode))) assert(false);
            src++;
            len--;
            if (len) goto restart;
            goto finished;
        }
        default: {
            SSRJSON_UNREACHABLE();
        }
    }
    // ---unreachable here---
ascii:;
    {
        const vector_a m_not_ascii = signed_cmpgt(broadcast(0), vec) | signed_cmpgt(vec, broadcast(0x7f));
        vector_a m = high_mask(m_not_ascii, len);
        // excess bytes written: 8 - max_utf8_bytes_per_ucs4 = 4
        avx2_trailing_cvt(src, src_end, writer);
        if (likely(testz(m))) {
            writer += len;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count_no_eq0(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u32 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count;
            len = READ_BATCH_COUNT - done_count - 1;
            assume(escape_unicode >= 128);
            if (unlikely(!encode_one_ucs4_noescape(&writer, escape_unicode))) return false;
            if (len) goto restart;
            goto finished;
        }
        // ---unreachable here---
    }
_2bytes:;
    {
        const vector_a m_not_2bytes = signed_cmpgt(broadcast(0x80), vec) | signed_cmpgt(vec, broadcast(0x7ff));
        vector_a m = high_mask(m_not_2bytes, len);
        // excess bytes written: 16 - max_utf8_bytes_per_ucs4 = 12
        ucs4_encode_2bytes_utf8_avx2_trailing(writer, vec, len);
        if (likely(testz(m))) {
            writer += len * 2;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u32 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count * 2;
            len = READ_BATCH_COUNT - done_count - 1;
            assume(!(escape_unicode >= 0x80 && escape_unicode <= 0x7ff));
            if (unlikely(!encode_one_ucs4_noescape(&writer, escape_unicode))) return false;
            if (len) goto restart;
            goto finished;
        }
        // ---unreachable here---
    }
_3bytes:;
    {
        const vector_a m_not_3bytes = signed_cmpgt(broadcast(0x800), vec) | (signed_cmpgt(vec, broadcast(0xd7ff)) & signed_cmpgt(broadcast(0xe000), vec)) | signed_cmpgt(vec, broadcast(0xffff));
        vector_a m = high_mask(m_not_3bytes, len);
        // excess bytes written: 24 - max_utf8_bytes_per_ucs4 = 20
        ucs4_encode_3bytes_utf8_avx2_trailing(src, src_end, writer);
        if (likely(testz(m))) {
            writer += len * 3;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count_no_eq0(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u32 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count * 3;
            len = READ_BATCH_COUNT - done_count - 1;
            assume(!(escape_unicode >= 0x800 && escape_unicode <= 0xffff && (escape_unicode <= 0xd7ff || escape_unicode >= 0xe000)));
            if (unlikely(!encode_one_ucs4_noescape(&writer, escape_unicode))) return false;
            if (len) goto restart;
            goto finished;
        }
        // ---unreachable here---
    }
finished:;
    *writer_addr = writer;
    return true;
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_SIMD_BITS
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#endif // SSRJSON_SIMD_AVX2_ENCODE_BYTES_UCS4_H

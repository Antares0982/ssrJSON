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

#ifndef SSRJSON_SIMD_AVX2_ENCODE_BYTES_UCS2_H
#define SSRJSON_SIMD_AVX2_ENCODE_BYTES_UCS2_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "encode/encode_utf8_shared.h"
#include "simd/avx2/checker.h"
#include "simd/avx2/common.h"
#include "simd/avx2/cvt.h"
//
#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 1
#define COMPILE_SIMD_BITS 256
#include "compile_context/srw_in.inl.h"

force_inline vector_a_u16_256 __ucs2_encode_2bytes_utf8_avx2_impl(vector_a y) {
    /* abcdefgh|12300000 -> gh123[mmm]|abcdef[mm] */
    vector_a_u8_256 t1 = {
            0x80, 0,
            0x80, 2,
            0x80, 4,
            0x80, 6,
            0x80, 8,
            0x80, 10,
            0x80, 12,
            0x80, 14,
            //
            0x80, 0,
            0x80, 2,
            0x80, 4,
            0x80, 6,
            0x80, 8,
            0x80, 10,
            0x80, 12,
            0x80, 14};
    /*y1 = gh123000|00000000 */
    vector_a_u8_256 y1 = rshift_u16(y, 6);
    /*y2 = 00000000|abcdefgh */
    vector_a_u8_256 y2 = shuffle_256(y, t1);
    /*y = gh123000|abcdefgh */
    y = y1 | y2;
    /*y = gh123000|abcdef00 */
    y = y & broadcast(0x3fff);
    /*y = gh123[mmm]|abcdef[mm] */
    y = y | broadcast(0x80c0);
    return y;
}

#define __reserve_ucs2_encode_2bytes_utf8_avx2 (32)

force_inline void ucs2_encode_2bytes_utf8_avx2(u8 *writer, vector_a y) {
    *(vector_u *)writer = __ucs2_encode_2bytes_utf8_avx2_impl(y);
}

force_inline vector_a __ucs2_encode_bytes_avx2_trailing(const u16 *src, const u16 *src_end) {
    const size_t half = 32 / 2 / sizeof(u16);
    const u16 *t1 = src_end - half;
    const bool t1_before_src = t1 < src;
    //
    vector_a_u8_128 s1, s2;
    s1 = *(vector_u_u8_128 *)(t1_before_src ? t1 : src);
    s2 = *(vector_u_u8_128 *)t1;
    //
    int __shl1 = (src - t1) * 2;
    int __shl2 = (half - (t1 - src)) * 2;
    int shl1 = t1_before_src ? __shl1 : 0;
    int shl2 = t1_before_src ? 0 : __shl2;
    s1 = runtime_byte_rshift_128(s1, shl1);
    s2 = runtime_byte_rshift_128(s2, shl2);
    return _mm256_set_m128i(s2, s1);
}

#define __reserve_ucs2_encode_2bytes_utf8_avx2_trailing (32)

force_inline void ucs2_encode_2bytes_utf8_avx2_trailing(const u16 *src, const u16 *src_end, u8 *writer) {
    vector_a y = __ucs2_encode_bytes_avx2_trailing(src, src_end);
    *SSRJSON_CAST(vector_u_u16_256 *, writer) = __ucs2_encode_2bytes_utf8_avx2_impl(y);
}

force_inline void __ucs2_encode_3bytes_utf8_avx2_impl(vector_a y, vector_a_u8_128 *out_x1, vector_a_u8_128 *out_x2, vector_a_u8_128 *out_x3, vector_a_u8_128 *out_x4) {
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

    vector_a_u32_256 y1, y2;
    y1 = cvt_u16_to_u32_256(extract_128_from_256(y, 0));
    y2 = cvt_u16_to_u32_256(extract_128_from_256(y, 1));
    /* y3,y4 = gh123456|78000000|00000000|00000000 */
    vector_a_u32_256 y3 = rshift_u32_256(y1, 6);
    vector_a_u32_256 y4 = rshift_u32_256(y2, 6);
    /* y5,y6 = 56780000|00000000|00000000|00000000 */
    vector_a_u32_256 y5 = rshift_u32_256(y1, 12);
    vector_a_u32_256 y6 = rshift_u32_256(y2, 12);
    /* y7,y8 = 00000000|00000000|abcdefgh */
    vector_a_u8_256 y7 = shuffle_256(y1, t1);
    vector_a_u8_256 y8 = shuffle_256(y2, t1);
    /* y9,y10 = 00000000|gh123456|00000000 */
    vector_a_u8_256 y9 = shuffle_256(y3, t2);
    vector_a_u8_256 y10 = shuffle_256(y4, t2);
    /* y11,y12 = 56780000|00000000|00000000 */
    vector_a_u8_256 y11 = shuffle_256(y5, t3);
    vector_a_u8_256 y12 = shuffle_256(y6, t3);
    //
    vector_a_u8_256 y13 = ((y7 | y9 | y11) & m1) | m2;
    vector_a_u8_256 y14 = ((y8 | y10 | y12) & m1) | m2;
    //
    vector_a_u8_128 x1 = extract_128_from_256(y13, 0);
    vector_a_u8_128 x2 = extract_128_from_256(y13, 1);
    vector_a_u8_128 x3 = _mm_alignr_epi8(x2, x1, 4);
    vector_a_u8_128 x4 = byte_rshift_128(x2, 4);
    vector_a_u8_128 x5 = extract_128_from_256(y14, 0);
    vector_a_u8_128 x6 = extract_128_from_256(y14, 1);
    vector_a_u8_128 x7 = _mm_alignr_epi8(x6, x5, 4);
    vector_a_u8_128 x8 = byte_rshift_128(x6, 4);
    *out_x1 = x3;
    *out_x2 = x4;
    *out_x3 = x7;
    *out_x4 = x8;
}

#define __reserve_ucs2_encode_3bytes_utf8_avx2 (48)

force_inline void ucs2_encode_3bytes_utf8_avx2(u8 *writer, vector_a y) {
    vector_a_u8_128 x1, x2, x3, x4;
    __ucs2_encode_3bytes_utf8_avx2_impl(y, &x1, &x2, &x3, &x4);
    // [0, 16)
    *(vector_u_u8_128 *)(writer + 0) = x1;
    // [16, 24) optimized to vpshufd + vmovq
    memcpy(writer + 16, &x2, 8);
    // [24, 40)
    *(vector_u_u8_128 *)(writer + 24) = x3;
    // [40, 48) optimized to vpshufd + vmovq
    memcpy(writer + 40, &x4, 8);
}

#define __reserve_ucs2_encode_3bytes_utf8_avx2_trailing (48)

force_inline void ucs2_encode_3bytes_utf8_avx2_trailing(const u16 *src, const u16 *src_end, u8 *writer) {
    vector_a y = __ucs2_encode_bytes_avx2_trailing(src, src_end);
    vector_a_u8_128 x1, x2, x3, x4;
    __ucs2_encode_3bytes_utf8_avx2_impl(y, &x1, &x2, &x3, &x4);
    // [0, 16)
    *(vector_u_u8_128 *)(writer + 0) = x1;
    // [16, 24) optimized to vpshufd + vmovq
    memcpy(writer + 16, &x2, 8);
    // [24, 40)
    *(vector_u_u8_128 *)(writer + 24) = x3;
    // [40, 48) optimized to vpshufd + vmovq
    memcpy(writer + 40, &x4, 8);
}

#define __readbefore_bytes_write_ucs2_trailing_256 (32)
#define __excess_bytes_write_ucs2_trailing_256 (48 - max_json_bytes_per_unicode)

/* 
 * Encode UCS2 trailing to UTF-8.
 * Consider 3 types of vector:
 *   vector in ASCII range
 *   vector in 2-bytes range
 *   vector in 3-bytes range
 */
force_inline bool bytes_write_ucs2_trailing_256(u8 **writer_addr, const u16 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    // constants
    const u16 *const src_end = src + len;
    const u16 *const last_batch_start = src_end - READ_BATCH_COUNT;
    const vector_a vec = *(const vector_u *)last_batch_start;
    //
    u8 *writer = *writer_addr;
restart:;
    if (len == 1) {
        // excess bytes written: 8 - max_json_bytes_per_unicode = 2
        if (unlikely(!encode_one_ucs2(&writer, *src))) return false;
        goto finished;
    }
    u16 cur_unicode = *src;
    bool is_escaped;
    int unicode_type = ucs2_get_type(cur_unicode, &is_escaped);
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
        default: {
            SSRJSON_UNREACHABLE();
        }
    }
    SSRJSON_UNREACHABLE();
ascii:;
    {
        const vector_a m_not_ascii = (vec == broadcast(_Quote)) | (vec == broadcast(_Slash)) | signed_cmpgt(broadcast(ControlMax), vec) | signed_cmpgt(vec, broadcast(0x7f));
        vector_a m = high_mask(m_not_ascii, len);
        // excess bytes written: 16 - max_json_bytes_per_unicode = 10
        avx2_trailing_cvt(src, src_end, writer);
        if (likely(testz(m))) {
            writer += len;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count_no_eq0(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u16 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count;
            len = READ_BATCH_COUNT - done_count - 1;
            if (escape_unicode >= ControlMax && escape_unicode < 0x80 && escape_unicode != _Slash && escape_unicode != _Quote) {
                SSRJSON_UNREACHABLE();
            } else {
                // excess bytes written: 16 - max_json_bytes_per_unicode = 10
                if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
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
        // excess bytes written: 32 - max_json_bytes_per_unicode = 26
        ucs2_encode_2bytes_utf8_avx2_trailing(src, src_end, writer);
        if (likely(testz(m))) {
            writer += len * 2;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count_no_eq0(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u16 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count * 2;
            len = READ_BATCH_COUNT - done_count - 1;
            if (escape_unicode >= 0x80 && escape_unicode <= 0x7ff) {
                SSRJSON_UNREACHABLE();
            } else {
                // excess bytes written: 8 - max_json_bytes_per_unicode = 2
                if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
            }
            if (len) goto restart;
            goto finished;
        }
        SSRJSON_UNREACHABLE();
    }
_3bytes:;
    {
        const vector_a m_not_3bytes = unsigned_saturate_minus(broadcast(0x800), vec) | (signed_cmpgt(vec, broadcast(0xd7ff)) & signed_cmpgt(broadcast(0xe000), vec));
        vector_a m = high_mask(m_not_3bytes, len);
        // excess bytes written: 48 - max_json_bytes_per_unicode = 42
        ucs2_encode_3bytes_utf8_avx2_trailing(src, src_end, writer);
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
                // excess bytes written: 8 - max_json_bytes_per_unicode = 2
                if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
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

#define __readbefore_bytes_write_ucs2_raw_utf8_trailing_256 (32)
#define __excess_bytes_write_ucs2_raw_utf8_trailing_256 (48 - max_utf8_bytes_per_ucs2)

force_inline bool bytes_write_ucs2_raw_utf8_trailing_256(u8 **writer_addr, const u16 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    // constants
    const u16 *const src_end = src + len;
    const u16 *const last_batch_start = src_end - READ_BATCH_COUNT;
    const vector_a vec = *(const vector_u *)last_batch_start;
    //
    u8 *writer = *writer_addr;
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
            goto _3bytes;
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
        // excess bytes written: 16 - max_utf8_bytes_per_ucs2 = 13
        avx2_trailing_cvt(src, src_end, writer);
        if (likely(testz(m))) {
            writer += len;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count_no_eq0(m);
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
        const vector_a m_not_2bytes = signed_cmpgt(broadcast(0x80), vec) | signed_cmpgt(vec, broadcast(0x7ff));
        vector_a m = high_mask(m_not_2bytes, len);
        // excess bytes written: 32 - max_utf8_bytes_per_ucs2 = 29
        ucs2_encode_2bytes_utf8_avx2_trailing(src, src_end, writer);
        if (likely(testz(m))) {
            writer += len * 2;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count_no_eq0(m);
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
_3bytes:;
    {
        const vector_a m_not_3bytes = unsigned_saturate_minus(broadcast(0x800), vec) | (signed_cmpgt(vec, broadcast(0xd7ff)) & signed_cmpgt(broadcast(0xe000), vec));
        vector_a m = high_mask(m_not_3bytes, len);
        // excess bytes written: 48 - max_utf8_bytes_per_ucs2 = 45
        ucs2_encode_3bytes_utf8_avx2_trailing(src, src_end, writer);
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
finished:;
    *writer_addr = writer;
    return true;
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_SIMD_BITS
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#endif // SSRJSON_SIMD_AVX2_ENCODE_BYTES_UCS2_H

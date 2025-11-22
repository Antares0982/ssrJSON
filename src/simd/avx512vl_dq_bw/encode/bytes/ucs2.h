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

#ifndef SSRJSON_SIMD_AVX512VLDQBW_ENCODE_BYTES_UCS2_H
#define SSRJSON_SIMD_AVX512VLDQBW_ENCODE_BYTES_UCS2_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "encode/encode_utf8_shared.h"
#include "simd/avx512vl_dq_bw/checker.h"
#include "simd/avx512vl_dq_bw/common.h"
#include "simd/avx512vl_dq_bw/cvt.h"
//
#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 1
#define COMPILE_SIMD_BITS 512
#include "compile_context/srw_in.inl.h"

#define __reserve_ucs2_encode_2bytes_utf8_avx512 (64)

force_inline void ucs2_encode_2bytes_utf8_avx512(u8 *writer, vector_a_u16_512 z) {
    /* abcdefgh|12300000 -> gh123[mmm]|abcdef[mm] */
    vector_a_u8_128 t1 = {
            0x80, 0,
            0x80, 2,
            0x80, 4,
            0x80, 6,
            0x80, 8,
            0x80, 10,
            0x80, 12,
            0x80, 14};
    /*z1 = gh123000|00000000 */
    vector_a_u16_512 z1 = rshift_u16_512(z, 6);
    /*z2 = 00000000|abcdefgh */
    vector_a_u16_512 z2 = shuffle_512(z, _mm512_broadcast_i32x4(t1));
    /*z = gh123000|abcdefgh */
    z = z1 | z2;
    /*z = gh123000|abcdef00 */
    z = z & broadcast_u16_512(0x3fff);
    /*z = gh123[mmm]|abcdef[mm] */
    z = z | broadcast_u16_512(0x80c0);
    *(vector_u_u16_512 *)writer = z;
}

#define __reserve_ucs2_encode_3bytes_utf8_avx512 (96)

force_inline void ucs2_encode_3bytes_utf8_avx512(u8 *writer, vector_a_u16_512 z) {
    vector_a_u8_512 t1 = {
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
            0x80, 0x80, 0x80, 0x80,
            //
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
    vector_a_u8_512 t2 = {
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
            0x80, 0x80, 0x80, 0x80,
            //
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
    vector_a_u8_512 t3 = {
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
            0x80, 0x80, 0x80, 0x80,
            //
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
    vector_a_u8_512 m1 = {
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
            0xff, 0xff, 0xff, 0xff,
            //
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
    vector_a_u8_512 m2 = {
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
            0, 0, 0, 0,
            //
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
    vector_a_u32_512 z1 = cvt_u16_to_u32_512(_mm512_extracti64x4_epi64(z, 0));
    vector_a_u32_512 z2 = cvt_u16_to_u32_512(_mm512_extracti64x4_epi64(z, 1));
    /* z3,z4 = gh123456|78000000 */
    vector_a_u16_512 z3 = rshift_u32_512(z1, 6);
    vector_a_u16_512 z4 = rshift_u32_512(z2, 6);
    /* z5,z6 = 56780000|00000000 */
    vector_a_u16_512 z5 = rshift_u32_512(z1, 12);
    vector_a_u16_512 z6 = rshift_u32_512(z2, 12);
    /* z7,z8 = 00000000|00000000|abcdefgh */
    vector_a_u8_512 z7 = shuffle_512(z1, t1);
    vector_a_u8_512 z8 = shuffle_512(z2, t1);
    /* z9,z10 = 00000000|gh123456|00000000 */
    vector_a_u8_512 z9 = shuffle_512(z3, t2);
    vector_a_u8_512 z10 = shuffle_512(z4, t2);
    /* z11,z12 = 56780000|00000000|00000000 */
    vector_a_u8_512 z11 = shuffle_512(z5, t3);
    vector_a_u8_512 z12 = shuffle_512(z6, t3);
    //
    vector_a_u8_512 z13 = ((z7 | z9 | z11) & m1) | m2;
    vector_a_u8_512 z14 = ((z8 | z10 | z12) & m1) | m2;
    // [0, 24)
    _mm512_mask_storeu_epi32(writer - 4, 0x7e, z13);
    // [48, 72)
    _mm512_mask_storeu_epi32(writer + 44, 0x7e, z14);
    // [24, 48)
    _mm512_mask_storeu_epi32(writer - 12, 0x7e00, z13);
    // [72, 96)
    _mm512_mask_storeu_epi32(writer + 36, 0x7e00, z14);
}

/* See AVX2 code for more details. */
#define __readbefore_bytes_write_ucs2_trailing_512 (0)
#define __excess_bytes_write_ucs2_trailing_512 (96 - max_json_bytes_per_unicode)

/* 
 * Encode UCS2 trailing to UTF-8.
 * Consider 3 types of vector:
 *   vector in ASCII range
 *   vector in 2-bytes range
 *   vector in 3-bytes range
 */
force_inline bool bytes_write_ucs2_trailing_512(u8 **writer_addr, const u16 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    //
    u8 *writer = *writer_addr;
    //
    u32 maskz = len_to_maskz(len);
    vector_a vec = maskz_loadu(maskz, src);
    if (len == 1) {
    one_left:;
        if (unlikely(!encode_one_ucs2(&writer, *src))) return false;
        goto finished;
    }
    u16 cur_unicode = *src;
    bool is_escaped = false;
restart:;
    int unicode_type = ucs2_get_type(cur_unicode, &is_escaped);
    switch (unicode_type) {
        case 1: {
            if (unlikely(is_escaped)) {
                is_escaped = false;
                memcpy(writer, &ControlEscapeTable_u8[cur_unicode * 8], 8);
                writer += _ControlJump[cur_unicode];
                src++;
                len--;
                if (len) {
                    if (len == 1) goto one_left;
                    maskz = maskz >> 1;
                    vec = maskz_loadu(maskz, src);
                    cur_unicode = *src;
                    goto restart;
                }
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
        avx512_bitmask_t m_not_ascii = cmpeq_bitmask(vec, broadcast(_Quote)) | cmpeq_bitmask(vec, broadcast(_Slash)) | unsigned_cmpgt_bitmask(broadcast(ControlMax), vec) | unsigned_cmpgt_bitmask(vec, broadcast(0x7f));
        m_not_ascii = m_not_ascii & maskz;
    __ascii:;
        cvt_to_dst(writer, vec);
        if (likely(m_not_ascii == 0)) {
            writer += len;
            goto finished;
        } else {
            usize done_count = escape_bitmask_to_done_count(m_not_ascii);
            u16 escape_unicode = src[done_count];
            src += done_count + 1;
            len -= done_count + 1;
            writer += done_count;
            if (escape_unicode >= ControlMax && escape_unicode < 0x80 && escape_unicode != _Slash && escape_unicode != _Quote) {
                SSRJSON_UNREACHABLE();
            } else {
                if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
            }
            if (len) {
                maskz = maskz >> (done_count + 1);
                cur_unicode = *src;
                vec = maskz_loadu(maskz, src);
                if (escape_unicode >= ControlMax && escape_unicode < 0x80 && escape_unicode != _Slash && escape_unicode != _Quote) {
                    m_not_ascii = m_not_ascii >> (done_count + 1);
                    goto __ascii;
                }
                goto restart;
            }
            goto finished;
        }
    }
_2bytes:;
    {
        avx512_bitmask_t m_not_2bytes = unsigned_cmpgt_bitmask(broadcast(0x80), vec) | unsigned_cmpgt_bitmask(vec, broadcast(0x7ff));
        m_not_2bytes = m_not_2bytes & maskz;
    __2bytes:;
        ucs2_encode_2bytes_utf8_avx512(writer, vec);
        if (likely(m_not_2bytes == 0)) {
            writer += len * 2;
            goto finished;
        } else {
            usize done_count = escape_bitmask_to_done_count(m_not_2bytes);
            u16 escape_unicode = src[done_count];
            src += done_count + 1;
            len -= done_count + 1;
            writer += done_count * 2;
            if (escape_unicode >= 0x80 && escape_unicode <= 0x7ff) {
                SSRJSON_UNREACHABLE();
            } else {
                if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
            }
            if (len) {
                maskz = maskz >> (done_count + 1);
                cur_unicode = *src;
                vec = maskz_loadu(maskz, src);
                if (cur_unicode >= 0x80 && cur_unicode <= 0x7ff) {
                    m_not_2bytes = m_not_2bytes >> (done_count + 1);
                    goto __2bytes;
                }
                goto restart;
            }
            goto finished;
        }
    }
_3bytes:;
    {
        avx512_bitmask_t m_not_3bytes = unsigned_cmpgt_bitmask(broadcast(0x800), vec) | (unsigned_cmpgt_bitmask(vec, broadcast(0xd7ff)) & unsigned_cmpgt_bitmask(broadcast(0xe000), vec));

        m_not_3bytes = m_not_3bytes & maskz;
    __3bytes:;
        ucs2_encode_3bytes_utf8_avx512(writer, vec);
        if (likely(m_not_3bytes == 0)) {
            writer += len * 3;
            goto finished;
        } else {
            usize done_count = escape_bitmask_to_done_count(m_not_3bytes);
            u16 escape_unicode = src[done_count];
            src += done_count + 1;
            len -= done_count + 1;
            writer += done_count * 3;
            if (escape_unicode >= 0x800 && (escape_unicode <= 0xd7ff || escape_unicode >= 0xe000)) {
                SSRJSON_UNREACHABLE();
            } else {
                if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
            }
            if (len) {
                maskz = maskz >> (done_count + 1);
                cur_unicode = *src;
                vec = maskz_loadu(maskz, src);
                if (cur_unicode >= 0x800 && (cur_unicode <= 0xd7ff || cur_unicode >= 0xe000)) {
                    m_not_3bytes = m_not_3bytes >> (done_count + 1);
                    goto __3bytes;
                }
                goto restart;
            }
            goto finished;
        }
    }
finished:;
    *writer_addr = writer;
    return true;
}

/* See AVX2 code for more details. */
#define __readbefore_bytes_write_ucs2_raw_utf8_trailing_512 (0)
#define __excess_bytes_write_ucs2_raw_utf8_trailing_512 (96 - max_utf8_bytes_per_ucs2)

force_inline bool bytes_write_ucs2_raw_utf8_trailing_512(u8 **writer_addr, const u16 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    //
    u8 *writer = *writer_addr;
    //
    u32 maskz = len_to_maskz(len);
    vector_a vec = maskz_loadu(maskz, src);
    if (len == 1) {
    one_left:;
        if (unlikely(!encode_one_ucs2_noescape(&writer, *src))) return false;
        goto finished;
    }
    u16 cur_unicode = *src;
    bool is_escaped_unused;
restart:;
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
        avx512_bitmask_t m_not_ascii = unsigned_cmpge_bitmask(vec, broadcast(0x80));
        m_not_ascii = m_not_ascii & maskz;
    __ascii:;
        cvt_to_dst(writer, vec);
        if (likely(m_not_ascii == 0)) {
            writer += len;
            goto finished;
        } else {
            usize done_count = escape_bitmask_to_done_count(m_not_ascii);
            u16 escape_unicode = src[done_count];
            src += done_count + 1;
            len -= done_count + 1;
            writer += done_count;
            assume(escape_unicode >= 128);
            if (unlikely(!encode_one_ucs2_noescape(&writer, escape_unicode))) return false;
            if (len) {
                maskz = maskz >> (done_count + 1);
                cur_unicode = *src;
                vec = maskz_loadu(maskz, src);
                if (escape_unicode >= ControlMax && escape_unicode < 0x80 && escape_unicode != _Slash && escape_unicode != _Quote) {
                    m_not_ascii = m_not_ascii >> (done_count + 1);
                    goto __ascii;
                }
                goto restart;
            }
            goto finished;
        }
        // ---unreachable here---
    }
_2bytes:;
    {
        avx512_bitmask_t m_not_2bytes = unsigned_cmpgt_bitmask(broadcast(0x80), vec) | unsigned_cmpgt_bitmask(vec, broadcast(0x7ff));
        m_not_2bytes = m_not_2bytes & maskz;
    __2bytes:;
        ucs2_encode_2bytes_utf8_avx512(writer, vec);
        if (likely(m_not_2bytes == 0)) {
            writer += len * 2;
            goto finished;
        } else {
            usize done_count = escape_bitmask_to_done_count(m_not_2bytes);
            u16 escape_unicode = src[done_count];
            src += done_count + 1;
            len -= done_count + 1;
            writer += done_count * 2;
            assume(!(escape_unicode >= 0x80 && escape_unicode <= 0x7ff));
            if (unlikely(!encode_one_ucs2_noescape(&writer, escape_unicode))) return false;
            if (len) {
                maskz = maskz >> (done_count + 1);
                cur_unicode = *src;
                vec = maskz_loadu(maskz, src);
                if (cur_unicode >= 0x80 && cur_unicode <= 0x7ff) {
                    m_not_2bytes = m_not_2bytes >> (done_count + 1);
                    goto __2bytes;
                }
                goto restart;
            }
            goto finished;
        }
        // ---unreachable here---
    }
_3bytes:;
    {
        avx512_bitmask_t m_not_3bytes = unsigned_cmpgt_bitmask(broadcast(0x800), vec) | (unsigned_cmpgt_bitmask(vec, broadcast(0xd7ff)) & unsigned_cmpgt_bitmask(broadcast(0xe000), vec));

        m_not_3bytes = m_not_3bytes & maskz;
    __3bytes:;
        ucs2_encode_3bytes_utf8_avx512(writer, vec);
        if (likely(m_not_3bytes == 0)) {
            writer += len * 3;
            goto finished;
        } else {
            usize done_count = escape_bitmask_to_done_count(m_not_3bytes);
            u16 escape_unicode = src[done_count];
            src += done_count + 1;
            len -= done_count + 1;
            writer += done_count * 3;
            assume(!(escape_unicode >= 0x800 && (escape_unicode <= 0xd7ff || escape_unicode >= 0xe000)));
            if (unlikely(!encode_one_ucs2_noescape(&writer, escape_unicode))) return false;
            if (len) {
                maskz = maskz >> (done_count + 1);
                cur_unicode = *src;
                vec = maskz_loadu(maskz, src);
                if (cur_unicode >= 0x800 && (cur_unicode <= 0xd7ff || cur_unicode >= 0xe000)) {
                    m_not_3bytes = m_not_3bytes >> (done_count + 1);
                    goto __3bytes;
                }
                goto restart;
            }
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

#endif // SSRJSON_SIMD_AVX512VLDQBW_ENCODE_BYTES_UCS2_H

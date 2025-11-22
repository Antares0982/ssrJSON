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

#ifndef SSRJSON_SIMD_NEON_ENCODE_BYTES_UCS1_H
#define SSRJSON_SIMD_NEON_ENCODE_BYTES_UCS1_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "encode/encode_utf8_shared.h"
#include "simd/neon/checker.h"
#include "simd/neon/common.h"
//
#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#define COMPILE_SIMD_BITS 128
#include "compile_context/srw_in.inl.h"

/* See AVX2 code for more details. */
#define __readbefore_bytes_write_ucs1_trailing_128 (16)
#define __excess_bytes_write_ucs1_trailing_128 (16 - max_json_bytes_per_unicode)

/* 
 * Encode UCS1 trailing to UTF-8.
 * Only consider vector in ASCII range,
 * because most of 2-bytes UTF-8 code points cannot be presented by UCS1.
 */
force_inline void bytes_write_ucs1_trailing_128(u8 **writer_addr, const u8 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    // constants
    const u8 *src_end = src + len;
    const u8 *last_batch_start = src_end - READ_BATCH_COUNT;
    const vector_a vec = *(const vector_u *)last_batch_start;
    const vector_a m0 = (vec == broadcast(_Quote)) | (vec == broadcast(_Slash)) | signed_cmpgt(broadcast(ControlMax), vec);
    //
    u8 *writer = *writer_addr;
restart:;
    vector_a x, m;
    int shift;
    shift = SSRJSON_CAST(int, READ_BATCH_COUNT - len);
    x = runtime_byte_rshift_128(vec, shift);
    m = runtime_byte_rshift_128(m0, shift);
    *(vector_u *)writer = x;
    if (likely(testz(m))) {
        writer += len;
    } else {
        usize done_count = escape_mask_to_done_count(m);
        assert(done_count < len);
        len -= done_count + 1;
        writer += done_count;
        src += done_count;
        u8 unicode = *src++;
        encode_one_special_ucs1(&writer, unicode);
        if (len) goto restart;
    }
    *writer_addr = writer;
}

/* See AVX2 code for more details. */
#define __readbefore_bytes_write_ucs1_raw_utf8_trailing_128 (16)
#define __excess_bytes_write_ucs1_raw_utf8_trailing_128 (16 - max_utf8_bytes_per_ucs1)

force_inline void bytes_write_ucs1_raw_utf8_trailing_128(u8 **writer_addr, const u8 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    // constants
    const u8 *src_end = src + len;
    const u8 *last_batch_start = src_end - READ_BATCH_COUNT;
    const vector_a vec = *(const vector_u *)last_batch_start;
    const vector_a m0 = (vec >= broadcast(0x80));
    //
    u8 *writer = *writer_addr;
restart:;
    vector_a x, m;
    int shift;
    shift = SSRJSON_CAST(int, READ_BATCH_COUNT - len);
    x = runtime_byte_rshift_128(vec, shift);
    m = runtime_byte_rshift_128(m0, shift);
    *(vector_u *)writer = x;
    if (likely(testz(m))) {
        writer += len;
    } else {
        usize done_count = escape_mask_to_done_count(m);
        assert(done_count < len);
        len -= done_count + 1;
        writer += done_count;
        src += done_count;
        u8 unicode = *src++;

        *writer++ = (unicode >> 6) | 0xc0;
        *writer++ = (unicode & 0x3f) | 0x80;

        if (len) goto restart;
    }
    *writer_addr = writer;
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_SIMD_BITS
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#endif // SSRJSON_SIMD_NEON_ENCODE_BYTES_UCS1_H

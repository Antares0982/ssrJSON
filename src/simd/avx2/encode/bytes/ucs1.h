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

#ifndef SSRJSON_SIMD_AVX2_ENCODE_BYTES_UCS1_H
#define SSRJSON_SIMD_AVX2_ENCODE_BYTES_UCS1_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "encode/encode_utf8_shared.h"
#include "simd/avx2/checker.h"
#include "simd/avx2/common.h"
#include "simd/avx2/cvt.h"
//
#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#define COMPILE_SIMD_BITS 256
#include "compile_context/srw_in.inl.h"

#define __readbefore_bytes_write_ucs1_trailing_256 (32)
#define __excess_bytes_write_ucs1_trailing_256 (32 - max_json_bytes_per_unicode)

/* 
 * Encode UCS1 trailing to UTF-8.
 * Only consider vector in ASCII range,
 * because most of 2-bytes UTF-8 code points cannot be presented by UCS1.
 */
force_inline void bytes_write_ucs1_trailing_256(u8 **writer_addr, const u8 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    // constants
    const u8 *src_end = src + len;
    const u8 *last_batch_start = src_end - READ_BATCH_COUNT;
    const vector_a vec = *(const vector_u *)last_batch_start;
    const vector_a m0 = (vec == broadcast(_Quote)) | (vec == broadcast(_Slash)) | signed_cmpgt(broadcast(ControlMax), vec);
    //
    u8 *writer = *writer_addr;
restart:;
    vector_a m = high_mask(m0, len);
    // excess bytes written: 32 - max_json_bytes_per_unicode (at least one unicode in src)
    avx2_trailing_cvt(src, src_end, writer);
    if (likely(testz(m))) {
        writer += len;
    } else {
        // excess bytes written: 8 - max_json_bytes_per_unicode
        usize done_count = escape_mask_to_done_count_no_eq0(m);
        assert(done_count >= READ_BATCH_COUNT - len);
        usize real_done_count = done_count - (READ_BATCH_COUNT - len);
        len = READ_BATCH_COUNT - done_count - 1;
        writer += real_done_count;
        src = last_batch_start + done_count + 1;
        u8 unicode = last_batch_start[done_count];
        assume(!(unicode >= ControlMax && unicode < 0x80 && unicode != _Slash && unicode != _Quote));
        encode_one_special_ucs1(&writer, unicode);
        if (len) goto restart;
    }
    *writer_addr = writer;
}

#define __readbefore_bytes_write_ucs1_raw_utf8_trailing_256 (32)
#define __excess_bytes_write_ucs1_raw_utf8_trailing_256 (32 - max_utf8_bytes_per_ucs1)

force_inline void bytes_write_ucs1_raw_utf8_trailing_256(u8 **writer_addr, const u8 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    // constants
    const u8 *src_end = src + len;
    const u8 *last_batch_start = src_end - READ_BATCH_COUNT;
    const vector_a vec = *(const vector_u *)last_batch_start;
    const vector_a m0 = signed_cmpgt(broadcast(0), vec);
    //
    u8 *writer = *writer_addr;
restart:;
    vector_a m = high_mask(m0, len);
    // excess bytes written: 32 - max_utf8_bytes_per_ucs1 (at least one ucs1 in src)
    avx2_trailing_cvt(src, src_end, writer);
    if (likely(testz(m))) {
        writer += len;
    } else {
        usize done_count = escape_anymask_to_done_count_no_eq0(m);
        assert(done_count >= READ_BATCH_COUNT - len);
        usize real_done_count = done_count - (READ_BATCH_COUNT - len);
        len = READ_BATCH_COUNT - done_count - 1;
        writer += real_done_count;
        src = last_batch_start + done_count + 1;
        u8 unicode = last_batch_start[done_count];

        // no excess bytes
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

#endif // SSRJSON_SIMD_AVX2_ENCODE_BYTES_UCS1_H

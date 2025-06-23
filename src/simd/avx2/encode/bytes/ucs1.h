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

/* 
 * Encode UCS1 trailing to utf-8.
 * Only consider vector in ASCII range,
 * because most of 2-bytes utf-8 code points cannot be presented by UCS1.
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
    cvt_to_dst_blendhigh(writer + len - READ_BATCH_COUNT, vec, len);
    if (likely(testz(m))) {
        writer += len;
    } else {
        usize done_count = escape_mask_to_done_count_no_eq0(m);
        assert(done_count >= READ_BATCH_COUNT - len);
        usize real_done_count = done_count - (READ_BATCH_COUNT - len);
        len = READ_BATCH_COUNT - done_count - 1;
        writer += real_done_count;
        src = last_batch_start + done_count + 1;
        u8 unicode = last_batch_start[done_count];
        if (unicode >= ControlMax && unicode < 0x80 && unicode != _Slash && unicode != _Quote) {
            SSRJSON_UNREACHABLE();
        } else {
            encode_one_special_ucs1(&writer, unicode);
        }
        if (len) goto restart;
    }
    *writer_addr = writer;
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_SIMD_BITS
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#endif // SSRJSON_SIMD_AVX2_ENCODE_BYTES_UCS1_H

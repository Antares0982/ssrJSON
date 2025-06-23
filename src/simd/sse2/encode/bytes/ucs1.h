#ifndef SSRJSON_SIMD_SSE2_ENCODE_BYTES_UCS1_H
#define SSRJSON_SIMD_SSE2_ENCODE_BYTES_UCS1_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "encode/encode_utf8_shared.h"
#include "simd/sse2/checker.h"
#include "simd/sse2/common.h"
//
#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#define COMPILE_SIMD_BITS 128
#include "compile_context/srw_in.inl.h"

/* 
 * Encode UCS1 trailing to utf-8.
 * Only consider vector in ASCII range,
 * because most of 2-bytes utf-8 code points cannot be presented by UCS1.
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
        usize done_count = escape_mask_to_done_count_no_eq0(m);
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

#include "compile_context/srw_out.inl.h"
#undef COMPILE_SIMD_BITS
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#endif // SSRJSON_SIMD_SSE2_ENCODE_BYTES_UCS1_H

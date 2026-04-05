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

#ifndef SSRJSON_ENCODE_UTF8_H
#define SSRJSON_ENCODE_UTF8_H
#ifdef SSRJSON_CLANGD_CHECKING
#    ifndef COMPILE_CONTEXT_ENCODE
#        define COMPILE_CONTEXT_ENCODE
#    endif
#    include "simd/simd_detect.h"
#    include "simd/simd_impl.h"
#endif
#include "simd/union_vector.h"
//
#include "simd/compile_feature_check.h"
/* write to u8, COMPILE_WRITE_UCS_LEVEL is always 1.*/
#define COMPILE_WRITE_UCS_LEVEL 1
/* ASCII and UCS1: COMPILE_READ_UCS_LEVEL is 1. */
#define COMPILE_READ_UCS_LEVEL 1
#include "compile_context/srw_in.inl.h"

/* UTF-8 src. */
// forward declaration
force_inline ssrjson_nofail u8 *bytes_write_ascii(u8 *writer, const u8 *src, usize len, ssrjson_compiletime bool is_key);

force_inline ssrjson_nofail u8 *bytes_write_utf8(u8 *writer, const u8 *src, usize len, ssrjson_compiletime bool is_key) {
    // UTF-8 trailing source case is a little different,
    // because the 16 bytes before `src_end` are not readable in general.
    // This function only impls the fast path that we can reuse the unicode encode loop,
    // so a check is needed.
    // AVX512 case: impl of encode_trailing_copy_with_cvt uses mask load, which is safe;
    // AVX2 case: avx2_trailing_cvt series require 16 bytes before src_end are readable;
    // Other cases: SIMD register size is 16 bytes (SSE, NEON).
    assert(USING_AVX512 || len >= 16);
    // reuse the ascii encode loop.
    return bytes_write_ascii(writer, src, len, is_key);
}

/* ASCII src. */
force_inline ssrjson_nofail u8 *bytes_write_ascii(u8 *writer, const u8 *src, usize len, ssrjson_compiletime bool is_key) {
    // reuse the unicode encode loop.
    // excess written bytes = ssrjson_max(READ_BATCH_COUNT, 8) - max_json_bytes_per_unicode >= 2
    if (ssrjson_consteval(!is_key)) writer = encode_unicode_loop4(writer, &src, &len);
    writer = encode_unicode_loop(writer, &src, &len);
    if (len) writer = encode_trailing_copy_with_cvt(writer, src, len);
    return writer;
}

/* UCS1 src. */
force_inline void check_ascii_in_ucs1_and_get_done_countx4(unionvector_a_x4 vec, bool *out_checked, usize *out_done_count) {
    vector_a *checker_masks = (vector_a *)&_CheckerMasks;

    vector_a t1 = checker_masks[0];
    vector_a t2 = checker_masks[1];
    vector_a t3 = checker_masks[2];
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    struct {
        u64 x[4];
    } m;

    u64 r;

    m.x[0] = cmpeq_bitmask(vec.x[0], t1) |
             cmpeq_bitmask(vec.x[0], t2) |
             signed_cmpgt_bitmask(t3, vec.x[0]);
    m.x[1] = cmpeq_bitmask(vec.x[1], t1) |
             cmpeq_bitmask(vec.x[1], t2) |
             signed_cmpgt_bitmask(t3, vec.x[1]);
    m.x[2] = cmpeq_bitmask(vec.x[2], t1) |
             cmpeq_bitmask(vec.x[2], t2) |
             signed_cmpgt_bitmask(t3, vec.x[2]);
    m.x[3] = cmpeq_bitmask(vec.x[3], t1) |
             cmpeq_bitmask(vec.x[3], t2) |
             signed_cmpgt_bitmask(t3, vec.x[3]);
#else
    // see CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
    unionvector_a_x4 m;
    vector_a r;
    m.x[0] = (vec.x[0] == t1) | (vec.x[0] == t2) | signed_cmpgt(t3, vec.x[0]);
    m.x[1] = (vec.x[1] == t1) | (vec.x[1] == t2) | signed_cmpgt(t3, vec.x[1]);
    m.x[2] = (vec.x[2] == t1) | (vec.x[2] == t2) | signed_cmpgt(t3, vec.x[2]);
    m.x[3] = (vec.x[3] == t1) | (vec.x[3] == t2) | signed_cmpgt(t3, vec.x[3]);
#endif

    r = m.x[0] | m.x[1];
    r = r | (m.x[2] | m.x[3]);
    //
    bool checked = testz_escape_mask(r);
    *out_checked = checked;
    //
    if (unlikely(!checked)) {
        usize done_count = 0;
        for (int i = 0; i < 4; ++i) {
            if (testz_escape_mask(m.x[i])) {
                done_count += READ_BATCH_COUNT;
            } else {
                done_count += escape_anymask_to_done_count_no_eq0(m.x[i]);
                break;
            }
        }
        *out_done_count = done_count;
    }
}

force_inline void check_ascii_in_ucs1_raw_utf8_and_get_done_countx4(unionvector_a_x4 vec, bool *out_checked, usize *out_done_count) {
    vector_a t = broadcast(0);
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    struct {
        u64 x[4];
    } m;

    u64 r;

    m.x[0] = signed_cmpgt_bitmask(t, vec.x[0]);
    m.x[1] = signed_cmpgt_bitmask(t, vec.x[1]);
    m.x[2] = signed_cmpgt_bitmask(t, vec.x[2]);
    m.x[3] = signed_cmpgt_bitmask(t, vec.x[3]);
#else
    unionvector_a_x4 m;
    vector_a r;
    m.x[0] = signed_cmpgt(t, vec.x[0]);
    m.x[1] = signed_cmpgt(t, vec.x[1]);
    m.x[2] = signed_cmpgt(t, vec.x[2]);
    m.x[3] = signed_cmpgt(t, vec.x[3]);
#endif

    r = m.x[0] | m.x[1];
    r = r | (m.x[2] | m.x[3]);
    //
    bool checked = testz_escape_mask(r);
    *out_checked = checked;
    //
    if (unlikely(!checked)) {
        usize done_count = 0;
        for (int i = 0; i < 4; ++i) {
            if (testz_escape_mask(m.x[i])) {
                done_count += READ_BATCH_COUNT;
            } else {
                done_count += escape_anymask_to_done_count_no_eq0(m.x[i]);
                break;
            }
        }
        *out_done_count = done_count;
    }
}

force_inline void check_ascii_in_ucs1_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a *checker_masks = (vector_a *)&_CheckerMasks;

    vector_a t1 = checker_masks[0];
    vector_a t2 = checker_masks[1];
    vector_a t3 = checker_masks[2];
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    u64 m;

    m = cmpeq_bitmask(vec, t1) |
        cmpeq_bitmask(vec, t2) |
        signed_cmpgt_bitmask(t3, vec);
#else
    // see CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
    vector_a m;
    m = (vec == t1) | (vec == t2) | signed_cmpgt(t3, vec);
#endif
    bool checked = testz_escape_mask(m);
    *out_checked = checked;
    if (unlikely(!checked)) {
        *out_done_count = escape_anymask_to_done_count_no_eq0(m);
    }
}

force_inline void check_ascii_in_ucs1_raw_utf8_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t = broadcast(0);
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    u64 m;

    m = signed_cmpgt_bitmask(t, vec);
#else
    vector_a m;
    m = signed_cmpgt(t, vec);
#endif
    bool checked = testz_escape_mask(m);
    *out_checked = checked;
    if (unlikely(!checked)) {
        *out_done_count = escape_anymask_to_done_count_no_eq0(m);
    }
}

force_inline ssrjson_nofail u8 *ascii_in_ucs1_encode_loop4(u8 *dst, const u8 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u8 *src = *src_addr;
    usize len = *len_addr;

    unionvector_a_x4 vec;

    // read
    vec.x[0] = *(const vector_u *)(src + READ_BATCH_COUNT * 0);
    vec.x[1] = *(const vector_u *)(src + READ_BATCH_COUNT * 1);
    vec.x[2] = *(const vector_u *)(src + READ_BATCH_COUNT * 2);
    vec.x[3] = *(const vector_u *)(src + READ_BATCH_COUNT * 3);

    // write
    *(vector_u *)(dst + READ_BATCH_COUNT * 0) = vec.x[0];
    *(vector_u *)(dst + READ_BATCH_COUNT * 1) = vec.x[1];
    *(vector_u *)(dst + READ_BATCH_COUNT * 2) = vec.x[2];
    *(vector_u *)(dst + READ_BATCH_COUNT * 3) = vec.x[3];

    // check
    // bool checked;
    usize done_count;
    check_ascii_in_ucs1_and_get_done_countx4(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += 4 * READ_BATCH_COUNT;
        src += 4 * READ_BATCH_COUNT;
        len -= 4 * READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline ssrjson_nofail u8 *ascii_in_ucs1_encode_loop(u8 *dst, const u8 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u8 *src = *src_addr;
    usize len = *len_addr;

    // read
    vector_a vec = *(const vector_u *)src;

    // write
    *(vector_u *)dst = vec;

    // check
    usize done_count;
    check_ascii_in_ucs1_and_get_done_count(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += READ_BATCH_COUNT;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline ssrjson_nofail u8 *ascii_in_ucs1_encode_loop4_raw_utf8(u8 *dst, const u8 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u8 *src = *src_addr;
    usize len = *len_addr;

    unionvector_a_x4 vec;

    // read
    vec.x[0] = *(const vector_u *)(src + READ_BATCH_COUNT * 0);
    vec.x[1] = *(const vector_u *)(src + READ_BATCH_COUNT * 1);
    vec.x[2] = *(const vector_u *)(src + READ_BATCH_COUNT * 2);
    vec.x[3] = *(const vector_u *)(src + READ_BATCH_COUNT * 3);

    // write
    *(vector_u *)(dst + READ_BATCH_COUNT * 0) = vec.x[0];
    *(vector_u *)(dst + READ_BATCH_COUNT * 1) = vec.x[1];
    *(vector_u *)(dst + READ_BATCH_COUNT * 2) = vec.x[2];
    *(vector_u *)(dst + READ_BATCH_COUNT * 3) = vec.x[3];

    // check
    usize done_count;
    check_ascii_in_ucs1_raw_utf8_and_get_done_countx4(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += 4 * READ_BATCH_COUNT;
        src += 4 * READ_BATCH_COUNT;
        len -= 4 * READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline ssrjson_nofail u8 *ascii_in_ucs1_encode_loop_raw_utf8(u8 *dst, const u8 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u8 *src = *src_addr;
    usize len = *len_addr;

    // read
    vector_a vec = *(const vector_u *)src;

    // write
    *(vector_u *)dst = vec;

    // check
    usize done_count;
    check_ascii_in_ucs1_raw_utf8_and_get_done_count(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += READ_BATCH_COUNT;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline ssrjson_nofail u8 *bytes_write_ucs1(u8 *writer, const u8 *src, usize len, ssrjson_compiletime bool is_key) {
#define CAN_LOOP4 (len >= 4 * READ_BATCH_COUNT)
#define CAN_LOOP (len >= READ_BATCH_COUNT)
    while (CAN_LOOP) {
        u8 unicode;
        unicode = *src;
        if (unicode < 128 && unicode >= _ControlMax && unicode != _Quote && unicode != _Slash) {
            bool continuous;
            if (ssrjson_consteval(!is_key)) {
                while (CAN_LOOP4) {
                    writer = ascii_in_ucs1_encode_loop4(writer, &src, &len, &continuous);
                    if (unlikely(!continuous)) {
                        goto encode_one;
                    }
                }
                assert(!CAN_LOOP4);
            }
            while (CAN_LOOP) {
                writer = ascii_in_ucs1_encode_loop(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else {
            goto do_encode_one;
        }
    encode_one:;
        unicode = *src;
    do_encode_one:;
        writer = encode_one_ucs1(writer, unicode);
        src++;
        len--;
    }
    if (len) writer = bytes_write_ucs1_trailing(writer, src, len);
    return writer;
#undef CAN_LOOP
#undef CAN_LOOP4
}

force_inline ssrjson_nofail u8 *bytes_write_ucs1_raw_utf8(u8 *writer, const u8 *src, usize len, ssrjson_compiletime bool is_key) {
#define CAN_LOOP4 (len >= 4 * READ_BATCH_COUNT)
#define CAN_LOOP (len >= READ_BATCH_COUNT)
    while (CAN_LOOP) {
        u8 unicode;
        unicode = *src;
        if (unicode < 128) {
            bool continuous;
            if (ssrjson_consteval(!is_key)) {
                while (CAN_LOOP4) {
                    writer = ascii_in_ucs1_encode_loop4_raw_utf8(writer, &src, &len, &continuous);
                    if (unlikely(!continuous)) {
                        goto encode_one;
                    }
                }
                assert(!CAN_LOOP4);
            }
            while (CAN_LOOP) {
                writer = ascii_in_ucs1_encode_loop_raw_utf8(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else {
            goto do_encode_one;
        }
    encode_one:;
        unicode = *src;
    do_encode_one:;
        writer = encode_one_ucs1_noescape(writer, unicode);
        src++;
        len--;
    }
    if (len) writer = bytes_write_ucs1_raw_utf8_trailing(writer, src, len);
    return writer;
#undef CAN_LOOP
#undef CAN_LOOP4
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

/* UCS2 src. */
#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 1
#include "compile_context/srw_in.inl.h"

force_inline void check_ascii_in_ucs2_and_get_done_countx4(unionvector_a_x4 vec, bool *out_checked, usize *out_done_count) {
    vector_a *checker_masks = (vector_a *)&_CheckerMasks;

    vector_a t1 = checker_masks[0];
    vector_a t2 = checker_masks[1];
    vector_a t3 = checker_masks[2];
    vector_a t4 = broadcast(0x7f);
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    struct {
        u32 x[4];
    } m;

    u32 r;
    m.x[0] = cmpeq_bitmask(vec.x[0], t1) |
             cmpeq_bitmask(vec.x[0], t2) |
             signed_cmpgt_bitmask(t3, vec.x[0]) |
             signed_cmpgt_bitmask(vec.x[0], t4);
    m.x[1] = cmpeq_bitmask(vec.x[1], t1) |
             cmpeq_bitmask(vec.x[1], t2) |
             signed_cmpgt_bitmask(t3, vec.x[1]) |
             signed_cmpgt_bitmask(vec.x[1], t4);
    m.x[2] = cmpeq_bitmask(vec.x[2], t1) |
             cmpeq_bitmask(vec.x[2], t2) |
             signed_cmpgt_bitmask(t3, vec.x[2]) |
             signed_cmpgt_bitmask(vec.x[2], t4);
    m.x[3] = cmpeq_bitmask(vec.x[3], t1) |
             cmpeq_bitmask(vec.x[3], t2) |
             signed_cmpgt_bitmask(t3, vec.x[3]) |
             signed_cmpgt_bitmask(vec.x[3], t4);
#elif SSRJSON_IS_X64
    // see CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
    unionvector_a_x4 m;
    vector_a r;
    m.x[0] = (vec.x[0] == t1) | (vec.x[0] == t2) | signed_cmpgt(t3, vec.x[0]) | signed_cmpgt(vec.x[0], t4);
    m.x[1] = (vec.x[1] == t1) | (vec.x[1] == t2) | signed_cmpgt(t3, vec.x[1]) | signed_cmpgt(vec.x[1], t4);
    m.x[2] = (vec.x[2] == t1) | (vec.x[2] == t2) | signed_cmpgt(t3, vec.x[2]) | signed_cmpgt(vec.x[2], t4);
    m.x[3] = (vec.x[3] == t1) | (vec.x[3] == t2) | signed_cmpgt(t3, vec.x[3]) | signed_cmpgt(vec.x[3], t4);
#elif SSRJSON_IS_AARCH64
    unionvector_a_x4 m;
    vector_a r;
    m.x[0] = (vec.x[0] == t1) | (vec.x[0] == t2) | (vec.x[0] < t3) | (vec.x[0] > t4);
    m.x[1] = (vec.x[1] == t1) | (vec.x[1] == t2) | (vec.x[1] < t3) | (vec.x[1] > t4);
    m.x[2] = (vec.x[2] == t1) | (vec.x[2] == t2) | (vec.x[2] < t3) | (vec.x[2] > t4);
    m.x[3] = (vec.x[3] == t1) | (vec.x[3] == t2) | (vec.x[3] < t3) | (vec.x[3] > t4);
#endif

    r = m.x[0] | m.x[1];
    r = r | (m.x[2] | m.x[3]);
    //
    bool checked = testz_escape_mask(r);
    *out_checked = checked;
    if (unlikely(!checked)) {
        usize done_count = 0;
        for (int i = 0; i < 4; ++i) {
            if (testz_escape_mask(m.x[i])) {
                done_count += READ_BATCH_COUNT;
            } else {
                done_count += escape_anymask_to_done_count_no_eq0(m.x[i]);
                break;
            }
        }
        *out_done_count = done_count;
    }
}

force_inline void check_ascii_in_ucs2_raw_utf8_and_get_done_countx4(unionvector_a_x4 vec, bool *out_checked, usize *out_done_count) {
    vector_a t3 = broadcast(0);
    vector_a t4 = broadcast(0x7f);
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    struct {
        u32 x[4];
    } m;

    u32 r;
    m.x[0] = signed_cmpgt_bitmask(t3, vec.x[0]) |
             signed_cmpgt_bitmask(vec.x[0], t4);
    m.x[1] = signed_cmpgt_bitmask(t3, vec.x[1]) |
             signed_cmpgt_bitmask(vec.x[1], t4);
    m.x[2] = signed_cmpgt_bitmask(t3, vec.x[2]) |
             signed_cmpgt_bitmask(vec.x[2], t4);
    m.x[3] = signed_cmpgt_bitmask(t3, vec.x[3]) |
             signed_cmpgt_bitmask(vec.x[3], t4);
#elif SSRJSON_IS_X64
    // see CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
    unionvector_a_x4 m;
    vector_a r;
    m.x[0] = signed_cmpgt(t3, vec.x[0]) | signed_cmpgt(vec.x[0], t4);
    m.x[1] = signed_cmpgt(t3, vec.x[1]) | signed_cmpgt(vec.x[1], t4);
    m.x[2] = signed_cmpgt(t3, vec.x[2]) | signed_cmpgt(vec.x[2], t4);
    m.x[3] = signed_cmpgt(t3, vec.x[3]) | signed_cmpgt(vec.x[3], t4);
#elif SSRJSON_IS_AARCH64
    unionvector_a_x4 m;
    vector_a r;
    m.x[0] = (vec.x[0] < t3) | (vec.x[0] > t4);
    m.x[1] = (vec.x[1] < t3) | (vec.x[1] > t4);
    m.x[2] = (vec.x[2] < t3) | (vec.x[2] > t4);
    m.x[3] = (vec.x[3] < t3) | (vec.x[3] > t4);
#endif

    r = m.x[0] | m.x[1];
    r = r | (m.x[2] | m.x[3]);
    //
    bool checked = testz_escape_mask(r);
    *out_checked = checked;
    if (unlikely(!checked)) {
        usize done_count = 0;
        for (int i = 0; i < 4; ++i) {
            if (testz_escape_mask(m.x[i])) {
                done_count += READ_BATCH_COUNT;
            } else {
                done_count += escape_anymask_to_done_count_no_eq0(m.x[i]);
                break;
            }
        }
        *out_done_count = done_count;
    }
}

force_inline void check_ascii_in_ucs2_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a *checker_masks = (vector_a *)&_CheckerMasks;

    vector_a t1 = checker_masks[0];
    vector_a t2 = checker_masks[1];
    vector_a t3 = checker_masks[2];
    vector_a t4 = broadcast(0x7f);
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    u32 m;
    m = cmpeq_bitmask(vec, t1) |
        cmpeq_bitmask(vec, t2) |
        signed_cmpgt_bitmask(t3, vec) |
        signed_cmpgt_bitmask(vec, t4);
#elif SSRJSON_IS_X64
    vector_a m;
    m = (vec == t1) | (vec == t2) | signed_cmpgt(t3, vec) | signed_cmpgt(vec, t4);
#elif SSRJSON_IS_AARCH64
    vector_a m;
    m = (vec == t1) | (vec == t2) | (vec < t3) | (vec > t4);
#endif
    bool checked = testz_escape_mask(m);
    *out_checked = checked;
    if (unlikely(!checked)) {
        *out_done_count = escape_anymask_to_done_count_no_eq0(m);
    }
}

force_inline void check_ascii_in_ucs2_raw_utf8_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t3 = broadcast(0);
    vector_a t4 = broadcast(0x7f);
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    u32 m;
    m = signed_cmpgt_bitmask(t3, vec) |
        signed_cmpgt_bitmask(vec, t4);
#elif SSRJSON_IS_X64
    vector_a m;
    m = signed_cmpgt(t3, vec) | signed_cmpgt(vec, t4);
#elif SSRJSON_IS_AARCH64
    vector_a m;
    m = (vec < t3) | (vec > t4);
#endif
    bool checked = testz_escape_mask(m);
    *out_checked = checked;
    if (unlikely(!checked)) {
        *out_done_count = escape_anymask_to_done_count_no_eq0(m);
    }
}

force_inline ssrjson_nofail u8 *ascii_in_ucs2_encode_loop4(u8 *dst, const u16 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u16 *src = *src_addr;
    usize len = *len_addr;

    unionvector_a_x4 vec;

    // read
    vec.x[0] = *(const vector_u *)(src + READ_BATCH_COUNT * 0);
    vec.x[1] = *(const vector_u *)(src + READ_BATCH_COUNT * 1);
    vec.x[2] = *(const vector_u *)(src + READ_BATCH_COUNT * 2);
    vec.x[3] = *(const vector_u *)(src + READ_BATCH_COUNT * 3);

    // write
    cvt_to_dst(dst + READ_BATCH_COUNT * 0, vec.x[0]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 1, vec.x[1]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 2, vec.x[2]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 3, vec.x[3]);

    // check
    usize done_count;
    check_ascii_in_ucs2_and_get_done_countx4(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += 4 * READ_BATCH_COUNT;
        src += 4 * READ_BATCH_COUNT;
        len -= 4 * READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline ssrjson_nofail u8 *ascii_in_ucs2_encode_loop4_raw_utf8(u8 *dst, const u16 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u16 *src = *src_addr;
    usize len = *len_addr;

    unionvector_a_x4 vec;

    // read
    vec.x[0] = *(const vector_u *)(src + READ_BATCH_COUNT * 0);
    vec.x[1] = *(const vector_u *)(src + READ_BATCH_COUNT * 1);
    vec.x[2] = *(const vector_u *)(src + READ_BATCH_COUNT * 2);
    vec.x[3] = *(const vector_u *)(src + READ_BATCH_COUNT * 3);

    // write
    cvt_to_dst(dst + READ_BATCH_COUNT * 0, vec.x[0]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 1, vec.x[1]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 2, vec.x[2]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 3, vec.x[3]);

    // check
    usize done_count;

    check_ascii_in_ucs2_raw_utf8_and_get_done_countx4(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += 4 * READ_BATCH_COUNT;
        src += 4 * READ_BATCH_COUNT;
        len -= 4 * READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline ssrjson_nofail u8 *ascii_in_ucs2_encode_loop(u8 *dst, const u16 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u16 *src = *src_addr;
    usize len = *len_addr;

    vector_a vec;

    // read
    vec = *(const vector_u *)src;

    // write
    cvt_to_dst(dst, vec);

    // check
    usize done_count;
    check_ascii_in_ucs2_and_get_done_count(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += READ_BATCH_COUNT;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline ssrjson_nofail u8 *ascii_in_ucs2_encode_loop_raw_utf8(u8 *dst, const u16 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u16 *src = *src_addr;
    usize len = *len_addr;

    vector_a vec;

    // read
    vec = *(const vector_u *)src;

    // write
    cvt_to_dst(dst, vec);

    // check
    usize done_count;
    check_ascii_in_ucs2_raw_utf8_and_get_done_count(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += READ_BATCH_COUNT;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline void check_2bytes_in_ucs2_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = broadcast(0x80);
    vector_a t2 = broadcast(0x7ff);
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    u32 m;
    m = unsigned_cmpgt_bitmask(t1, vec) | unsigned_cmpgt_bitmask(vec, t2);
#elif SSRJSON_IS_X64
    vector_a m;
    m = signed_cmpgt(t1, vec) | signed_cmpgt(vec, t2);
#else
    vector_a m;
    m = (vec < t1) | (vec > t2);
#endif
    bool checked = testz_escape_mask(m);
    *out_checked = checked;
    if (unlikely(!checked)) {
        *out_done_count = escape_anymask_to_done_count_no_eq0(m);
    }
}

force_inline ssrjson_nofail u8 *_2bytes_in_ucs2_encode_loop(u8 *dst, const u16 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u16 *src = *src_addr;
    usize len = *len_addr;

    vector_a vec;

    // read
    vec = *(const vector_u *)src;

    // write
#if SSRJSON_IS_X64
#    if _CompileVectorBits == 512
    ucs2_encode_2bytes_utf8_avx512(dst, vec);
#    elif _CompileVectorBits == 256
    ucs2_encode_2bytes_utf8_avx2(dst, vec);
#    else
    ucs2_encode_2bytes_utf8_sse2(dst, vec);
#    endif
#elif SSRJSON_IS_AARCH64
    ucs2_encode_2bytes_utf8_neon(dst, vec);
#endif

    // check
    usize done_count;
    check_2bytes_in_ucs2_and_get_done_count(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += READ_BATCH_COUNT * 2;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count * 2;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline void check_3bytes_in_ucs2_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = broadcast(0x800);
    vector_a t2 = broadcast(0xd7ff);
    vector_a t3 = broadcast(0xe000);

#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    u32 m;

    m = unsigned_cmplt_bitmask(vec, t1) |
        (unsigned_cmpgt_bitmask(vec, t2) &
         unsigned_cmplt_bitmask(vec, t3));
#elif SSRJSON_IS_X64
    // see CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
    vector_a m;
    // use 2 signed_cmpgt to do unsigned range check
    m = unsigned_saturate_minus(t1, vec) | (signed_cmpgt(vec, t2) & signed_cmpgt(t3, vec));
#elif SSRJSON_IS_AARCH64
    vector_a m;
    m = (vec < t1) | ((vec > t2) & (vec < t3));
#endif
    bool checked = testz_escape_mask(m);
    *out_checked = checked;
    if (unlikely(!checked)) {
        // cannot use no eq0 version
        *out_done_count = escape_anymask_to_done_count(m);
    }
}

force_inline void check_3bytes_in_ucs4_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = broadcast(0x800);
    vector_a t2 = broadcast(0xd7ff);
    vector_a t3 = broadcast(0xe000);
    vector_a t4 = broadcast(0xffff);

#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    u32 m;
    m = unsigned_cmpgt_bitmask(t1, vec) | (unsigned_cmpgt_bitmask(vec, t2) & unsigned_cmpgt_bitmask(t3, vec)) | unsigned_cmpgt_bitmask(vec, t4);
#elif SSRJSON_IS_X64
    vector_a m;
    m = signed_cmpgt(t1, vec) | (signed_cmpgt(vec, t2) & signed_cmpgt(t3, vec)) | signed_cmpgt(vec, t4);
#elif SSRJSON_IS_AARCH64
    vector_a m;
    m = (vec < t1) | ((vec > t2) & (vec < t3)) | (vec > t4);
#endif
    bool checked = testz_escape_mask(m);
    *out_checked = checked;
    if (unlikely(!checked)) {
        *out_done_count = escape_anymask_to_done_count_no_eq0(m);
    }
}

force_inline ssrjson_nofail u8 *_3bytes_in_ucs2_encode_loop(u8 *dst, const u16 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u16 *src = *src_addr;
    usize len = *len_addr;

    vector_a vec;

    // read
    vec = *(const vector_u *)src;

    // write
#if SSRJSON_IS_X64
#    if USING_AVX512
    ucs2_encode_3bytes_utf8_avx512(dst, vec);
#    elif USING_AVX2
    ucs2_encode_3bytes_utf8_avx2(dst, vec);
#    elif __SSSE3__
    ucs2_encode_3bytes_utf8_ssse3(dst, vec);
#    else
    ssrjson_unreachable();
#    endif
#else
    ucs2_encode_3bytes_utf8_neon(dst, vec);
#endif

    // check
    usize done_count;
    check_3bytes_in_ucs2_and_get_done_count(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += READ_BATCH_COUNT * 3;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count * 3;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

/* Return false when src contains invalid character. */
force_inline u8 *bytes_write_ucs2(u8 *writer, const u16 *src, usize len, ssrjson_compiletime bool is_key) {
#define CAN_LOOP4 (len >= 4 * READ_BATCH_COUNT)
#define CAN_LOOP (len >= READ_BATCH_COUNT)
    while (CAN_LOOP) {
        u16 unicode;
        unicode = *src;
        if (unicode < 128) {
            // ascii range
            bool continuous;
            if (ssrjson_consteval(!is_key)) {
                while (CAN_LOOP4) {
                    writer = ascii_in_ucs2_encode_loop4(writer, &src, &len, &continuous);
                    if (unlikely(!continuous)) {
                        goto encode_one;
                    }
                }
                assert(!CAN_LOOP4);
            }
            while (CAN_LOOP) {
                writer = ascii_in_ucs2_encode_loop(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else if (unicode < 0x800) {
            bool continuous;
            while (CAN_LOOP) {
                writer = _2bytes_in_ucs2_encode_loop(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else {
#if _CompileVectorBits >= 256 || __SSSE3__
            bool continuous;
            while (CAN_LOOP) {
                writer = _3bytes_in_ucs2_encode_loop(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
#else
            goto do_encode_one;
#endif
        }
    encode_one:;
        unicode = *src;
    do_encode_one:;
        writer = encode_one_ucs2(writer, unicode);
        if (unlikely(!writer)) {
            return NULL;
        }
        src++;
        len--;
    }
    if (len) writer = bytes_write_ucs2_trailing(writer, src, len);
    return writer;
#undef CAN_LOOP
#undef CAN_LOOP4
}

force_inline u8 *bytes_write_ucs2_raw_utf8(u8 *writer, const u16 *src, usize len, ssrjson_compiletime bool is_key) {
#define CAN_LOOP4 (len >= 4 * READ_BATCH_COUNT)
#define CAN_LOOP (len >= READ_BATCH_COUNT)
    while (CAN_LOOP) {
        u16 unicode;
        unicode = *src;
        if (unicode < 128) {
            // ascii range
            bool continuous;
            if (ssrjson_consteval(!is_key)) {
                while (CAN_LOOP4) {
                    writer = ascii_in_ucs2_encode_loop4_raw_utf8(writer, &src, &len, &continuous);
                    if (unlikely(!continuous)) {
                        goto encode_one;
                    }
                }
                assert(!CAN_LOOP4);
            }
            while (CAN_LOOP) {
                writer = ascii_in_ucs2_encode_loop_raw_utf8(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else if (unicode < 0x800) {
            bool continuous;
            while (CAN_LOOP) {
                writer = _2bytes_in_ucs2_encode_loop(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else {
#if _CompileVectorBits >= 256 || __SSSE3__
            bool continuous;
            while (CAN_LOOP) {
                writer = _3bytes_in_ucs2_encode_loop(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
#else
            goto do_encode_one;
#endif
        }
    encode_one:;
        unicode = *src;
    do_encode_one:;
        writer = encode_one_ucs2_noescape(writer, unicode);
        if (unlikely(!writer)) {
            return NULL;
        }
        src++;
        len--;
    }
    if (len) writer = bytes_write_ucs2_raw_utf8_trailing(writer, src, len);
    return writer;
#undef CAN_LOOP
#undef CAN_LOOP4
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

/* UCS4 src. */
#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 1
#include "compile_context/srw_in.inl.h"

force_inline void check_ascii_in_ucs4_and_get_done_countx4(unionvector_a_x4 vec, bool *out_checked, usize *out_done_count) {
    vector_a *checker_masks = (vector_a *)&_CheckerMasks;

    vector_a t1 = checker_masks[0];
    vector_a t2 = checker_masks[1];
    vector_a t3 = checker_masks[2];
    vector_a t4 = broadcast(0x7f);
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    struct {
        u32 x[4];
    } m;

    u32 r;
    m.x[0] = cmpeq_bitmask(vec.x[0], t1) |
             cmpeq_bitmask(vec.x[0], t2) |
             signed_cmpgt_bitmask(t3, vec.x[0]) |
             signed_cmpgt_bitmask(vec.x[0], t4);
    m.x[1] = cmpeq_bitmask(vec.x[1], t1) |
             cmpeq_bitmask(vec.x[1], t2) |
             signed_cmpgt_bitmask(t3, vec.x[1]) |
             signed_cmpgt_bitmask(vec.x[1], t4);
    m.x[2] = cmpeq_bitmask(vec.x[2], t1) |
             cmpeq_bitmask(vec.x[2], t2) |
             signed_cmpgt_bitmask(t3, vec.x[2]) |
             signed_cmpgt_bitmask(vec.x[2], t4);
    m.x[3] = cmpeq_bitmask(vec.x[3], t1) |
             cmpeq_bitmask(vec.x[3], t2) |
             signed_cmpgt_bitmask(t3, vec.x[3]) |
             signed_cmpgt_bitmask(vec.x[3], t4);
#elif SSRJSON_IS_X64
    // see CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
    unionvector_a_x4 m;
    vector_a r;
    m.x[0] = (vec.x[0] == t1) | (vec.x[0] == t2) | signed_cmpgt(t3, vec.x[0]) | signed_cmpgt(vec.x[0], t4);
    m.x[1] = (vec.x[1] == t1) | (vec.x[1] == t2) | signed_cmpgt(t3, vec.x[1]) | signed_cmpgt(vec.x[1], t4);
    m.x[2] = (vec.x[2] == t1) | (vec.x[2] == t2) | signed_cmpgt(t3, vec.x[2]) | signed_cmpgt(vec.x[2], t4);
    m.x[3] = (vec.x[3] == t1) | (vec.x[3] == t2) | signed_cmpgt(t3, vec.x[3]) | signed_cmpgt(vec.x[3], t4);
#elif SSRJSON_IS_AARCH64
    unionvector_a_x4 m;
    vector_a r;
    m.x[0] = (vec.x[0] == t1) | (vec.x[0] == t2) | (vec.x[0] < t3) | (vec.x[0] > t4);
    m.x[1] = (vec.x[1] == t1) | (vec.x[1] == t2) | (vec.x[1] < t3) | (vec.x[1] > t4);
    m.x[2] = (vec.x[2] == t1) | (vec.x[2] == t2) | (vec.x[2] < t3) | (vec.x[2] > t4);
    m.x[3] = (vec.x[3] == t1) | (vec.x[3] == t2) | (vec.x[3] < t3) | (vec.x[3] > t4);
#endif

    r = m.x[0] | m.x[1];
    r = r | (m.x[2] | m.x[3]);
    //
    bool checked = testz_escape_mask(r);
    *out_checked = checked;
    if (unlikely(!checked)) {
        usize done_count = 0;
        for (int i = 0; i < 4; ++i) {
            if (testz_escape_mask(m.x[i])) {
                done_count += READ_BATCH_COUNT;
            } else {
                done_count += escape_anymask_to_done_count_no_eq0(m.x[i]);
                break;
            }
        }
        *out_done_count = done_count;
    }
}

force_inline void check_ascii_in_ucs4_raw_utf8_and_get_done_countx4(unionvector_a_x4 vec, bool *out_checked, usize *out_done_count) {
    vector_a t3 = broadcast(0);
    vector_a t4 = broadcast(0x7f);
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    struct {
        u32 x[4];
    } m;

    u32 r;
    m.x[0] = signed_cmpgt_bitmask(t3, vec.x[0]) |
             signed_cmpgt_bitmask(vec.x[0], t4);
    m.x[1] = signed_cmpgt_bitmask(t3, vec.x[1]) |
             signed_cmpgt_bitmask(vec.x[1], t4);
    m.x[2] = signed_cmpgt_bitmask(t3, vec.x[2]) |
             signed_cmpgt_bitmask(vec.x[2], t4);
    m.x[3] = signed_cmpgt_bitmask(t3, vec.x[3]) |
             signed_cmpgt_bitmask(vec.x[3], t4);
#elif SSRJSON_IS_X64
    // see CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
    unionvector_a_x4 m;
    vector_a r;
    m.x[0] = signed_cmpgt(t3, vec.x[0]) | signed_cmpgt(vec.x[0], t4);
    m.x[1] = signed_cmpgt(t3, vec.x[1]) | signed_cmpgt(vec.x[1], t4);
    m.x[2] = signed_cmpgt(t3, vec.x[2]) | signed_cmpgt(vec.x[2], t4);
    m.x[3] = signed_cmpgt(t3, vec.x[3]) | signed_cmpgt(vec.x[3], t4);
#elif SSRJSON_IS_AARCH64
    unionvector_a_x4 m;
    vector_a r;
    m.x[0] = (vec.x[0] < t3) | (vec.x[0] > t4);
    m.x[1] = (vec.x[1] < t3) | (vec.x[1] > t4);
    m.x[2] = (vec.x[2] < t3) | (vec.x[2] > t4);
    m.x[3] = (vec.x[3] < t3) | (vec.x[3] > t4);
#endif

    r = m.x[0] | m.x[1];
    r = r | (m.x[2] | m.x[3]);
    //
    bool checked = testz_escape_mask(r);
    *out_checked = checked;
    if (unlikely(!checked)) {
        usize done_count = 0;
        for (int i = 0; i < 4; ++i) {
            if (testz_escape_mask(m.x[i])) {
                done_count += READ_BATCH_COUNT;
            } else {
                done_count += escape_anymask_to_done_count_no_eq0(m.x[i]);
                break;
            }
        }
        *out_done_count = done_count;
    }
}

force_inline ssrjson_nofail u8 *ascii_in_ucs4_encode_loop4(u8 *dst, const u32 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u32 *src = *src_addr;
    usize len = *len_addr;

    unionvector_a_x4 vec;

    // read
    vec.x[0] = *(const vector_u *)(src + READ_BATCH_COUNT * 0);
    vec.x[1] = *(const vector_u *)(src + READ_BATCH_COUNT * 1);
    vec.x[2] = *(const vector_u *)(src + READ_BATCH_COUNT * 2);
    vec.x[3] = *(const vector_u *)(src + READ_BATCH_COUNT * 3);

    // write
    cvt_to_dst(dst + READ_BATCH_COUNT * 0, vec.x[0]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 1, vec.x[1]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 2, vec.x[2]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 3, vec.x[3]);

    // check
    usize done_count;
    check_ascii_in_ucs4_and_get_done_countx4(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += 4 * READ_BATCH_COUNT;
        src += 4 * READ_BATCH_COUNT;
        len -= 4 * READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline ssrjson_nofail u8 *ascii_in_ucs4_encode_loop4_raw_utf8(u8 *dst, const u32 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u32 *src = *src_addr;
    usize len = *len_addr;

    unionvector_a_x4 vec;

    // read
    vec.x[0] = *(const vector_u *)(src + READ_BATCH_COUNT * 0);
    vec.x[1] = *(const vector_u *)(src + READ_BATCH_COUNT * 1);
    vec.x[2] = *(const vector_u *)(src + READ_BATCH_COUNT * 2);
    vec.x[3] = *(const vector_u *)(src + READ_BATCH_COUNT * 3);

    // write
    cvt_to_dst(dst + READ_BATCH_COUNT * 0, vec.x[0]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 1, vec.x[1]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 2, vec.x[2]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 3, vec.x[3]);

    // check
    usize done_count;
    check_ascii_in_ucs4_raw_utf8_and_get_done_countx4(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += 4 * READ_BATCH_COUNT;
        src += 4 * READ_BATCH_COUNT;
        len -= 4 * READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline void check_ascii_in_ucs4_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a *checker_masks = (vector_a *)&_CheckerMasks;

    vector_a t1 = checker_masks[0];
    vector_a t2 = checker_masks[1];
    vector_a t3 = checker_masks[2];
    vector_a t4 = broadcast(0x7f);
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    u32 m;
    m = cmpeq_bitmask(vec, t1) |
        cmpeq_bitmask(vec, t2) |
        signed_cmpgt_bitmask(t3, vec) |
        signed_cmpgt_bitmask(vec, t4);
#elif SSRJSON_IS_X64
    vector_a m;
    m = (vec == t1) | (vec == t2) | signed_cmpgt(t3, vec) | signed_cmpgt(vec, t4);
#elif SSRJSON_IS_AARCH64
    vector_a m;
    m = (vec == t1) | (vec == t2) | (vec < t3) | (vec > t4);
#endif
    bool checked = testz_escape_mask(m);
    *out_checked = checked;
    if (unlikely(!checked)) {
        *out_done_count = escape_anymask_to_done_count_no_eq0(m);
    }
}

force_inline void check_ascii_in_ucs4_raw_utf8_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t3 = broadcast(0);
    vector_a t4 = broadcast(0x7f);
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    u32 m;
    m = signed_cmpgt_bitmask(t3, vec) |
        signed_cmpgt_bitmask(vec, t4);
#elif SSRJSON_IS_X64
    vector_a m;
    m = signed_cmpgt(t3, vec) | signed_cmpgt(vec, t4);
#elif SSRJSON_IS_AARCH64
    vector_a m;
    m = (vec < t3) | (vec > t4);
#endif
    bool checked = testz_escape_mask(m);
    *out_checked = checked;
    if (unlikely(!checked)) {
        *out_done_count = escape_anymask_to_done_count_no_eq0(m);
    }
}

force_inline ssrjson_nofail u8 *ascii_in_ucs4_encode_loop(u8 *dst, const u32 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u32 *src = *src_addr;
    usize len = *len_addr;

    vector_a vec;

    // read
    vec = *(const vector_u *)src;

    // write
    cvt_to_dst(dst, vec);

    // check
    usize done_count;
    check_ascii_in_ucs4_and_get_done_count(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += READ_BATCH_COUNT;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline ssrjson_nofail u8 *ascii_in_ucs4_encode_loop_raw_utf8(u8 *dst, const u32 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u32 *src = *src_addr;
    usize len = *len_addr;

    vector_a vec;

    // read
    vec = *(const vector_u *)src;

    // write
    cvt_to_dst(dst, vec);

    // check
    usize done_count;
    check_ascii_in_ucs4_raw_utf8_and_get_done_count(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += READ_BATCH_COUNT;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline void check_2bytes_in_ucs4_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = broadcast(0x80);
    vector_a t2 = broadcast(0x7ff);
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    u32 m;
    m = unsigned_cmpgt_bitmask(t1, vec) | unsigned_cmpgt_bitmask(vec, t2);
#elif SSRJSON_IS_X64
    vector_a m;
    m = signed_cmpgt(t1, vec) | signed_cmpgt(vec, t2);
#elif SSRJSON_IS_AARCH64
    vector_a m;
    m = (vec < t1) | (vec > t2);
#endif
    bool checked = testz_escape_mask(m);
    *out_checked = checked;
    if (unlikely(!checked)) {
        *out_done_count = escape_anymask_to_done_count_no_eq0(m);
    }
}

force_inline ssrjson_nofail u8 *_2bytes_in_ucs4_encode_loop(u8 *dst, const u32 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u32 *src = *src_addr;
    usize len = *len_addr;

    vector_a vec;

    // read
    vec = *(const vector_u *)src;

    // write
#if SSRJSON_IS_X64
#    if _CompileVectorBits == 512
    ucs4_encode_2bytes_utf8_avx512(dst, vec);
#    elif _CompileVectorBits == 256
    ucs4_encode_2bytes_utf8_avx2(dst, vec);
#    else
    ucs4_encode_2bytes_utf8_sse2(dst, vec);
#    endif
#elif SSRJSON_IS_AARCH64
    ucs4_encode_2bytes_utf8_neon(dst, vec);
#endif

    // check
    usize done_count;
    check_2bytes_in_ucs4_and_get_done_count(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += READ_BATCH_COUNT * 2;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count * 2;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

force_inline ssrjson_nofail u8 *_3bytes_in_ucs4_encode_loop(u8 *dst, const u32 **src_addr, usize *len_addr, bool *continuous_out) {
    // prepare
    const u32 *src = *src_addr;
    usize len = *len_addr;

    vector_a vec;

    // read
    vec = *(const vector_u *)src;

    // write
#if SSRJSON_IS_X64
#    if USING_AVX512
    ucs4_encode_3bytes_utf8_avx512(dst, vec);
#    elif USING_AVX2
    ucs4_encode_3bytes_utf8_avx2(dst, vec);
#    elif __SSSE3__
    ucs4_encode_3bytes_utf8_ssse3(dst, vec);
#    else
    ssrjson_unreachable();
#    endif
#elif SSRJSON_IS_AARCH64
    ucs4_encode_3bytes_utf8_neon(dst, vec);
#endif

    // check
    usize done_count;
    check_3bytes_in_ucs4_and_get_done_count(vec, continuous_out, &done_count);

    // update ptr
    if (likely(*continuous_out)) {
        dst += READ_BATCH_COUNT * 3;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count * 3;
        src += done_count;
        len -= done_count;
    }
    *src_addr = src;
    *len_addr = len;
    return dst;
}

/* Return false when src contains invalid character. */
force_inline u8 *bytes_write_ucs4(u8 *writer, const u32 *src, usize len, ssrjson_compiletime bool is_key) {
#define CAN_LOOP4 (len >= 4 * READ_BATCH_COUNT)
#define CAN_LOOP (len >= READ_BATCH_COUNT)
    while (CAN_LOOP) {
        u32 unicode;
        unicode = *src;
        if (unicode < 128) {
            // ascii range
            bool continuous;
            if (ssrjson_consteval(!is_key)) {
                while (CAN_LOOP4) {
                    writer = ascii_in_ucs4_encode_loop4(writer, &src, &len, &continuous);
                    if (unlikely(!continuous)) {
                        goto encode_one;
                    }
                }
                assert(!CAN_LOOP4);
            }
            while (CAN_LOOP) {
                writer = ascii_in_ucs4_encode_loop(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else if (unicode < 0x800) {
            bool continuous;
            while (CAN_LOOP) {
                writer = _2bytes_in_ucs4_encode_loop(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else if (unicode < 0x10000) {
#if _CompileVectorBits >= 256 || __SSSE3__
            bool continuous;
            while (CAN_LOOP) {
                writer = _3bytes_in_ucs4_encode_loop(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
#else
            goto do_encode_one;
#endif
        } else {
            goto do_encode_one;
        }
    encode_one:;
        unicode = *src;
    do_encode_one:;
        writer = encode_one_ucs4(writer, unicode);
        if (unlikely(!writer)) {
            return NULL;
        }
        src++;
        len--;
    }
    if (len) writer = bytes_write_ucs4_trailing(writer, src, len);
    return writer;
#undef CAN_LOOP
#undef CAN_LOOP4
}

force_inline u8 *bytes_write_ucs4_raw_utf8(u8 *writer, const u32 *src, usize len, ssrjson_compiletime bool is_key) {
#define CAN_LOOP4 (len >= 4 * READ_BATCH_COUNT)
#define CAN_LOOP (len >= READ_BATCH_COUNT)
    while (CAN_LOOP) {
        u32 unicode;
        unicode = *src;
        if (unicode < 128) {
            // ascii range
            bool continuous;
            if (ssrjson_consteval(!is_key)) {
                while (CAN_LOOP4) {
                    writer = ascii_in_ucs4_encode_loop4_raw_utf8(writer, &src, &len, &continuous);
                    if (unlikely(!continuous)) {
                        goto encode_one;
                    }
                }
                assert(!CAN_LOOP4);
            }
            while (CAN_LOOP) {
                writer = ascii_in_ucs4_encode_loop_raw_utf8(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else if (unicode < 0x800) {
            bool continuous;
            while (CAN_LOOP) {
                writer = _2bytes_in_ucs4_encode_loop(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else if (unicode < 0x10000) {
#if _CompileVectorBits >= 256 || __SSSE3__
            bool continuous;
            while (CAN_LOOP) {
                writer = _3bytes_in_ucs4_encode_loop(writer, &src, &len, &continuous);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
#else
            goto do_encode_one;
#endif
        } else {
            goto do_encode_one;
        }
    encode_one:;
        unicode = *src;
    do_encode_one:;
        writer = encode_one_ucs4_noescape(writer, unicode);
        if (unlikely(!writer)) {
            return NULL;
        }
        src++;
        len--;
    }
    if (len) writer = bytes_write_ucs4_raw_utf8_trailing(writer, src, len);
    return writer;
#undef CAN_LOOP
#undef CAN_LOOP4
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#undef _IMPL_INLINE_SPECIFIER_UCS1
#undef _IMPL_INLINE_SPECIFIER_UCS2
#undef _IMPL_INLINE_SPECIFIER_UCS4

#endif // SSRJSON_ENCODE_UTF8_H

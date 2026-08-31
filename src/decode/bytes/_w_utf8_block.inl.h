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

#ifdef SSRJSON_CLANGD_CHECKING
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        include "decode/bytes/utf8_shared.h"
#        include "decode/bytes/utf8_simd128.h"
#        include "decode/decode_shared.h"
#        define COMPILE_WRITE_UCS_LEVEL 1
#        include "simd/compile_feature_check.h"
#    endif
#endif

#include "compile_context/sw_in.inl.h"

/* bounds on the scalar fallback budget, in source bytes */
#define _MinScalarBudget 32
#define _MaxScalarBudget 256

#if COMPILE_WRITE_UCS_LEVEL == 1
#    define cvt_ascii_to_dst cvt_to_dst_u8_u8_128
/* a code point from a lead byte >= 0xc4 does not fit latin1 */
#    define _PromoteAbove 0xc3
#elif COMPILE_WRITE_UCS_LEVEL == 2
#    define cvt_ascii_to_dst cvt_to_dst_u8_u16_128
/* a code point from a lead byte >= 0xf0 does not fit ucs2 */
#    define _PromoteAbove 0xef
#else
#    define cvt_ascii_to_dst cvt_to_dst_u8_u32_128
#endif

/*
 * Scalar decode, stopping at `limit`. Used both to walk to a byte the vector
 * loop has to report, and to decode a block whose sequence lengths are mixed.
 * Reaching `limit` returns DECODE_LOOPSTATE_CONTINUE.
 */
force_inline int decode_bytes_block_scalar(dst_t **dst_addr, const u8 **src_addr, const u8 *src_end,
                                           bool *is_ascii_addr, const u8 *limit) {
    const u8 *src = *src_addr;
    dst_t *dst = *dst_addr;
    int ret;

    while (1) {
        u8 c;
        u32 uni, tmp;
        const u8 *run_start;
        /* one lookup covers '"', '\\', the control bytes and 0x80-0xff */
        while (src < limit && likely(!char_is_ascii_stop(*src))) { *dst++ = (dst_t)*src++; }
        if (src >= limit) {
            ret = DECODE_LOOPSTATE_CONTINUE;
            goto done;
        }
        c = *src;
        if (c < 0x80) {
            if (c == _Quote) {
                ret = DECODE_LOOPSTATE_END;
                goto done;
            }
            if (c == _Slash) {
                ret = DECODE_LOOPSTATE_ESCAPE;
                goto done;
            }
            if (src >= src_end) {
                PyErr_SetString(JSONDecodeError, "Unexpected end of string");
            } else {
                PyErr_SetString(JSONDecodeError, "Invalid control character in string");
            }
            ret = DECODE_LOOPSTATE_INVALID;
            goto done;
        }
        /*
         * A run of non-ASCII sequences, one length at a time. No `limit` test:
         * overshooting costs nothing and a run stops on its own at the NUL
         * that terminates the source buffer. Lengths are tried in the order
         * this destination width makes likely.
         */
        run_start = src;
        uni = byte_load_4(src);
#if COMPILE_WRITE_UCS_LEVEL == 1
        while (utf8_is_seq_2(uni)) {
            u16 u = to_b2_unicode(uni);
            if (unlikely(u >= 0x100)) break;
            *dst++ = (dst_t)u;
            src += 2;
            uni = byte_load_4(src);
        }
        if (src != run_start) {
            *is_ascii_addr = false;
            continue;
        }
#elif COMPILE_WRITE_UCS_LEVEL == 2
        while (utf8_is_seq_3(uni, tmp)) {
            *dst++ = (dst_t)to_b3_unicode(uni);
            src += 3;
            uni = byte_load_4(src);
        }
        /* the likeliest end of a run; a non-lead byte cannot start another */
        if (!(uni & 0x80)) continue;
        while (utf8_is_seq_2(uni)) {
            *dst++ = (dst_t)to_b2_unicode(uni);
            src += 2;
            uni = byte_load_4(src);
        }
        if (src != run_start) continue;
#else
        while (utf8_is_seq_4(uni, tmp)) {
            *dst++ = to_b4_unicode(uni);
            src += 4;
            uni = byte_load_4(src);
        }
        if (!(uni & 0x80)) continue;
        while (utf8_is_seq_3(uni, tmp)) {
            *dst++ = to_b3_unicode(uni);
            src += 3;
            uni = byte_load_4(src);
        }
        if (!(uni & 0x80)) continue;
        while (utf8_is_seq_2(uni)) {
            *dst++ = to_b2_unicode(uni);
            src += 2;
            uni = byte_load_4(src);
        }
        if (src != run_start) continue;
#endif
        /*
         * Nothing was consumed: the byte at `src` either starts a sequence too
         * wide for this state, or no valid sequence at all.
         */
#if COMPILE_WRITE_UCS_LEVEL != 4
        /* Too wide for this state. Stop on the lead byte; the caller widens
         * the buffer. Invalid sequences are reported here rather than after
         * widening, so the error position matches the input. */
#    if COMPILE_WRITE_UCS_LEVEL == 1
        if (likely(utf8_is_seq_2(uni) || utf8_is_seq_3(uni, tmp) || utf8_is_seq_4(uni, tmp)))
#    else
        if (likely(utf8_is_seq_4(uni, tmp)))
#    endif
        {
            ret = DECODE_LOOPSTATE_PROMOTE;
            goto done;
        }
#endif
        utf8_set_seq_error(src, src_end);
        ret = DECODE_LOOPSTATE_INVALID;
        goto done;
    }

done:;
    *src_addr = src;
    *dst_addr = dst;
    return ret;
}

/*
 * Decode UTF-8 source bytes into `dst_t` units until something happens that
 * the caller has to deal with.
 *
 * On entry `*src_addr` points at a lead byte inside a JSON string. Both the
 * source and the destination pointer are advanced past everything consumed and
 * produced. The return value is one of:
 *
 *   DECODE_LOOPSTATE_CONTINUE  progress was made, nothing else happened
 *   DECODE_LOOPSTATE_END       src points at the closing '"'
 *   DECODE_LOOPSTATE_ESCAPE    src points at a '\\'
 *   DECODE_LOOPSTATE_PROMOTE   src points at the lead byte of a sequence whose
 *                              code point does not fit dst_t; the sequence is
 *                              *not* consumed, the caller widens the buffer and
 *                              re-enters the wider variant of this function
 *   DECODE_LOOPSTATE_INVALID   a Python exception has been set
 *
 * The caller guarantees a whole SIMD register can always be loaded from
 * `*src_addr`: the source buffer has SSRJSON_MEMCPY_SIMD_SIZE bytes of slack
 * past `src_end` and `*src_end` is a NUL, which stops the scan. It also
 * guarantees 4 * SSRJSON_MEMCPY_SIMD_SIZE bytes of slack past the destination,
 * since whole registers are stored even when only some lanes hold real output.
 */
force_inline int decode_bytes_block(dst_t **dst_addr, const u8 **src_addr, const u8 *src_end, bool *is_ascii_addr) {
    const u8 *src = *src_addr;
    dst_t *dst = *dst_addr;
    /*
     * How far the scalar fallback may run in one go. Doubled per consecutive
     * mixed block and reset as soon as a shape matches, so text that never
     * matches one stops paying the block preamble.
     */
    usize budget = _MinScalarBudget;

    while (1) {
        vector_a_u8_128 in = *(const vector_u_u8_128 *)src;

        /* '"', '\\' or a control byte; the scalar path stops on it and reports */
        if (unlikely(utf8_has_stop_128(in))) break;

        if (utf8_all_ascii_128(in)) {
            cvt_ascii_to_dst(dst, in);
            dst += 16;
            src += 16;
            continue;
        }

#if COMPILE_WRITE_UCS_LEVEL != 4
        /* too wide for dst_t; the scalar path returns PROMOTE without consuming */
        if (unlikely(utf8_any_byte_above_128(in, _PromoteAbove))) break;
#endif
#if COMPILE_WRITE_UCS_LEVEL == 1
        *is_ascii_addr = false;
#endif

        /* src[16] is inside the source buffer's padding, see
         * utf8_end_of_cp_bitmask_128 */
        utf8_eocp_t eocp = utf8_end_of_cp_bitmask_128(in, src[16]);

        /*
         * Pick the shape of the block. The homogeneous shapes are keyed on the
         * mask alone, and each validates itself out of its own composed
         * result.
         */
        {
            if ((eocp & _UTF8_EOCP_ALL) == _UTF8_EOCP_SEQ2X8) {
                /* eight two byte sequences filling the register */
                const vector_a_u8_128 sh = _UTF8_SHUF_SWAP2;
                vector_a_u16_128 composed = utf8_compose_upto2_128(shuffle_128(in, sh));
                if (unlikely(utf8_seq2x8_has_error_128(in))) goto invalid_block;
#if COMPILE_WRITE_UCS_LEVEL == 1
                *(vector_u_u8_64 *)dst = cvt_u16_to_u8_128(composed);
#elif COMPILE_WRITE_UCS_LEVEL == 2
                *(vector_u_u16_128 *)dst = composed;
#else
                cvt_to_dst_u16_u32_128(dst, composed);
#endif
                dst += 8;
                src += 16;
                budget = _MinScalarBudget;
                continue;
            }
#if COMPILE_WRITE_UCS_LEVEL == 4
            if ((eocp & _UTF8_EOCP_ALL) == _UTF8_EOCP_SEQ4X4) {
                /* four four byte sequences filling the register */
                const vector_a_u8_128 sh = _UTF8_SHUF_REV4;
                vector_a_u32_128 composed = utf8_compose_4_128(shuffle_128(in, sh));
                if (unlikely(utf8_seq4x4_has_error_128(in, composed))) goto invalid_block;
                *(vector_u_u32_128 *)dst = composed;
                dst += 4;
                src += 16;
                budget = _MinScalarBudget;
                continue;
            }
#endif
#if COMPILE_WRITE_UCS_LEVEL != 1
            if ((eocp & _UTF8_EOCP_LOW12) == _UTF8_EOCP_SEQ3X4) {
                /* four three byte sequences in the low twelve bytes */
                const vector_a_u8_128 sh = _UTF8_SHUF_REV3;
                vector_a_u32_128 composed = utf8_compose_upto3_128(shuffle_128(in, sh));
                if (unlikely(utf8_seq3x4_has_error_128(in, composed))) goto invalid_block;
#    if COMPILE_WRITE_UCS_LEVEL == 2
                *(vector_u_u16_64 *)dst = cvt_u32_to_u16_128(composed);
#    else
                *(vector_u_u32_128 *)dst = composed;
#    endif
                dst += 4;
                src += 12;
                budget = _MinScalarBudget;
                continue;
            }
#endif
/*
 * The shuffle table path, x86 only. Handles a mixed run of one and two byte
 * sequences, which is what Cyrillic, Greek, Hebrew and Arabic text looks like.
 * Measured slower than the scalar fallback on NEON, so it is not built there.
 */
#if COMPILE_WRITE_UCS_LEVEL != 4 && SSRJSON_IS_X64
            /* the shuffle gathers six code points, worth its two dependent
             * loads only when they cover most of the register */
            if (__builtin_popcount(get_bitmask_from_u8_128(in)) >= 12 && !utf8_any_byte_above_128(in, 0xdf)) {
                /* lengths are not known in advance here, so the general
                 * validator is needed, and it has to run first: the index
                 * below only names a six code point shuffle for a valid block */
                u32 m12;
                u8 idx;
                vector_a_u16_128 composed;
                if (unlikely(utf8_block_has_error_128(in))) goto invalid_block;
                m12 = eocp & 0xfff;
                idx = _Utf8ToUcsIndex[m12][0];
                /* no byte is a three or four byte lead, so the first six code
                 * points always fit in twelve bytes */
                assert(idx < 64);
                composed = utf8_compose_upto2_128(shuffle_128(in, *(const vector_u_u8_128 *)_Utf8ToUcsShuffle[idx]));
                /* the guard above leaves only levels 1 and 2 here */
#    if COMPILE_WRITE_UCS_LEVEL == 1
                *(vector_u_u8_64 *)dst = cvt_u16_to_u8_128(composed);
#    else
                *(vector_u_u16_128 *)dst = composed;
#    endif
                dst += 6;
                src += _Utf8ToUcsIndex[m12][1];
                budget = _MinScalarBudget;
                continue;
            }
#endif
        }

        /*
         * Mixed lengths, and not dense enough for the shuffle table, so decode
         * scalar. The limit spreads this block's load, masks and validation
         * over two registers worth of input instead of over one code point.
         */
        {
            int ret = decode_bytes_block_scalar(&dst, &src, src_end, is_ascii_addr, src + budget);
            if (budget < _MaxScalarBudget) budget *= 2;
            if (ret != DECODE_LOOPSTATE_CONTINUE) {
                *src_addr = src;
                *dst_addr = dst;
                return ret;
            }
        }
    }

    *src_addr = src;
    *dst_addr = dst;
    return decode_bytes_block_scalar(dst_addr, src_addr, src_end, is_ascii_addr, src_end + 1);

invalid_block:;
    PyErr_SetString(JSONDecodeError, "Invalid UTF-8 encoding in string");
    *src_addr = src;
    *dst_addr = dst;
    return DECODE_LOOPSTATE_INVALID;
}

#undef _MinScalarBudget
#undef _MaxScalarBudget
#undef cvt_ascii_to_dst
#ifdef _PromoteAbove
#    undef _PromoteAbove
#endif

#include "compile_context/sw_out.inl.h"

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

/* Derived from simdutf (Apache-2.0/MIT). */
/* Uses simdutf's utf8_lookup4 algorithm. */

#ifndef SSRJSON_DECODE_BYTES_UTF8_SIMD128_H
#define SSRJSON_DECODE_BYTES_UTF8_SIMD128_H

#include "simd/simd_impl.h"
#include "ssrjson.h"

#if SSRJSON_IS_X64
extern const u8 _Utf8ToUcsShuffle[64][16];
extern const u8 _Utf8ToUcsIndex[4096][2];
#endif

#if SSRJSON_IS_X64
#    define u8v_as_u16v(_x_) ((vector_a_u16_128)(_x_))
#    define u8v_as_u32v(_x_) ((vector_a_u32_128)(_x_))
#    define u16v_as_u8v(_x_) ((vector_a_u8_128)(_x_))
#    define u32v_as_u8v(_x_) ((vector_a_u8_128)(_x_))
#else
#    define u8v_as_u16v(_x_) vreinterpretq_u16_u8(_x_)
#    define u8v_as_u32v(_x_) vreinterpretq_u32_u8(_x_)
#    define u16v_as_u8v(_x_) vreinterpretq_u8_u16(_x_)
#    define u32v_as_u8v(_x_) vreinterpretq_u8_u32(_x_)
#endif

#if SSRJSON_IS_X64 && __AVX512VL__ && __AVX512BW__
force_inline bool utf8_has_stop_128(vector_a_u8_128 x) {
    vector_a_u8_128 quote = cmpeq_u8_128(x, broadcast_u8_128(_Quote));
    vector_a_u8_128 slash = cmpeq_u8_128(x, broadcast_u8_128(_Slash));
    vector_a_u8_128 ctrl = cmpeq_u8_128(
            unsigned_saturate_minus_u8_128(x, broadcast_u8_128(_ControlMax - 1)), setzero_128());
    return get_bitmask_from_u8_128(quote | slash | ctrl) != 0;
}
#else
force_inline bool utf8_has_stop_128(vector_a_u8_128 x) {
    vector_a_u8_128 quote = cmpeq_u8_128(x, broadcast_u8_128(_Quote));
    vector_a_u8_128 slash = cmpeq_u8_128(x, broadcast_u8_128(_Slash));
    vector_a_u8_128 ctrl = unsigned_saturate_minus_u8_128(broadcast_u8_128(_ControlMax), x);
    return !testz_128(quote | slash | ctrl);
}
#endif

#if SSRJSON_IS_X64
force_inline bool utf8_all_ascii_128(vector_a_u8_128 x) { return get_bitmask_from_u8_128(x) == 0; }
#else
force_inline bool utf8_all_ascii_128(vector_a_u8_128 x) { return testz_128(x & broadcast_u8_128(0x80)); }
#endif

force_inline bool utf8_any_byte_above_128(vector_a_u8_128 x, u8 limit) {
    return !testz_128(unsigned_saturate_minus_u8_128(x, broadcast_u8_128(limit)));
}

/* Mark each code point's ending byte. */
/* x86 uses bits; NEON uses nibbles. */
#if SSRJSON_IS_X64
typedef u32 utf8_eocp_t;

#    define _UTF8_EOCP_ALL 0xffffu
#    define _UTF8_EOCP_LOW12 0xfffu
#    define _UTF8_EOCP_SEQ2X8 0xaaaau
#    define _UTF8_EOCP_SEQ4X4 0x8888u
#    define _UTF8_EOCP_SEQ3X4 0x924u

force_inline utf8_eocp_t utf8_end_of_cp_bitmask_128(vector_a_u8_128 x, u8 next) {
    vector_a_u8_128 is_cont = cmpeq_u8_128(x & broadcast_u8_128(0xc0), broadcast_u8_128(0x80));
    u32 cont = (u32)get_bitmask_from_u8_128(is_cont) | ((u32)(((next) & 0xc0) == 0x80) << 16);
    return ~(cont >> 1);
}
#else
typedef u64 utf8_eocp_t;

#    define _UTF8_EOCP_ALL 0xffffffffffffffffull
#    define _UTF8_EOCP_LOW12 0x0000ffffffffffffull
#    define _UTF8_EOCP_SEQ2X8 0xf0f0f0f0f0f0f0f0ull
#    define _UTF8_EOCP_SEQ4X4 0xf000f000f000f000ull
#    define _UTF8_EOCP_SEQ3X4 0x0000f00f00f00f00ull

force_inline utf8_eocp_t utf8_end_of_cp_bitmask_128(vector_a_u8_128 x, u8 next) {
    vector_a_u8_128 is_cont = cmpeq_u8_128(x & broadcast_u8_128(0xc0), broadcast_u8_128(0x80));
    /* Pack high nibbles into one lane. */
    u64 cont = vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(u8v_as_u16v(is_cont), 4)), 0);
    u64 next_cont = ((next & 0xc0) == 0x80) ? 0xf000000000000000ull : 0;
    return ~((cont >> 4) | next_cont);
}
#endif

force_inline vector_a_u8_128 _utf8_high_nibble_128(vector_a_u8_128 x) {
    return u16v_as_u8v(rshift_u16_128(u8v_as_u16v(x), 4)) & broadcast_u8_128(0x0f);
}

/* Validate boundary-aligned UTF-8 blocks. */
force_inline bool utf8_block_has_error_128(vector_a_u8_128 x) {
    const u8 TOO_SHORT = 1 << 0;
    const u8 TOO_LONG = 1 << 1;
    const u8 OVERLONG_3 = 1 << 2;
    const u8 TOO_LARGE = 1 << 3;
    const u8 SURROGATE = 1 << 4;
    const u8 OVERLONG_2 = 1 << 5;
    const u8 TOO_LARGE_1000 = 1 << 6;
    const u8 OVERLONG_4 = 1 << 6;
    const u8 TWO_CONTS = 1 << 7;
    const u8 CARRY = TOO_SHORT | TOO_LONG | TWO_CONTS;

    const vector_a_u8_128 t_prev_high = {/* 0_______ ASCII lead */
                                         TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
                                         /* 10______ continuation */
                                         TWO_CONTS, TWO_CONTS, TWO_CONTS, TWO_CONTS,
                                         /* 1100____ / 1101____ two byte lead */
                                         TOO_SHORT | OVERLONG_2, TOO_SHORT,
                                         /* 1110____ three byte lead */
                                         TOO_SHORT | OVERLONG_3 | SURROGATE,
                                         /* 1111____ four byte lead or worse */
                                         TOO_SHORT | TOO_LARGE | TOO_LARGE_1000 | OVERLONG_4};
    const vector_a_u8_128 t_prev_low = {
            CARRY | OVERLONG_3 | OVERLONG_2 | OVERLONG_4, CARRY | OVERLONG_2, CARRY, CARRY, CARRY | TOO_LARGE,
            CARRY | TOO_LARGE | TOO_LARGE_1000, CARRY | TOO_LARGE | TOO_LARGE_1000, CARRY | TOO_LARGE | TOO_LARGE_1000,
            CARRY | TOO_LARGE | TOO_LARGE_1000, CARRY | TOO_LARGE | TOO_LARGE_1000, CARRY | TOO_LARGE | TOO_LARGE_1000,
            CARRY | TOO_LARGE | TOO_LARGE_1000, CARRY | TOO_LARGE | TOO_LARGE_1000,
            /* ____1101, the surrogate range */
            CARRY | TOO_LARGE | TOO_LARGE_1000 | SURROGATE, CARRY | TOO_LARGE | TOO_LARGE_1000,
            CARRY | TOO_LARGE | TOO_LARGE_1000};
    const vector_a_u8_128 t_cur_high = {TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,
                                        TOO_SHORT,
                                        /* 1000____ */
                                        TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE_1000,
                                        /* 1001____ */
                                        TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE,
                                        /* 101_____ */
                                        TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE | TOO_LARGE,
                                        TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE | TOO_LARGE,
                                        /* 11______ */
                                        TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT};

    vector_a_u8_128 prev1 = byte_lshift_128(x, 1);
    vector_a_u8_128 prev2 = byte_lshift_128(x, 2);
    vector_a_u8_128 prev3 = byte_lshift_128(x, 3);

    vector_a_u8_128 sc = shuffle_128(t_prev_high, _utf8_high_nibble_128(prev1)) &
                         shuffle_128(t_prev_low, prev1 & broadcast_u8_128(0x0f)) &
                         shuffle_128(t_cur_high, _utf8_high_nibble_128(x));

    /* Prior leads require continuation bytes. */
    vector_a_u8_128 must23 = unsigned_saturate_minus_u8_128(prev2, broadcast_u8_128(0xe0 - 0x80)) |
                             unsigned_saturate_minus_u8_128(prev3, broadcast_u8_128(0xf0 - 0x80));
    return !testz_128((must23 & broadcast_u8_128(0x80)) ^ sc);
}

/* Validate homogeneous UTF-8 blocks. */
/* Lead checks reject masked invalid prefixes. */

/* Eight two-byte sequences. */
force_inline bool utf8_seq2x8_has_error_128(vector_a_u8_128 x) {
    /* Reject overlong two-byte forms. */
    const vector_a_u8_128 lo = {
            0xc2, 0x80, 0xc2, 0x80, 0xc2, 0x80, 0xc2, 0x80, 0xc2, 0x80, 0xc2, 0x80, 0xc2, 0x80, 0xc2, 0x80};
    const vector_a_u8_128 hi = {
            0xdf, 0xbf, 0xdf, 0xbf, 0xdf, 0xbf, 0xdf, 0xbf, 0xdf, 0xbf, 0xdf, 0xbf, 0xdf, 0xbf, 0xdf, 0xbf};
    return !testz_128(unsigned_saturate_minus_u8_128(lo, x) | unsigned_saturate_minus_u8_128(x, hi));
}

/* Four three-byte sequences. */
force_inline bool utf8_seq3x4_has_error_128(vector_a_u8_128 x, vector_a_u32_128 cp) {
    const vector_a_u8_128 lead_mask = {0xf0, 0, 0, 0xf0, 0, 0, 0xf0, 0, 0, 0xf0, 0, 0, 0, 0, 0, 0};
    const vector_a_u8_128 lead_patt = {0xe0, 0, 0, 0xe0, 0, 0, 0xe0, 0, 0, 0xe0, 0, 0, 0, 0, 0, 0};
    /* Reject overlong and surrogate code points. */
    vector_a_u32_128 overlong = cmpeq_u32_128(
            unsigned_max_u32_128(cp, broadcast_u32_128(0x7ff)), broadcast_u32_128(0x7ff));
    vector_a_u32_128 surrogate = cmpeq_u32_128(cp & broadcast_u32_128(0xf800), broadcast_u32_128(0xd800));
    return !testz_128(((x & lead_mask) ^ lead_patt) | u32v_as_u8v(overlong | surrogate));
}

/* Four four-byte sequences. */
force_inline bool utf8_seq4x4_has_error_128(vector_a_u8_128 x, vector_a_u32_128 cp) {
    const vector_a_u8_128 lead_mask = {0xf8, 0, 0, 0, 0xf8, 0, 0, 0, 0xf8, 0, 0, 0, 0xf8, 0, 0, 0};
    const vector_a_u8_128 lead_patt = {0xf0, 0, 0, 0, 0xf0, 0, 0, 0, 0xf0, 0, 0, 0, 0xf0, 0, 0, 0};
    /* Wrapped subtraction checks both range bounds. */
    vector_a_u32_128 d = cp - broadcast_u32_128(0x10000);
    vector_a_u32_128 range = unsigned_max_u32_128(d, broadcast_u32_128(0xfffff)) ^ broadcast_u32_128(0xfffff);
    return !testz_128(((x & lead_mask) ^ lead_patt) | u32v_as_u8v(range));
}

/* Gather bytes; 0xff selects zero. */
#define _UTF8_SHUF_SWAP2 {1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14}
#define _UTF8_SHUF_REV3 {2, 1, 0, 0xff, 5, 4, 3, 0xff, 8, 7, 6, 0xff, 11, 10, 9, 0xff}
#define _UTF8_SHUF_REV4 {3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12}

/* Compose gathered bytes into code points. */
force_inline vector_a_u16_128 utf8_compose_upto2_128(vector_a_u8_128 perm) {
    vector_a_u16_128 p = u8v_as_u16v(perm);
    return (p & broadcast_u16_128(0x007f)) | rshift_u16_128(p & broadcast_u16_128(0x1f00), 2);
}

force_inline vector_a_u32_128 utf8_compose_upto3_128(vector_a_u8_128 perm) {
    vector_a_u32_128 p = u8v_as_u32v(perm);
    return (p & broadcast_u32_128(0x0000007f)) | rshift_u32_128(p & broadcast_u32_128(0x00003f00), 2) |
           rshift_u32_128(p & broadcast_u32_128(0x000f0000), 4);
}

force_inline vector_a_u32_128 utf8_compose_4_128(vector_a_u8_128 perm) {
    vector_a_u32_128 p = u8v_as_u32v(perm);
    return (p & broadcast_u32_128(0x0000003f)) | rshift_u32_128(p & broadcast_u32_128(0x00003f00), 2) |
           rshift_u32_128(p & broadcast_u32_128(0x003f0000), 4) | rshift_u32_128(p & broadcast_u32_128(0x07000000), 6);
}

#endif // SSRJSON_DECODE_BYTES_UTF8_SIMD128_H

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

#ifndef DECODE_FLOAT_UTILS_H
#define DECODE_FLOAT_UTILS_H
#include "decode/decode_shared.h"

/** Maximum exponent of exact pow10 */
#define U64_POW10_MAX_EXP 19

/** Maximum numbers of chunks used by a bigint (58 is enough here). */
#define BIGINT_MAX_CHUNKS 64

/** Maximum exact pow10 exponent for double value. */
#define F64_POW10_EXP_MAX_EXACT 22

/** Cached pow10 table. */
static const f64 f64_pow10_table[] = {
        1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12,
        1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};

/** Table: [ 10^0, ..., 10^19 ] (generate with misc/make_tables.c) */
static const u64 u64_pow10_table[U64_POW10_MAX_EXP + 1] = {
        U64(0x00000000, 0x00000001), U64(0x00000000, 0x0000000A),
        U64(0x00000000, 0x00000064), U64(0x00000000, 0x000003E8),
        U64(0x00000000, 0x00002710), U64(0x00000000, 0x000186A0),
        U64(0x00000000, 0x000F4240), U64(0x00000000, 0x00989680),
        U64(0x00000000, 0x05F5E100), U64(0x00000000, 0x3B9ACA00),
        U64(0x00000002, 0x540BE400), U64(0x00000017, 0x4876E800),
        U64(0x000000E8, 0xD4A51000), U64(0x00000918, 0x4E72A000),
        U64(0x00005AF3, 0x107A4000), U64(0x00038D7E, 0xA4C68000),
        U64(0x002386F2, 0x6FC10000), U64(0x01634578, 0x5D8A0000),
        U64(0x0DE0B6B3, 0xA7640000), U64(0x8AC72304, 0x89E80000)};

/** Unsigned arbitrarily large integer */
typedef struct bigint {
    u32 used;                    /* used chunks count, should not be 0 */
    u64 bits[BIGINT_MAX_CHUNKS]; /* chunks */
} bigint;

/** "Do It Yourself Floating Point" struct. */
typedef struct diy_fp {
    u64 sig; /* significand */
    i32 exp; /* exponent, base 2 */
    i32 pad; /* padding, useless */
} diy_fp;

/*==============================================================================
 * BigInt For Floating Point Number Reader
 *
 * The bigint algorithm is used by floating-point number reader to get correctly
 * rounded result for numbers with lots of digits. This part of code is rarely
 * used for common numbers.
 *============================================================================*/
/**
 Evaluate 'big += val'.
 @param big A big number (can be 0).
 @param val An unsigned integer (can be 0).
 */
force_inline void bigint_add_u64(bigint *big, u64 val) {
    u32 idx, max;
    u64 num = big->bits[0];
    u64 add = num + val;
    big->bits[0] = add;
    if (likely((add >= num) || (add >= val))) return;
    for ((void)(idx = 1), max = big->used; idx < max; idx++) {
        if (likely(big->bits[idx] != U64_MAX)) {
            big->bits[idx] += 1;
            return;
        }
        big->bits[idx] = 0;
    }
    big->bits[big->used++] = 1;
}

/**
 Evaluate 'big *= val'.
 @param big A big number (can be 0).
 @param val An unsigned integer (cannot be 0).
 */
force_inline void bigint_mul_u64(bigint *big, u64 val) {
    u32 idx = 0, max = big->used;
    u64 hi, lo, carry = 0;
    for (; idx < max; idx++) {
        if (big->bits[idx]) break;
    }
    for (; idx < max; idx++) {
        u128_mul_add(big->bits[idx], val, carry, &hi, &lo);
        big->bits[idx] = lo;
        carry = hi;
    }
    if (carry) big->bits[big->used++] = carry;
}

/**
 Evaluate 'big *= 2^exp'.
 @param big A big number (can be 0).
 @param exp An exponent integer (can be 0).
 */
force_inline void bigint_mul_pow2(bigint *big, u32 exp) {
    u32 shft = exp % 64;
    u32 move = exp / 64;
    u32 idx = big->used;
    if (unlikely(shft == 0)) {
        for (; idx > 0; idx--) {
            big->bits[idx + move - 1] = big->bits[idx - 1];
        }
        big->used += move;
        while (move) big->bits[--move] = 0;
    } else {
        big->bits[idx] = 0;
        for (; idx > 0; idx--) {
            u64 num = big->bits[idx] << shft;
            num |= big->bits[idx - 1] >> (64 - shft);
            big->bits[idx + move] = num;
        }
        big->bits[move] = big->bits[0] << shft;
        big->used += move + (big->bits[big->used + move] > 0);
        while (move) big->bits[--move] = 0;
    }
}

/**
 Evaluate 'big *= 10^exp'.
 @param big A big number (can be 0).
 @param exp An exponent integer (cannot be 0).
 */
force_inline void bigint_mul_pow10(bigint *big, i32 exp) {
    for (; exp >= U64_POW10_MAX_EXP; exp -= U64_POW10_MAX_EXP) {
        bigint_mul_u64(big, u64_pow10_table[U64_POW10_MAX_EXP]);
    }
    if (exp) {
        bigint_mul_u64(big, u64_pow10_table[exp]);
    }
}

/**
 Compare two bigint.
 @return -1 if 'a < b', +1 if 'a > b', 0 if 'a == b'.
 */
force_inline i32 bigint_cmp(bigint *a, bigint *b) {
    u32 idx = a->used;
    if (a->used < b->used) return -1;
    if (a->used > b->used) return +1;
    while (idx-- > 0) {
        u64 av = a->bits[idx];
        u64 bv = b->bits[idx];
        if (av < bv) return -1;
        if (av > bv) return +1;
    }
    return 0;
}

/**
 Evaluate 'big = val'.
 @param big A big number (can be 0).
 @param val An unsigned integer (can be 0).
 */
force_inline void bigint_set_u64(bigint *big, u64 val) {
    big->used = 1;
    big->bits[0] = val;
}

/*==============================================================================
 * Diy Floating Point
 *============================================================================*/
/** Get cached rounded diy_fp with pow(10, e) The input value must in range
    [POW10_SIG_TABLE_MIN_EXP, POW10_SIG_TABLE_MAX_EXP]. */
force_inline diy_fp diy_fp_get_cached_pow10(i32 exp10) {
    diy_fp fp;
    u64 sig_ext;
    pow10_table_get_sig(exp10, &fp.sig, &sig_ext);
    pow10_table_get_exp(exp10, &fp.exp);
    fp.sig += (sig_ext >> 63);
    return fp;
}

/** Returns fp * fp2. */
force_inline diy_fp diy_fp_mul(diy_fp fp, diy_fp fp2) {
    u64 hi, lo;
    u128_mul(fp.sig, fp2.sig, &hi, &lo);
    fp.sig = hi + (lo >> 63);
    fp.exp += fp2.exp + 64;
    return fp;
}

/** Convert diy_fp to IEEE-754 raw value. */
force_inline u64 diy_fp_to_ieee_raw(diy_fp fp) {
    u64 sig = fp.sig;
    i32 exp = fp.exp;
    u32 lz_bits;
    if (unlikely(fp.sig == 0)) return 0;

    lz_bits = u64_lz_bits(sig);
    sig <<= lz_bits;
    sig >>= F64_BITS - F64_SIG_FULL_BITS;
    exp -= (i32)lz_bits;
    exp += F64_BITS - F64_SIG_FULL_BITS;
    exp += F64_SIG_BITS;

    if (unlikely(exp >= F64_MAX_BIN_EXP)) {
        /* overflow */
        return F64_RAW_INF;
    } else if (likely(exp >= F64_MIN_BIN_EXP - 1)) {
        /* normal */
        exp += F64_EXP_BIAS;
        return ((u64)exp << F64_SIG_BITS) | (sig & F64_SIG_MASK);
    } else if (likely(exp >= F64_MIN_BIN_EXP - F64_SIG_FULL_BITS)) {
        /* subnormal */
        return sig >> (F64_MIN_BIN_EXP - exp - 1);
    } else {
        /* underflow */
        return 0;
    }
}

#endif // DECODE_FLOAT_UTILS_H

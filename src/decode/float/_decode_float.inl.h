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

#ifdef SSRJSON_CLANGD_DUMMY
#    ifndef COMPILE_READ_UCS_LEVEL
#        include "bigint.h"
#        include "decode/float/decode_float_utils.h"
#        include "decode/str/tools.h"
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif

#include "compile_context/r_in.inl.h"

force_inline bool digi_is_digit(_src_t d) {
    return d <= U8MAX && _digi_is_digit((u8)d);
}

force_inline bool digi_is_digit_or_fp(_src_t d) {
    return d <= U8MAX && _digi_is_digit_or_fp((u8)d);
}

force_inline bool digi_is_exp(_src_t d) {
    return d <= U8MAX && _digi_is_exp((u8)d);
}

force_inline bool digi_is_sign(_src_t d) {
    return d <= U8MAX && _digi_is_sign((u8)d);
}

force_inline bool digi_is_fp(_src_t d) {
    return d <= U8MAX && _digi_is_fp((u8)d);
}

#define DIGI_IS_NONZERO MAKE_R_NAME(digi_is_nonzero)

////////////////
force_inline bool DIGI_IS_NONZERO(_src_t d) {
    return d <= U8MAX && _digi_is_nonzero((u8)d);
}

/*==============================================================================
 * JSON Number Reader (IEEE-754)
 *============================================================================*/
/**
 Read a JSON number.
 
 1. This function assume that the floating-point number is in IEEE-754 format.
 2. This function support uint64/int64/double number. If an integer number
    cannot fit in uint64/int64, it will returns as a double number. If a double
    number is infinite, the return value is based on flag.
 3. This function (with inline attribute) may generate a lot of instructions.
 */
internal_simd_noinline PyObject *read_number(const _src_t **ptr, const _src_t *buffer_end) {
#define return_err(_end, _msg)                                                  \
    do {                                                                        \
        PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, _end - hdr); \
        return NULL;                                                            \
    } while (0)

#define return_0()                     \
    do {                               \
        *end = cur;                    \
        return PyLong_FromLongLong(0); \
    } while (false)

#define return_i64(_v)                                                                                \
    do {                                                                                              \
        *end = cur;                                                                                   \
        u64 temp = (sign ? (u64)(~(_v) + 1) : (u64)(_v));                                             \
        if (unlikely(SSRJSON_CAST(i64, temp) < 0 && !sign)) return PyLong_FromUnsignedLongLong(temp); \
        return PyLong_FromLongLong((i64)temp);                                                        \
    } while (false)

#define return_u64(_v)                                                                  \
    do {                                                                                \
        *end = cur;                                                                     \
        return PyLong_FromUnsignedLongLong((u64)(sign ? (u64)(~(_v) + 1) : (u64)(_v))); \
    } while (false)

#define return_f64(_v)                                            \
    do {                                                          \
        *end = cur;                                               \
        return PyFloat_FromDouble(sign ? -(f64)(_v) : (f64)(_v)); \
    } while (false)

#define return_f64_bin(_v)                           \
    do {                                             \
        *end = cur;                                  \
        u64 temp = ((u64)sign << 63) | (u64)(_v);    \
        return PyFloat_FromDouble(*(double *)&temp); \
    } while (false)

#define return_inf()                 \
    do {                             \
        return_f64_bin(F64_RAW_INF); \
    } while (false)

    const _src_t *sig_cut = NULL; /* significant part cutting position for long number */
    const _src_t *sig_end = NULL; /* significant part ending position */
    const _src_t *dot_pos = NULL; /* decimal point position */

    u64 sig = 0; /* significant part of the number */
    i32 exp = 0; /* exponent part of the number */

    bool exp_sign;     /* temporary exponent sign from literal part */
    i64 exp_sig = 0;   /* temporary exponent number from significant part */
    i64 exp_lit = 0;   /* temporary exponent number from exponent literal part */
    u64 num;           /* temporary number for reading */
    const _src_t *tmp; /* temporary cursor for reading */

    const _src_t *hdr = *ptr;
    const _src_t *cur = *ptr;
    const _src_t **end = ptr;
    bool sign;

    sign = (*hdr == '-');
    cur += sign;

    /* begin with a leading zero or non-digit */
    if (unlikely(!DIGI_IS_NONZERO(*cur))) { /* 0 or non-digit char */
        if (unlikely(*cur != '0')) {        /* non-digit char */
            PyObject *number_obj = read_inf_or_nan(sign, &cur, buffer_end);
            if (likely(number_obj)) {
                *end = cur;
                return number_obj;
            }
            if (unlikely(!PyErr_Occurred())) {
                return_err(cur, "no digit after minus sign");
            }
            return NULL;
        }
        /* begin with 0 */
        if (likely(!digi_is_digit_or_fp(*++cur))) return_0();
        if (likely(*cur == '.')) {
            dot_pos = cur++;
            if (unlikely(!digi_is_digit(*cur))) {
                return_err(cur, "no digit after decimal point");
            }
            while (unlikely(*cur == '0')) cur++;
            if (likely(digi_is_digit(*cur))) {
                /* first non-zero digit after decimal point */
                sig = (u64)(*cur - '0'); /* read first digit */
                cur--;
                goto digi_frac_1; /* continue read fraction part */
            }
        }
        if (unlikely(digi_is_digit(*cur))) {
            return_err(cur - 1, "number with leading zero is not allowed");
        }
        if (unlikely(digi_is_exp(*cur))) { /* 0 with any exponent is still 0 */
            cur += (usize)1 + digi_is_sign(cur[1]);
            if (unlikely(!digi_is_digit(*cur))) {
                return_err(cur, "no digit after exponent sign");
            }
            while (digi_is_digit(*++cur));
        }
        return_f64_bin(0);
    }

    /* begin with non-zero digit */
    sig = (u64)(*cur - '0');

    /*
     Read integral part, same as the following code.
     
         for (int i = 1; i <= 18; i++) {
            num = cur[i] - '0';
            if (num <= 9) sig = num + sig * 10;
            else goto digi_sepr_i;
         }
     */
#define expr_intg(i)                                                        \
    if (likely((num = (u64)(cur[i] - (u8)'0')) <= 9)) sig = num + sig * 10; \
    else { goto digi_sepr_##i; }
    REPEAT_INCR_IN_1_18(expr_intg)
#undef expr_intg


    cur += 19; /* skip continuous 19 digits */
    if (!digi_is_digit_or_fp(*cur)) {
        /* this number is an integer consisting of 19 digits */
        if (sign && (sig > ((u64)1 << 63))) { /* overflow */
            return_f64(normalized_u64_to_f64(sig));
        }
        return_i64(sig);
    }
    goto digi_intg_more; /* read more digits in integral part */


    /* process first non-digit character */
#define expr_sepr(i)                                   \
    digi_sepr_##i : if (likely(!digi_is_fp(cur[i]))) { \
        cur += i;                                      \
        return_i64(sig);                               \
    }                                                  \
    dot_pos = cur + i;                                 \
    if (likely(cur[i] == '.')) goto digi_frac_##i;     \
    cur += i;                                          \
    sig_end = cur;                                     \
    goto digi_exp_more;
    REPEAT_INCR_IN_1_18(expr_sepr)
#undef expr_sepr


    /* read fraction part */
#define expr_frac(i)                                                      \
    digi_frac_##i : if (likely((num = (u64)(cur[i + 1] - (u8)'0')) <= 9)) \
                            sig = num + sig * 10;                         \
    else { goto digi_stop_##i; }
    REPEAT_INCR_IN_1_18(expr_frac)
#undef expr_frac

    cur += 20;                                    /* skip 19 digits and 1 decimal point */
    if (!digi_is_digit(*cur)) goto digi_frac_end; /* fraction part end */
    goto digi_frac_more;                          /* read more digits in fraction part */


    /* significant part end */
#define expr_stop(i)              \
    digi_stop_##i : cur += i + 1; \
    goto digi_frac_end;
    REPEAT_INCR_IN_1_18(expr_stop)
#undef expr_stop


    /* read more digits in integral part */
digi_intg_more:
    if (digi_is_digit(*cur)) {
        if (!digi_is_digit_or_fp(cur[1])) {
            /* this number is an integer consisting of 20 digits */
            num = (u64)(*cur - '0');
            if ((sig < (U64_MAX / 10)) ||
                (sig == (U64_MAX / 10) && num <= (U64_MAX % 10))) {
                sig = num + sig * 10;
                cur++;
                /* convert to double if overflow */
                if (sign) {
                    return_f64(normalized_u64_to_f64(sig));
                }
                return_u64(sig);
            }
        }
    }

    if (digi_is_exp(*cur)) {
        dot_pos = cur;
        goto digi_exp_more;
    }

    if (*cur == '.') {
        dot_pos = cur++;
        if (!digi_is_digit(*cur)) {
            return_err(cur, "no digit after decimal point");
        }
    }


    /* read more digits in fraction part */
digi_frac_more:
    sig_cut = cur;        /* too large to fit in u64, excess digits need to be cut */
    sig += (*cur >= '5'); /* round */
    while (digi_is_digit(*++cur));
    if (!dot_pos) {
        dot_pos = cur;
        if (*cur == '.') {
            if (!digi_is_digit(*++cur)) {
                return_err(cur, "no digit after decimal point");
            }
            while (digi_is_digit(*cur)) cur++;
        }
    }
    exp_sig = (i64)(dot_pos - sig_cut);
    exp_sig += (dot_pos < sig_cut);

    /* ignore trailing zeros */
    tmp = cur - 1;
    while (*tmp == '0' || *tmp == '.') tmp--;
    if (tmp < sig_cut) {
        sig_cut = NULL;
    } else {
        sig_end = cur;
    }

    if (digi_is_exp(*cur)) goto digi_exp_more;
    goto digi_exp_finish;


    /* fraction part end */
digi_frac_end:
    if (unlikely(dot_pos + 1 == cur)) {
        return_err(cur, "no digit after decimal point");
    }
    sig_end = cur;
    exp_sig = -(i64)((u64)(cur - dot_pos) - 1);
    if (likely(!digi_is_exp(*cur))) {
        if (unlikely(exp_sig < F64_MIN_DEC_EXP - 19)) {
            return_f64_bin(0); /* underflow */
        }
        exp = (i32)exp_sig;
        goto digi_finish;
    } else {
        goto digi_exp_more;
    }


    /* read exponent part */
digi_exp_more:
    exp_sign = (*++cur == '-');
    cur += digi_is_sign(*cur);
    if (unlikely(!digi_is_digit(*cur))) {
        return_err(cur, "no digit after exponent sign");
    }
    while (*cur == '0') cur++;

    /* read exponent literal */
    tmp = cur;
    while (digi_is_digit(*cur)) {
        exp_lit = (i64)((u64)(*cur++ - '0') + (u64)exp_lit * 10);
    }
    if (unlikely(cur - tmp >= U64_SAFE_DIG)) {
        if (exp_sign) {
            return_f64_bin(0); /* underflow */
        } else {
            return_inf(); /* overflow */
        }
    }
    exp_sig += exp_sign ? -exp_lit : exp_lit;


    /* validate exponent value */
digi_exp_finish:
    if (unlikely(exp_sig < F64_MIN_DEC_EXP - 19)) {
        return_f64_bin(0); /* underflow */
    }
    if (unlikely(exp_sig > F64_MAX_DEC_EXP)) {
        return_inf(); /* overflow */
    }
    exp = (i32)exp_sig;


    /* all digit read finished */
digi_finish:

    /*
     Fast path 1:
     
     1. The floating-point number calculation should be accurate, see the
        comments of macro `SSRJSON_DOUBLE_MATH_CORRECT`.
     2. Correct rounding should be performed (fegetround() == FE_TONEAREST).
     3. The input of floating point number calculation does not lose precision,
        which means: 64 - leading_zero(input) - trailing_zero(input) < 53.
    
     We don't check all available inputs here, because that would make the code
     more complicated, and not friendly to branch predictor.
     */
#if SSRJSON_DOUBLE_MATH_CORRECT
    if (sig < ((u64)1 << 53) &&
        exp >= -F64_POW10_EXP_MAX_EXACT &&
        exp <= +F64_POW10_EXP_MAX_EXACT) {
        f64 dbl = (f64)sig;
        if (exp < 0) {
            dbl /= f64_pow10_table[-exp];
        } else {
            dbl *= f64_pow10_table[+exp];
        }
        return_f64(dbl);
    }
#endif

    /*
     Fast path 2:
     
     To keep it simple, we only accept normal number here,
     let the slow path to handle subnormal and infinity number.
     */
    if (likely(!sig_cut &&
               exp > -F64_MAX_DEC_EXP + 1 &&
               exp < +F64_MAX_DEC_EXP - 20)) {
        /*
         The result value is exactly equal to (sig * 10^exp),
         the exponent part (10^exp) can be converted to (sig2 * 2^exp2).
         
         The sig2 can be an infinite length number, only the highest 128 bits
         is cached in the pow10_sig_table.
         
         Now we have these bits:
         sig1 (normalized 64bit)        : aaaaaaaa
         sig2 (higher 64bit)            : bbbbbbbb
         sig2_ext (lower 64bit)         : cccccccc
         sig2_cut (extra unknown bits)  : dddddddddddd....
         
         And the calculation process is:
         ----------------------------------------
                 aaaaaaaa *
                 bbbbbbbbccccccccdddddddddddd....
         ----------------------------------------
         abababababababab +
                 acacacacacacacac +
                         adadadadadadadadadad....
         ----------------------------------------
         [hi____][lo____] +
                 [hi2___][lo2___] +
                         [unknown___________....]
         ----------------------------------------
         
         The addition with carry may affect higher bits, but if there is a 0
         in higher bits, the bits higher than 0 will not be affected.
         
         `lo2` + `unknown` may get a carry bit and may affect `hi2`, the max
         value of `hi2` is 0xFFFFFFFFFFFFFFFE, so `hi2` will not overflow.
         
         `lo` + `hi2` may also get a carry bit and may affect `hi`, but only
         the highest significant 53 bits of `hi` is needed. If there is a 0
         in the lower bits of `hi`, then all the following bits can be dropped.
         
         To convert the result to IEEE-754 double number, we need to perform
         correct rounding:
         1. if bit 54 is 0, round down,
         2. if bit 54 is 1 and any bit beyond bit 54 is 1, round up,
         3. if bit 54 is 1 and all bits beyond bit 54 are 0, round to even,
            as the extra bits is unknown, this case will not be handled here.
         */

        u64 raw;
        u64 sig1, sig2, sig2_ext, hi, lo, hi2, lo2, add, bits;
        i32 exp2;
        u32 lz;
        bool exact = false, carry, round_up;

        /* convert (10^exp) to (sig2 * 2^exp2) */
        pow10_table_get_sig(exp, &sig2, &sig2_ext);
        pow10_table_get_exp(exp, &exp2);

        /* normalize and multiply */
        lz = u64_lz_bits(sig);
        sig1 = sig << lz;
        exp2 -= (i32)lz;
        u128_mul(sig1, sig2, &hi, &lo);

        /*
         The `hi` is in range [0x4000000000000000, 0xFFFFFFFFFFFFFFFE],
         To get normalized value, `hi` should be shifted to the left by 0 or 1.
         
         The highest significant 53 bits is used by IEEE-754 double number,
         and the bit 54 is used to detect rounding direction.
         
         The lowest (64 - 54 - 1) bits is used to check whether it contains 0.
         */
        bits = hi & (((u64)1 << (64 - 54 - 1)) - 1);
        if (bits - 1 < (((u64)1 << (64 - 54 - 1)) - 2)) {
            /*
             (bits != 0 && bits != 0x1FF) => (bits - 1 < 0x1FF - 1)
             The `bits` is not zero, so we don't need to check `round to even`
             case. The `bits` contains bit `0`, so we can drop the extra bits
             after `0`.
             */
            exact = true;

        } else {
            /*
             (bits == 0 || bits == 0x1FF)
             The `bits` is filled with all `0` or all `1`, so we need to check
             lower bits with another 64-bit multiplication.
             */
            u128_mul(sig1, sig2_ext, &hi2, &lo2);

            add = lo + hi2;
            if (add + 1 > (u64)1) {
                /*
                 (add != 0 && add != U64_MAX) => (add + 1 > 1)
                 The `add` is not zero, so we don't need to check `round to
                 even` case. The `add` contains bit `0`, so we can drop the
                 extra bits after `0`. The `hi` cannot be U64_MAX, so it will
                 not overflow.
                 */
                carry = add < lo || add < hi2;
                hi += carry;
                exact = true;
            }
        }

        if (exact) {
            /* normalize */
            lz = hi < ((u64)1 << 63);
            hi <<= lz;
            exp2 -= (i32)lz;
            exp2 += 64;

            /* test the bit 54 and get rounding direction */
            round_up = (hi & ((u64)1 << (64 - 54))) > (u64)0;
            hi += (round_up ? ((u64)1 << (64 - 54)) : (u64)0);

            /* test overflow */
            if (hi < ((u64)1 << (64 - 54))) {
                hi = ((u64)1 << 63);
                exp2 += 1;
            }

            /* This is a normal number, convert it to IEEE-754 format. */
            hi >>= F64_BITS - F64_SIG_FULL_BITS;
            exp2 += F64_BITS - F64_SIG_FULL_BITS + F64_SIG_BITS;
            exp2 += F64_EXP_BIAS;
            raw = ((u64)exp2 << F64_SIG_BITS) | (hi & F64_SIG_MASK);
            return_f64_bin(raw);
        }
    }

    /*
     Slow path: read double number exactly with diyfp.
     1. Use cached diyfp to get an approximation value.
     2. Use bigcomp to check the approximation value if needed.
     
     This algorithm refers to google's double-conversion project:
     https://github.com/google/double-conversion
     */
    {
        const i32 ERR_ULP_LOG = 3;
        const i32 ERR_ULP = 1 << ERR_ULP_LOG;
        const i32 ERR_CACHED_POW = ERR_ULP / 2;
        const i32 ERR_MUL_FIXED = ERR_ULP / 2;
        const i32 DIY_SIG_BITS = 64;
        const i32 EXP_BIAS = F64_EXP_BIAS + F64_SIG_BITS;
        const i32 EXP_SUBNORMAL = -EXP_BIAS + 1;

        u64 fp_err;
        u32 bits;
        i32 order_of_magnitude;
        i32 effective_significand_size;
        i32 precision_digits_count;
        u64 precision_bits;
        u64 half_way;

        u64 raw;
        diy_fp fp, fp_upper;
        bigint big_full, big_comp;
        i32 cmp;

        fp.sig = sig;
        fp.exp = 0;
        fp_err = sig_cut ? (u64)(ERR_ULP / 2) : (u64)0;

        /* normalize */
        bits = u64_lz_bits(fp.sig);
        fp.sig <<= bits;
        fp.exp -= (i32)bits;
        fp_err <<= bits;

        /* multiply and add error */
        fp = diy_fp_mul(fp, diy_fp_get_cached_pow10(exp));
        fp_err += (u64)ERR_CACHED_POW + (fp_err != 0) + (u64)ERR_MUL_FIXED;

        /* normalize */
        bits = u64_lz_bits(fp.sig);
        fp.sig <<= bits;
        fp.exp -= (i32)bits;
        fp_err <<= bits;

        /* effective significand */
        order_of_magnitude = DIY_SIG_BITS + fp.exp;
        if (likely(order_of_magnitude >= EXP_SUBNORMAL + F64_SIG_FULL_BITS)) {
            effective_significand_size = F64_SIG_FULL_BITS;
        } else if (order_of_magnitude <= EXP_SUBNORMAL) {
            effective_significand_size = 0;
        } else {
            effective_significand_size = order_of_magnitude - EXP_SUBNORMAL;
        }

        /* precision digits count */
        precision_digits_count = DIY_SIG_BITS - effective_significand_size;
        if (unlikely(precision_digits_count + ERR_ULP_LOG >= DIY_SIG_BITS)) {
            i32 shr = (precision_digits_count + ERR_ULP_LOG) - DIY_SIG_BITS + 1;
            fp.sig >>= shr;
            fp.exp += shr;
            fp_err = (fp_err >> shr) + 1 + (u32)ERR_ULP;
            precision_digits_count -= shr;
        }

        /* half way */
        precision_bits = fp.sig & (((u64)1 << precision_digits_count) - 1);
        precision_bits *= (u32)ERR_ULP;
        half_way = (u64)1 << (precision_digits_count - 1);
        half_way *= (u32)ERR_ULP;

        /* rounding */
        fp.sig >>= precision_digits_count;
        fp.sig += (precision_bits >= half_way + fp_err);
        fp.exp += precision_digits_count;

        /* get IEEE double raw value */
        raw = diy_fp_to_ieee_raw(fp);
        if (unlikely(raw == F64_RAW_INF)) return_inf();
        if (likely(precision_bits <= half_way - fp_err ||
                   precision_bits >= half_way + fp_err)) {
            return_f64_bin(raw); /* number is accurate */
        }
        /* now the number is the correct value, or the next lower value */

        /* upper boundary */
        if (raw & F64_EXP_MASK) {
            fp_upper.sig = (raw & F64_SIG_MASK) + ((u64)1 << F64_SIG_BITS);
            fp_upper.exp = (i32)((raw & F64_EXP_MASK) >> F64_SIG_BITS);
        } else {
            fp_upper.sig = (raw & F64_SIG_MASK);
            fp_upper.exp = 1;
        }
        fp_upper.exp -= F64_EXP_BIAS + F64_SIG_BITS;
        fp_upper.sig <<= 1;
        fp_upper.exp -= 1;
        fp_upper.sig += 1; /* add half ulp */

        /* compare with bigint */
        bigint_set_buf_noinline(&big_full, sig, &exp, sig_cut, sig_end, dot_pos);
        bigint_set_u64(&big_comp, fp_upper.sig);
        if (exp >= 0) {
            bigint_mul_pow10(&big_full, +exp);
        } else {
            bigint_mul_pow10(&big_comp, -exp);
        }
        if (fp_upper.exp > 0) {
            bigint_mul_pow2(&big_comp, (u32) + fp_upper.exp);
        } else {
            bigint_mul_pow2(&big_full, (u32)-fp_upper.exp);
        }
        cmp = bigint_cmp(&big_full, &big_comp);
        if (likely(cmp != 0)) {
            /* round down or round up */
            raw += (cmp > 0);
        } else {
            /* falls midway, round to even */
            raw += (raw & 1);
        }

        if (unlikely(raw == F64_RAW_INF)) return_inf();
        return_f64_bin(raw);
    }

#undef return_err
#undef return_inf
#undef return_0
#undef return_i64
#undef return_u64
#undef return_f64
#undef return_f64_bin
#undef return_raw
}

#undef DIGI_IS_NONZERO


#undef digi_is_fp
#undef digi_is_sign
#undef digi_is_exp
#undef digi_is_digit_or_fp
#undef digi_is_digit
#undef read_number
//
#include "compile_context/r_out.inl.h"

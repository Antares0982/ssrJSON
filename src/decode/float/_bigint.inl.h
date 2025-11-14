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

#include "decode_float_utils.h"
//
#include "compile_context/r_in.inl.h"

/** Set a bigint with floating point number string. */
force_inline void bigint_set_buf(
        bigint *big, u64 sig, i32 *exp,
        const _src_t *sig_cut, const _src_t *sig_end, const _src_t *dot_pos) {

    if (unlikely(!sig_cut)) {
        /* no digit cut, set significant part only */
        bigint_set_u64(big, sig);
        return;

    } else {
        /* some digits were cut, read them from 'sig_cut' to 'sig_end' */
        const _src_t *hdr = sig_cut;
        const _src_t *cur = hdr;
        u32 len = 0;
        u64 val = 0;
        bool dig_big_cut = false;
        bool has_dot = (hdr < dot_pos) & (dot_pos < sig_end);
        u32 dig_len_total = U64_SAFE_DIG + (u32)(sig_end - hdr) - has_dot;

        sig -= (*sig_cut >= '5'); /* sig was rounded before */
        if (dig_len_total > F64_MAX_DEC_DIG) {
            dig_big_cut = true;
            sig_end -= dig_len_total - (F64_MAX_DEC_DIG + 1);
            sig_end -= (dot_pos + 1 == sig_end);
            dig_len_total = (F64_MAX_DEC_DIG + 1);
        }
        *exp -= (i32)dig_len_total - U64_SAFE_DIG;

        big->used = 1;
        big->bits[0] = sig;
        while (cur < sig_end) {
            if (likely(cur != dot_pos)) {
                val = val * 10 + (u64)(*cur++ - '0');
                len++;
                if (unlikely(cur == sig_end && dig_big_cut)) {
                    /* The last digit must be non-zero,    */
                    /* set it to '1' for correct rounding. */
                    val = val - (val % 10) + 1;
                }
                if (len == U64_SAFE_DIG || cur == sig_end) {
                    bigint_mul_pow10(big, (i32)len);
                    bigint_add_u64(big, val);
                    val = 0;
                    len = 0;
                }
            } else {
                cur++;
            }
        }
    }
}

void bigint_set_buf_noinline(
        bigint *big, u64 sig, i32 *exp,
        const _src_t *sig_cut, const _src_t *sig_end, const _src_t *dot_pos)
#if BIGINT_IMPL
{
    bigint_set_buf(big, sig, exp, sig_cut, sig_end, dot_pos);
}
#else
        ;
#endif // BIGINT_IMPL

#include "compile_context/r_out.inl.h"

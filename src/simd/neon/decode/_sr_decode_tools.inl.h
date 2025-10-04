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
#    include "simd/neon/checker.h"
#    include "simd/neon/common.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef SSRJSON_SIMD_NEON_DECODE_H
#        include "_r_decode_tools.inl.h"
#    endif
#endif

#define COMPILE_SIMD_BITS 128
#include "compile_context/sr_in.inl.h"

force_inline void fast_skip_spaces(const _src_t **cur_addr, const _src_t *end) {
    const vector_a template = broadcast(' ');
    const _src_t *cur = *cur_addr;
    assert(*cur == ' ');
    const _src_t *final_batch = end - READ_BATCH_COUNT;
loop:;
    if (likely(cur < final_batch)) {
        vector_a vec = *(const vector_u *)cur;
        vector_a mask = (vec == template) == setzero();
        if (testz(mask)) {
            cur += READ_BATCH_COUNT;
            goto loop;
        } else {
            u16 done_count = escape_mask_to_done_count(mask);
            cur += done_count;
        }
    } else {
        static _src_t _t[2] = {' ', ' '};
        while (true) REPEAT_CALL_16({
            if (cmpeq_2chars(cur, _t, end)) cur += 2;
            else
                break;
        })
        if (*cur == ' ') cur++;
    }
    *cur_addr = cur;
    assert(*cur != ' ');
}

#include "compile_context/sr_out.inl.h"
#undef COMPILE_SIMD_BITS

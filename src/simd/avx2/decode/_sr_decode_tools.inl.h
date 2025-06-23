#ifdef SSRJSON_CLANGD_DUMMY
#    include "simd/avx2/checker.h"
#    include "simd/avx2/common.h"
#    include "simd/sse2/decode.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif

#define COMPILE_SIMD_BITS 256
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

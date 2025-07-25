#ifndef SSRJSON_SIMD_IMPL_H
#define SSRJSON_SIMD_IMPL_H

#include "simd/simd_detect.h"
#include "ssrjson.h"
#include "vector_types.h"
//

#if SSRJSON_X86
#    if __AVX512VL__ && __AVX512DQ__ && __AVX512BW__
#        include "avx512vl_dq_bw/full.h"
#    endif
#    if __AVX512F__ && __AVX512CD__
#        include "avx512f_cd/full.h"
#    endif
#    if __AVX2__
#        include "avx2/full.h"
#    endif
#    if __AVX__
#        include "avx/full.h"
#    endif
#    if __SSE4_1__
#        include "sse4.1/full.h"
#    endif
#    if __SSSE3__
#        include "ssse3/full.h"
#    endif
#    include "sse2/full.h"


#elif SSRJSON_AARCH

force_inline void write_u8_128(void *dst, vector_a_u8_128 x) {
    memcpy(dst, &x, sizeof(x));
}

force_inline void write_u16_128(void *dst, vector_a_u16_128 x) {
    memcpy(dst, &x, sizeof(x));
}

force_inline void write_u32_128(void *dst, vector_a_u32_128 x) {
    memcpy(dst, &x, sizeof(x));
}

force_inline vector_a_u16_128 elevate_1_2_to_128(vector_a_u8_64 _in) {
    return vmovl_u8(_in);
}

force_inline vector_a_u32_128 elevate_2_4_to_128(vector_a_u16_64 _in) {
    return vmovl_u16(_in);
}

force_inline vector_a_u32_128 elevate_1_4_to_128(vector_a_u8_32 _in) {
    vector_a_u32_128 _out;
    for (int i = 0; i < 4; ++i) {
        _out[i] = _in[i];
    }
    return _out;
}

#endif
#endif // SSRJSON_SIMD_IMPL_H

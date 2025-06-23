#ifndef SSRJSON_SIMD_AVX512FCD_CHECKMAX_H
#define SSRJSON_SIMD_AVX512FCD_CHECKMAX_H
#if !defined(__AVX512F__) || !__AVX512F__ || !defined(__AVX512CD__) || !__AVX512CD__
#    error "AVX512F and AVX512CD is required for this file"
#endif

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "common.h"

force_inline bool checkmax_u32_512(vector_a_u32_512 z, u32 lower_bound_minus_1) {
    const vector_a_u32_512 t = broadcast_u32_512(lower_bound_minus_1);
    return 0 == unsigned_cmpgt_bitmask_u32_512(z, t);
}

// checkmax_u16_512: AVX512VL+DQ+BW
// checkmax_u8_512: AVX512VL+DQ+BW

#endif // SSRJSON_SIMD_AVX512FCD_CHECKMAX_H

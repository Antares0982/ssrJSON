#ifndef SSRJSON_SIMD_AVX2_CHECKMAX_H
#define SSRJSON_SIMD_AVX2_CHECKMAX_H

#if !defined(__AVX2__) || !__AVX2__
#    error "AVX2 is required for this file"
#endif
#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "simd/avx2/common.h"

// return true if all elements in the vector are less than or equal to lower_bound_minus_1
force_inline bool checkmax_u32_256(vector_a_u32_256 y, u32 lower_bound_minus_1) {
    // NOTE: ucs4 range is 0-0x10FFFF. Signed compare is equal to unsigned compare
    const vector_a_u32_256 t = broadcast_u32_256(lower_bound_minus_1);
    vector_a_u32_256 mask = signed_cmpgt_u32_256(y, t);
    return testz_256(mask);
}

force_inline bool checkmax_u16_256(vector_a_u16_256 y, u16 lower_bound_minus_1) {
    const vector_a_u16_256 t = broadcast_u16_256(lower_bound_minus_1);
    vector_a_u16_256 mask = unsigned_saturate_minus_u16_256(y, t);
    return testz_256(mask);
}

force_inline bool checkmax_u8_256(vector_a_u8_256 y, u8 lower_bound_minus_1) {
    const vector_a_u8_256 t = broadcast_u8_256(lower_bound_minus_1);
    vector_a_u8_256 mask = unsigned_saturate_minus_u8_256(y, t);
    return testz_256(mask);
}

#endif // SSRJSON_SIMD_AVX2_CHECKMAX_H

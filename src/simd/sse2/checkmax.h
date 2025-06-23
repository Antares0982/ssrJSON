#ifndef SSRJSON_SIMD_SSE2_CHECKMAX_H
#define SSRJSON_SIMD_SSE2_CHECKMAX_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "common.h"

force_inline bool checkmax_u32_128(vector_a_u32_128 y, u32 lower_bound_minus_1) {
    // NOTE: ucs4 range is 0-0x10FFFF. Signed compare is equal to unsigned compare
    const vector_a_u32_128 t = broadcast_u32_128(lower_bound_minus_1);
    vector_a_u32_128 mask = signed_cmpgt_u32_128(y, t);
    return testz_128(mask);
}

force_inline bool checkmax_u16_128(vector_a_u16_128 y, u16 lower_bound_minus_1) {
    const vector_a_u16_128 t = broadcast_u16_128(lower_bound_minus_1);
    vector_a_u16_128 mask = unsigned_saturate_minus_u16_128(y, t);
    return testz_128(mask);
}

force_inline bool checkmax_u8_128(vector_a_u8_128 y, u8 lower_bound_minus_1) {
    const vector_a_u8_128 t = broadcast_u8_128(lower_bound_minus_1);
    vector_a_u8_128 mask = unsigned_saturate_minus_u8_128(y, t);
    return testz_128(mask);
}

#endif // SSRJSON_SIMD_SSE2_CHECKMAX_H

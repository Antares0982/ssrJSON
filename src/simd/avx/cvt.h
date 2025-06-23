#ifndef SSRJSON_SIMD_AVX_CVT_H
#define SSRJSON_SIMD_AVX_CVT_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "common.h"

force_inline void cvt_to_dst_u8_u8_256(u8 *dst, vector_a_u8_256 x) {
    *(vector_u_u8_256 *)dst = x;
}

force_inline void cvt_to_dst_u16_u16_256(u16 *dst, vector_a_u16_256 x) {
    *(vector_u_u16_256 *)dst = x;
}

force_inline void cvt_to_dst_u32_u32_256(u32 *dst, vector_a_u32_256 x) {
    *(vector_u_u32_256 *)dst = x;
}

#endif // SSRJSON_SIMD_AVX_CVT_H

#ifndef SSRJSON_SIMD_AVX512VLDQBW_CVT_H
#define SSRJSON_SIMD_AVX512VLDQBW_CVT_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "common.h"
#include "simd/avx512f_cd/cvt.h"

force_inline void cvt_to_dst_u8_u16_512(u16 *dst, vector_a_u8_512 y) {
    *(vector_u_u16_512 *)(dst + 0) = cvt_u8_to_u16_512(extract_256_from_512(y, 0));
    *(vector_u_u16_512 *)(dst + 32) = cvt_u8_to_u16_512(extract_256_from_512(y, 1));
}

force_inline void cvt_to_dst_u32_u8_512(u8 *dst, vector_a_u32_512 z) {
    *(vector_u_u8_128 *)dst = cvt_u32_to_u8_512(z);
}

// other: AVX512F+CD

#endif // SSRJSON_SIMD_AVX512VLDQBW_CVT_H

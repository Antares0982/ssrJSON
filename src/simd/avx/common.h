#ifndef SSRJSON_SIMD_AVX_COMMON_H
#define SSRJSON_SIMD_AVX_COMMON_H

#if !defined(__AVX__) || !__AVX__
#    error "AVX is required for this file"
#endif
#include "simd/simd_detect.h"
#include "simd/vector_types.h"

#define extract_64_from_256(_y_, _index_) ((u64)_mm256_extract_epi64((_y_), (_index_)))
#define setzero_256 _mm256_setzero_si256

/* Return true if y is all zero. */
force_inline bool testz_256(SIMD_256 y) {
    return (bool)_mm256_testz_si256(y, y);
}

force_inline vector_a_u8_256 broadcast_u8_256(u8 v) {
    return _mm256_set1_epi8((char)v);
}

force_inline vector_a_u16_256 broadcast_u16_256(u16 v) {
    return _mm256_set1_epi16((short)v);
}

force_inline vector_a_u32_256 broadcast_u32_256(u32 v) {
    return _mm256_set1_epi32((int)v);
}


#endif // SSRJSON_SIMD_AVX_COMMON_H

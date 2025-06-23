#ifndef SSRJSON_SIMD_SSSE3_COMMON_H
#define SSRJSON_SIMD_SSSE3_COMMON_H
#if !defined(__SSSE3__) || !__SSSE3__
#    error "SSSE3 is required for this file"
#endif

#include "simd/simd_detect.h"
#include "simd/vector_types.h"

#define alignr_128(_x1_, _x2_, _imm_) (_mm_alignr_epi8((_x2_), (_x1_), (_imm_)))

#define shuffle_128 _mm_shuffle_epi8


#endif // SSRJSON_SIMD_SSSE3_COMMON_H

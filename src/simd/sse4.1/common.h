#ifndef SSRJSON_SIMD_SSE4_COMMON_H
#define SSRJSON_SIMD_SSE4_COMMON_H

#define blendv_128 _mm_blendv_epi8

#define unsigned_max_u16_128 _mm_max_epu16
#define unsigned_max_u32_128 _mm_max_epu32

#endif // SSRJSON_SIMD_SSE4_COMMON_H

#ifndef SSRJSON_SIMD_SSSE3_UTF8_H
#define SSRJSON_SIMD_SSSE3_UTF8_H
#if !defined(__SSSE3__) || !__SSSE3__
#    error "SSSE3 is required for this file"
#endif
#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "simd/sse2/common.h"
#include "simd/ssse3/common.h"

/*
 * Encode UCS2 string to UTF-8, using SSSE3.
 * Need: pshufb, palignr
 */




#endif // SSRJSON_SIMD_SSSE3_UTF8_H

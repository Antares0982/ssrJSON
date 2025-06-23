#ifndef SSRJSON_SIMD_AVX2_ENCODE_UTF8_H
#define SSRJSON_SIMD_AVX2_ENCODE_UTF8_H

#ifndef __AVX2__
#    error "AVX2 is required for this file"
#endif
#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "bytes/ucs1.h"
#include "bytes/ucs2.h"
#include "bytes/ucs4.h"

#endif // SSRJSON_SIMD_AVX2_ENCODE_UTF8_H

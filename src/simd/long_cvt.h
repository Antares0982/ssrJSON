#ifndef SSRJSON_SIMD_LONG_CVT_H
#define SSRJSON_SIMD_LONG_CVT_H

#include "simd/simd_impl.h"

#include "compile_feature_check.h"
#include "long_cvt/part_back_cvt.h"
#include "long_cvt/part_cvt.h"
#undef COMPILE_SIMD_BITS

#define COMPILE_SIMD_BITS 128
#include "long_cvt/_s_long_cvt_wrap.inl.h"
#undef COMPILE_SIMD_BITS

#ifdef SSRJSON_SIMD_AVX2_CVT_H
#    define COMPILE_SIMD_BITS 256
#    include "long_cvt/_s_long_cvt_wrap.inl.h"
#    undef COMPILE_SIMD_BITS
#endif

#ifdef SSRJSON_SIMD_AVX512VLDQBW_CVT_H
#    define COMPILE_SIMD_BITS 512
#    include "long_cvt/_s_long_cvt_wrap.inl.h"
#    undef COMPILE_SIMD_BITS
#endif

#endif // SSRJSON_SIMD_LONG_CVT_H

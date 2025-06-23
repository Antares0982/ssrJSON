#ifndef SSRJSON_SIMD_AVX512VLDQBW_CHECKER_H
#define SSRJSON_SIMD_AVX512VLDQBW_CHECKER_H

#include "simd/avx512vl_dq_bw/common.h"
#include "simd/mask_table.h"
#include "simd/simd_detect.h"
#include "simd/union_vector.h"
#include "simd/vector_types.h"

#define COMPILE_READ_UCS_LEVEL 1
#include "checker/_sr_escape.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#include "checker/_sr_escape.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#include "checker/_sr_escape.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#endif // SSRJSON_SIMD_AVX512VLDQBW_CHECKER_H

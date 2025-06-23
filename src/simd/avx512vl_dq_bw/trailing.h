#ifndef SSRJSON_SIMD_AVX512VLDQBW_TRAILING_H
#define SSRJSON_SIMD_AVX512VLDQBW_TRAILING_H

#include "cvt.h"


#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#include "trailing/_srw_trailing_copy.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 2
#include "trailing/_srw_trailing_copy.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 4
#include "trailing/_srw_trailing_copy.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "trailing/_srw_trailing_copy.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 4
#include "trailing/_srw_trailing_copy.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "trailing/_srw_trailing_copy.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL


#endif // SSRJSON_SIMD_AVX512VLDQBW_TRAILING_H

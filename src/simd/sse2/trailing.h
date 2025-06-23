#ifndef SSRJSON_SIMD_SSE2_TRAILING_H
#define SSRJSON_SIMD_SSE2_TRAILING_H

#include "checker.h"
#include "cvt.h"

#define COMPILE_SIMD_BITS 128

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

#undef COMPILE_SIMD_BITS

#endif // SSRJSON_SIMD_SSE2_TRAILING_H

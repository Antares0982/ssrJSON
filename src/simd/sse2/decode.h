#ifndef SSRJSON_SIMD_SSE2_DECODE_H
#define SSRJSON_SIMD_SSE2_DECODE_H

#include "checker.h"
#include "common.h"

#define COMPILE_READ_UCS_LEVEL 1
#include "decode/_r_decode_tools.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#include "decode/_r_decode_tools.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#include "decode/_r_decode_tools.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#include "decode/_sr_decode_tools.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#include "decode/_sr_decode_tools.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#include "decode/_sr_decode_tools.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#endif // SSRJSON_SIMD_SSE2_DECODE_H

#ifndef SSRJSON_SIMD_AVX2_ENCODE_H
#define SSRJSON_SIMD_AVX2_ENCODE_H

#include "encode/utf8.h"
#include "trailing.h"

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#include "encode/_srw_loop.inl.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 2
#include "encode/_srw_loop.inl.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode/_srw_loop.inl.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "encode/_srw_loop.inl.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode/_srw_loop.inl.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode/_srw_loop.inl.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL


#endif // SSRJSON_SIMD_AVX2_ENCODE_H

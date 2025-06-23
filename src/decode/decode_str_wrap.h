#ifndef SSRJSON_DECODE_DECODE_STR_WRAP_H
#define SSRJSON_DECODE_DECODE_STR_WRAP_H

#include "decode_str_root_wrap.h"
//
#include "simd/compile_feature_check.h"

#define COMPILE_UCS_LEVEL 0
#include "decode/decode_str.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 1
#include "decode/decode_str.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 2
#include "decode/decode_str.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 4
#include "decode/decode_str.inl.h"
#undef COMPILE_UCS_LEVEL

#undef COMPILE_SIMD_BITS

#endif // SSRJSON_DECODE_DECODE_STR_WRAP_H

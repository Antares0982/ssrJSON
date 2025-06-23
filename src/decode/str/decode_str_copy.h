#ifndef SSRJSON_DECODE_STR_DECODE_STR_COPY_H
#define SSRJSON_DECODE_STR_DECODE_STR_COPY_H

#include "decoder_impl_wrap.h"
//
#include "simd/compile_feature_check.h"

#define COMPILE_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#include "decode_str_copy/_srw_decode_str_copy.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 2
#include "decode_str_copy/_srw_decode_str_copy.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 4
#include "decode_str_copy/_srw_decode_str_copy.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "decode_str_copy/_srw_decode_str_copy.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 4
#include "decode_str_copy/_srw_decode_str_copy.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "decode_str_copy/_srw_decode_str_copy.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_UCS_LEVEL

#undef COMPILE_SIMD_BITS

#endif // SSRJSON_DECODE_STR_DECODE_STR_COPY_H

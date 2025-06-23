#ifndef SSRJSON_DECODE_STR_UCS_H
#define SSRJSON_DECODE_STR_UCS_H

#include "cache_key.h"
#include "copy_to_new.h"
#include "decode_str_copy.h"
#include "process_escape.h"
#include "pythonlib.h"
#include "simd/long_cvt.h"
//
#include "simd/compile_feature_check.h"
#define COMPILE_UCS_LEVEL 1
#include "_ucs.inl.h"
#undef COMPILE_UCS_LEVEL
#define COMPILE_UCS_LEVEL 2
#include "_ucs.inl.h"
#undef COMPILE_UCS_LEVEL
#define COMPILE_UCS_LEVEL 4
#include "_ucs.inl.h"
#undef COMPILE_UCS_LEVEL
#undef COMPILE_SIMD_BITS

#endif // SSRJSON_DECODE_STR_UCS_H

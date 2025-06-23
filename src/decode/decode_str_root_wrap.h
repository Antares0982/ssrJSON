#ifndef SSRJSON_DECODE_DECODE_STR_ROOT_WRAP_H
#define SSRJSON_DECODE_DECODE_STR_ROOT_WRAP_H

#include "decode_float_utils.h"
#include "decode_float_wrap.h"
#include "ssrjson.h"
#include "str/ascii.h"
#include "str/tools.h"
#include "str/ucs.h"
//
#include "simd/compile_feature_check.h"

// ascii
#define COMPILE_UCS_LEVEL 0
#define COMPILE_READ_UCS_LEVEL 1

#define READ_ROOT_IMPL decode_root_pretty
#define DECODE_READ_PRETTY 1
#include "str/decode_str_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL
//
#define READ_ROOT_IMPL decode_root_minify
#define DECODE_READ_PRETTY 0
#include "str/decode_str_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL

#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_UCS_LEVEL

// ucs1
#define COMPILE_UCS_LEVEL 1
#define COMPILE_READ_UCS_LEVEL 1

#define READ_ROOT_IMPL decode_root_pretty
#define DECODE_READ_PRETTY 1
#include "str/decode_str_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL
//
#define READ_ROOT_IMPL decode_root_minify
#define DECODE_READ_PRETTY 0
#include "str/decode_str_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL

#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_UCS_LEVEL

// ucs2
#define COMPILE_UCS_LEVEL 2
#define COMPILE_READ_UCS_LEVEL 2

#define READ_ROOT_IMPL decode_root_pretty
#define DECODE_READ_PRETTY 1
#include "str/decode_str_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL
//
#define READ_ROOT_IMPL decode_root_minify
#define DECODE_READ_PRETTY 0
#include "str/decode_str_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL

#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_UCS_LEVEL

// ucs4
#define COMPILE_UCS_LEVEL 4
#define COMPILE_READ_UCS_LEVEL 4

#define READ_ROOT_IMPL decode_root_pretty
#define DECODE_READ_PRETTY 1
#include "str/decode_str_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL
//
#define READ_ROOT_IMPL decode_root_minify
#define DECODE_READ_PRETTY 0
#include "str/decode_str_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL

#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_UCS_LEVEL


#undef COMPILE_SIMD_BITS


#endif // SSRJSON_DECODE_DECODE_STR_ROOT_WRAP_H

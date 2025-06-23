#ifndef SSRJSON_DECODE_DECODE_BYTES_ROOT_WRAP_H
#define SSRJSON_DECODE_DECODE_BYTES_ROOT_WRAP_H

#include "decode_shared.h"
#include "decode_str_wrap.h"
#include "str/ascii.h"
#include "str/tools.h"
//
#include "simd/compile_feature_check.h"

#define READ_ROOT_IMPL read_bytes_root_pretty
#define DECODE_READ_PRETTY 1
#include "bytes/decode_bytes_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL

#define READ_ROOT_IMPL read_bytes_root_minify
#define DECODE_READ_PRETTY 0
#include "bytes/decode_bytes_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL

#undef COMPILE_SIMD_BITS

#endif // SSRJSON_DECODE_DECODE_BYTES_ROOT_WRAP_H

#ifndef SSRJSON_DECODE_TOOLS_H
#define SSRJSON_DECODE_TOOLS_H

#include "decode/decode_shared.h"
#include "simd/simd_impl.h"

// _r_tools
#define COMPILE_READ_UCS_LEVEL 1
#include "tools/_r_tools.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#include "tools/_r_tools.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#include "tools/_r_tools.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#endif // SSRJSON_DECODE_TOOLS_H

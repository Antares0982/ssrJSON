#ifndef SSRJSON_COMPILE_CONTEXT_SW
#define SSRJSON_COMPILE_CONTEXT_SW

#include "s_in.inl.h"
#include "w_in.inl.h"

#define WRITE_BATCH_COUNT (COMPILE_SIMD_BITS / 8 / sizeof(_dst_t))
#endif // SSRJSON_COMPILE_CONTEXT_SW

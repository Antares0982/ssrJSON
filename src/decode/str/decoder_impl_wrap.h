#ifndef SSRJSON_DECODE_STR_DECODER_IMPL_WRAP_H
#define SSRJSON_DECODE_STR_DECODER_IMPL_WRAP_H

#include "decode/decode_shared.h"
#include "simd/simd_impl.h"
#include "simd/union_vector.h"
#include "tools.h"
//
#include "simd/compile_feature_check.h"

// _r_impls and decoder_impl/_sr_loop_impls
#define COMPILE_READ_UCS_LEVEL 1
#include "decoder_impl/_sr_impls.inl.h"
#include "decoder_impl/_sr_loop_impls.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#include "decoder_impl/_sr_impls.inl.h"
#include "decoder_impl/_sr_loop_impls.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#include "decoder_impl/_sr_impls.inl.h"
#include "decoder_impl/_sr_loop_impls.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#undef COMPILE_SIMD_BITS

#endif // SSRJSON_DECODE_STR_DECODER_IMPL_WRAP_H

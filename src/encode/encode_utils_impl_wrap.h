#ifndef ENCODE_UTILS_IMPL_WRAP_H
#define ENCODE_UTILS_IMPL_WRAP_H

#include "encode_shared.h"
#include "simd/long_cvt/part_cvt.h"
#include "simd/simd_impl.h"
//
#include "simd/compile_feature_check.h"
/* 
 * Some utility functions only related to *write*, like unicode buffer reserve, writing number
 * need macro: COMPILE_WRITE_UCS_LEVEL, value: 1, 2, or 4.
 */

#define COMPILE_WRITE_UCS_LEVEL 1
#include "_encode_utils_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 2
#include "_encode_utils_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_utils_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#undef COMPILE_SIMD_BITS

#endif // ENCODE_UTILS_IMPL_WRAP_H

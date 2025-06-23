#ifndef ENCODE_IMPL_WRAP_H
#define ENCODE_IMPL_WRAP_H

#include "encode_shared.h"
#include "encode_unicode_impl_wrap.h"
#include "tls.h"

#include "simd/compile_feature_check.h"
#define COMPILE_INDENT_LEVEL 0

#define COMPILE_UCS_LEVEL 4
#include "_encode_impl.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 2
#include "_encode_impl.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 1
#include "_encode_impl.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 0
#include "_encode_impl.inl.h"
#undef COMPILE_UCS_LEVEL

#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 2

#define COMPILE_UCS_LEVEL 4
#include "_encode_impl.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 2
#include "_encode_impl.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 1
#include "_encode_impl.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 0
#include "_encode_impl.inl.h"
#undef COMPILE_UCS_LEVEL

#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 4

#define COMPILE_UCS_LEVEL 4
#include "_encode_impl.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 2
#include "_encode_impl.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 1
#include "_encode_impl.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 0
#include "_encode_impl.inl.h"
#undef COMPILE_UCS_LEVEL

#undef COMPILE_INDENT_LEVEL
#undef COMPILE_SIMD_BITS

#endif // ENCODE_IMPL_WRAP_H

#ifndef ENCODE_BYTES_IMPL_WRAP_H
#define ENCODE_BYTES_IMPL_WRAP_H

#include "encode/encode_impl_wrap.h"
#include "encode/encode_shared.h"
#include "encode_utf8.h"
#include "ssrjson.h"
#include "tls.h"
#include "utils/unicode.h"


#define COMPILE_INDENT_LEVEL 0
#include "_encode_bytes_impl.inl.h"
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 2
#include "_encode_bytes_impl.inl.h"
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 4
#include "_encode_bytes_impl.inl.h"
#undef COMPILE_INDENT_LEVEL

#endif // ENCODE_BYTES_IMPL_WRAP_H

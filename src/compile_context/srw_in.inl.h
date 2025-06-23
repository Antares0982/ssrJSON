#ifndef SSRJSON_COMPILE_CONTEXT_SRW
#define SSRJSON_COMPILE_CONTEXT_SRW
#include "rw_in.inl.h"
#include "sr_in.inl.h"
#include "sw_in.inl.h"

// Name creation macros.
#define MAKE_SRW_NAME(_x_) SSRJSON_CONCAT4(_x_, _src_t, _dst_t, COMPILE_SIMD_BITS)

#define trailing_copy_with_cvt MAKE_SRW_NAME(trailing_copy_with_cvt)
#define encode_trailing_copy_with_cvt MAKE_SRW_NAME(encode_trailing_copy_with_cvt)
#define cvt_to_dst MAKE_SRW_NAME(cvt_to_dst)
#define cvt_to_dst_blendhigh MAKE_SRW_NAME(cvt_to_dst_blendhigh)
#define encode_unicode_loop MAKE_SRW_NAME(encode_unicode_loop)
#define encode_unicode_loop4 MAKE_SRW_NAME(encode_unicode_loop4)
#define encode_unicode_impl MAKE_SRW_NAME(encode_unicode_impl)
#define encode_unicode_impl_noinline MAKE_SRW_NAME(encode_unicode_impl_noinline)
#define long_cvt MAKE_SRW_NAME(long_cvt)
#define long_back_cvt MAKE_SRW_NAME(long_back_cvt)
#endif // SSRJSON_COMPILE_CONTEXT_SRW

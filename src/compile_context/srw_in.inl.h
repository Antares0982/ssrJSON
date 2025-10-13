/*==============================================================================
 Copyright (c) 2025 Antares <antares0982@gmail.com>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *============================================================================*/

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
#define _addr_cvt MAKE_SRW_NAME(_addr_cvt)
#define _addr_cvt4 MAKE_SRW_NAME(_addr_cvt4)
#define encode_unicode_loop MAKE_SRW_NAME(encode_unicode_loop)
#define encode_unicode_loop4 MAKE_SRW_NAME(encode_unicode_loop4)
#define encode_unicode_impl MAKE_SRW_NAME(encode_unicode_impl)
#define encode_unicode_impl_no_key MAKE_SRW_NAME(encode_unicode_impl_no_key)
#define long_cvt MAKE_SRW_NAME(long_cvt)
#define long_back_cvt MAKE_SRW_NAME(long_back_cvt)
#endif // SSRJSON_COMPILE_CONTEXT_SRW

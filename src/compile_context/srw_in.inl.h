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

/* Generate function names with SIMD level, reader type and writer type. */
#define make_srw_name(_x_) ssrjson_concat4(_x_, src_t, dst_t, _CompileVectorBits)

#define trailing_copy_with_cvt make_srw_name(trailing_copy_with_cvt)
#define encode_trailing_copy_with_cvt make_srw_name(encode_trailing_copy_with_cvt)
#define cvt_to_dst make_srw_name(cvt_to_dst)
#define _addr_cvt make_srw_name(_addr_cvt)
#define encode_unicode_loop make_srw_name(encode_unicode_loop)
#define encode_unicode_impl make_srw_name(encode_unicode_impl)
#define long_cvt make_srw_name(long_cvt)
#define long_back_cvt make_srw_name(long_back_cvt)
#endif // SSRJSON_COMPILE_CONTEXT_SRW

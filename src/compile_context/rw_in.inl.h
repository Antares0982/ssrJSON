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

#ifndef SSRJSON_COMPILE_CONTEXT_RW
#define SSRJSON_COMPILE_CONTEXT_RW

// Include sub contexts.
#include "r_in.inl.h"
#include "w_in.inl.h"


// Name creation macro.
#define MAKE_RW_NAME(_x_) SSRJSON_CONCAT3(_x_, _src_t, _dst_t)

#define avx2_trailing_cvt MAKE_RW_NAME(avx2_trailing_cvt)

#ifdef COMPILE_UCS_LEVEL
#    define MAKE_UCS_W_NAME(_x_) MAKE_W_NAME(MAKE_UCS_NAME(_x_))
// some decoder impls
#    define decode_str_copy_loop4 MAKE_UCS_W_NAME(decode_str_copy_loop4)
#    define decode_str_copy_loop MAKE_UCS_W_NAME(decode_str_copy_loop)
#    define decode_str_copy_trailing MAKE_UCS_W_NAME(decode_str_copy_trailing)
#    define process_escape MAKE_UCS_W_NAME(process_escape)
#endif

#endif // SSRJSON_COMPILE_CONTEXT_RW

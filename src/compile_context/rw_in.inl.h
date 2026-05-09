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


/* Generate function names with reader type and writer type. */
#define make_rw_name(_x_) ssrjson_concat3(_x_, src_t, dst_t)

#define avx2_trailing_cvt make_rw_name(avx2_trailing_cvt)
#define u_buf_apd_key_impl make_rw_name(u_buf_apd_key_impl)
#define u_buf_apd_str_impl make_rw_name(u_buf_apd_str_impl)

#ifdef COMPILE_UCS_LEVEL
/* Generate function names with unicode type and writer type. */
#    define make_ucs_w_name(_x_) make_w_name(make_ucs_name(_x_))
// some decoder impls
#    define decode_str_copy_loop4 make_ucs_w_name(decode_str_copy_loop4)
#    define decode_str_copy_loop make_ucs_w_name(decode_str_copy_loop)
#    define decode_str_copy_trailing make_ucs_w_name(decode_str_copy_trailing)
#    define process_escape make_ucs_w_name(process_escape)
#endif

#endif // SSRJSON_COMPILE_CONTEXT_RW

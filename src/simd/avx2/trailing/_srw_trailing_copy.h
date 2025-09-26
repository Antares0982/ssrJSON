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

/* This file is unused but left here intentionally for future use. */

#ifdef SSRJSON_CLANGD_DUMMY
#    include "simd/avx2/checker.h"
#    include "simd/avx2/common.h"
#    include "simd/avx2/cvt.h"
#    include "simd/sse2/encode.h"
#    include "simd/sse2/trailing.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif
#endif
//
#define COMPILE_SIMD_BITS 256

#include "compile_context/srw_in.inl.h"

// unused
force_inline void trailing_copy_with_cvt(_dst_t **dst_addr, const _src_t *src, usize len) {
    // use 128-bits trailing impl
    if (len >= READ_BATCH_COUNT / 2) {
#define half_vec_t SSRJSON_CONCAT4(vector, a, _src_t, 128)
#define half_vec_u_t SSRJSON_CONCAT4(vector, u, _src_t, 128)
#define half_cvt SSRJSON_CONCAT5(cvt_to, dst, _src_t, _dst_t, 128)
        half_vec_t half_vec = *(half_vec_u_t *)src;
        half_cvt(*dst_addr, half_vec);
        *dst_addr += READ_BATCH_COUNT / 2;
        src += READ_BATCH_COUNT / 2;
        len -= READ_BATCH_COUNT / 2;
        if (!len) return;
#undef half_cvt
#undef half_vec_u_t
#undef half_vec_t
    }
    SSRJSON_CONCAT5(trailing_copy_with, cvt, _src_t, _dst_t, 128)(dst_addr, src, len);
}

#undef COMPILE_SIMD_BITS
#include "compile_context/srw_out.inl.h"

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
#    include "simd/sse2/checker.h"
#    include "simd/sse2/common.h"
#    include "simd/sse2/cvt.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif
#endif
//
#define COMPILE_SIMD_BITS 128

#include "compile_context/srw_in.inl.h"
extern const Py_ssize_t _ControlJump[_Slash + 1];
extern const _dst_t ControlEscapeTable[(_Slash + 1) * 8];

force_inline void trailing_copy_with_cvt(_dst_t **dst_addr, const _src_t *src, usize copy_len) {
    _dst_t *dst = *dst_addr;
    assert(copy_len * sizeof(_src_t) < 16);
    const _src_t *const load_start = src + copy_len - 16 / sizeof(_src_t);
    const vector_a vec = *(vector_u *)load_start;
    vector_a vec_shifted = runtime_byte_rshift_128(vec, 16 - copy_len * sizeof(_src_t));
    cvt_to_dst(dst, vec_shifted);
    dst += copy_len;
    *dst_addr = dst;
}

#undef COMPILE_SIMD_BITS
#include "compile_context/srw_out.inl.h"

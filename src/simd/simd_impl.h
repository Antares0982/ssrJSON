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

#ifndef SSRJSON_SIMD_IMPL_H
#define SSRJSON_SIMD_IMPL_H

#include "simd/simd_detect.h"
#include "ssrjson.h"
#include "vector_types.h"
//

#if SSRJSON_X86
#    if __AVX512VL__ && __AVX512DQ__ && __AVX512BW__
#        include "avx512vl_dq_bw/full.h"
#    endif
#    if __AVX512F__ && __AVX512CD__
#        include "avx512f_cd/full.h"
#    endif
#    if __AVX2__
#        include "avx2/full.h"
#    endif
#    if __AVX__
#        include "avx/full.h"
#    endif
#    if __SSE4_1__
#        include "sse4.1/full.h"
#    endif
#    if __SSSE3__
#        include "ssse3/full.h"
#    endif
#    include "sse2/full.h"


#elif SSRJSON_AARCH

force_inline void write_u8_128(void *dst, vector_a_u8_128 x) {
    memcpy(dst, &x, sizeof(x));
}

force_inline void write_u16_128(void *dst, vector_a_u16_128 x) {
    memcpy(dst, &x, sizeof(x));
}

force_inline void write_u32_128(void *dst, vector_a_u32_128 x) {
    memcpy(dst, &x, sizeof(x));
}

force_inline vector_a_u16_128 elevate_1_2_to_128(vector_a_u8_64 _in) {
    return vmovl_u8(_in);
}

force_inline vector_a_u32_128 elevate_2_4_to_128(vector_a_u16_64 _in) {
    return vmovl_u16(_in);
}

force_inline vector_a_u32_128 elevate_1_4_to_128(vector_a_u8_32 _in) {
    vector_a_u32_128 _out;
    for (int i = 0; i < 4; ++i) {
        _out[i] = _in[i];
    }
    return _out;
}

#endif
#endif // SSRJSON_SIMD_IMPL_H

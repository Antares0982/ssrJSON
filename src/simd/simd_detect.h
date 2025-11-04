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

#ifndef SSRJSON_SIMD_DETECT_H
#define SSRJSON_SIMD_DETECT_H

#define HAS_AVX512 0
#define HAS_AVX2 0
#define HAS_SSE4_2 0
#define USING_AVX512 0
#define USING_AVX2 0
#define USING_SSE4_2 0

#if SSRJSON_X86
#    if __AVX512F__ && __AVX512CD__ && __AVX512BW__ && __AVX512VL__ && __AVX512DQ__
#        define SIMD_FEATURE_NAME avx512
#        undef USING_AVX512
#        define USING_AVX512 1
#        undef HAS_AVX512
#        define HAS_AVX512 1
#        undef HAS_AVX2
#        define HAS_AVX2 1
#        undef HAS_SSE4_2
#        define HAS_SSE4_2 1
#    elif __AVX2__
#        define SIMD_FEATURE_NAME avx2
#        undef USING_AVX2
#        define USING_AVX2 1
#        undef HAS_AVX2
#        define HAS_AVX2 1
#        undef HAS_SSE4_2
#        define HAS_SSE4_2 1
#    else
#        if __SSE4_2__
#            define SIMD_FEATURE_NAME sse4_2
#            undef USING_SSE4_2
#            define USING_SSE4_2 1
#            undef HAS_SSE4_2
#            define HAS_SSE4_2 1
#        else
#            define SIMD_FEATURE_NAME sse2
#        endif
#    endif

#    define SIMD_128 __m128i
#    if defined(_MSC_VER)
#        define SIMD_128_IU __m128i
#    else
#        define SIMD_128_IU __m128i_u
#    endif
#    define SIMD_256 __m256i
#    if defined(_MSC_VER)
#        define SIMD_256_IU __m256i
#    else
#        define SIMD_256_IU __m256i_u
#    endif
#    define SIMD_512 __m512i
// x86: WRITE_SUPPORT_MASK_WRITE
#    if __AVX512F__ && __AVX512CD__ && __AVX512BW__ && __AVX512VL__ && __AVX512DQ__
#        define WRITE_SUPPORT_MASK_WRITE 1
#    else
#        define WRITE_SUPPORT_MASK_WRITE 0
#    endif
#elif SSRJSON_AARCH
#    define SIMD_FEATURE_NAME neon
#    define WRITE_SUPPORT_MASK_WRITE 0
#    define USING_AVX512 0
#    define USING_AVX2 0
#else
#    error "unsupported architecture"
#endif


#if BUILD_MULTI_LIB
#    ifndef SIMD_FEATURE_NAME
#        error "SIMD_FEATURE_NAME is not defined"
#    endif
#    define SIMD_NAME_MODIFIER(x) SSRJSON_CONCAT2(x, SIMD_FEATURE_NAME)
#else
#    define SIMD_NAME_MODIFIER(x) x
#endif

#if SSRJSON_X86
#    include <immintrin.h>
#    if defined(_MSC_VER)
#        include <intrin.h>
#    endif
#elif SSRJSON_AARCH
#    include <arm_neon.h>
#    include <assert.h>
static_assert(__LITTLE_ENDIAN__, "currently only little endian is supported");
#endif


#endif // SSRJSON_SIMD_DETECT_H

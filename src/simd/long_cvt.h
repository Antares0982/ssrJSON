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

#ifndef SSRJSON_SIMD_LONG_CVT_H
#define SSRJSON_SIMD_LONG_CVT_H

#include "simd/simd_impl.h"

#include "compile_feature_check.h"
#include "long_cvt/part_back_cvt.h"
#include "long_cvt/part_cvt.h"
#undef COMPILE_SIMD_BITS

#define COMPILE_SIMD_BITS 128
#include "long_cvt/_s_long_cvt_wrap.inl.h"
#undef COMPILE_SIMD_BITS

#ifdef SSRJSON_SIMD_AVX2_CVT_H
#    define COMPILE_SIMD_BITS 256
#    include "long_cvt/_s_long_cvt_wrap.inl.h"
#    undef COMPILE_SIMD_BITS
#endif

#ifdef SSRJSON_SIMD_AVX512VLDQBW_CVT_H
#    define COMPILE_SIMD_BITS 512
#    include "long_cvt/_s_long_cvt_wrap.inl.h"
#    undef COMPILE_SIMD_BITS
#endif

#endif // SSRJSON_SIMD_LONG_CVT_H

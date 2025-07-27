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

#ifndef SSRJSON_DECODE_STR_COPY_TO_NEW_H
#define SSRJSON_DECODE_STR_COPY_TO_NEW_H
#include "decode/decode_shared.h"
#include "pythonlib.h"
#include "simd/long_cvt.h"
#include "simd/memcpy.h"
#include "utils/unicode.h"
//
#include "simd/compile_feature_check.h"
//
#include "compile_context/s_in.inl.h"

force_inline void copy_to_new_unicode_ucs1(void **dst_addr, PyObject *ret, bool need_cvt, const u8 *src, usize count, int kind) {
    u8 *dst = need_cvt ? PYUNICODE_ASCII_START(ret) : PYUNICODE_UCS1_START(ret);
    *dst_addr = dst;
    ssrjson_memcpy(dst, src, count);
}

force_inline void copy_to_new_unicode_ucs2(void **dst_addr, PyObject *ret, bool need_cvt, const u16 *src, usize count, int kind) {
    if (!need_cvt) {
        u16 *dst = PYUNICODE_UCS2_START(ret);
        *dst_addr = dst;
        ssrjson_memcpy(dst, src, count * 2);
    } else {
        u8 *dst = (kind == 0) ? PYUNICODE_ASCII_START(ret) : PYUNICODE_UCS1_START(ret);
        *dst_addr = dst;
        MAKE_S_NAME(long_cvt_u16_u8)(dst, src, count);
    }
}

force_inline void copy_to_new_unicode_ucs4(void **dst_addr, PyObject *ret, bool need_cvt, const u32 *src, usize count, int kind) {
    if (!need_cvt) {
        u32 *dst = PYUNICODE_UCS4_START(ret);
        *dst_addr = dst;
        ssrjson_memcpy(dst, src, count * 4);
    } else {
        if (kind <= 1) {
            u8 *dst = (kind == 0) ? PYUNICODE_ASCII_START(ret) : PYUNICODE_UCS1_START(ret);
            *dst_addr = dst;
            MAKE_S_NAME(long_cvt_u32_u8)(dst, src, count);
        } else {
            // this should be unlikely, use noinline version
            u16 *dst = PYUNICODE_UCS2_START(ret);
            *dst_addr = dst;
            long_cvt_noinline_u32_u16_interface(dst, src, count);
        }
    }
}

#undef COMPILE_SIMD_BITS
#include "compile_context/s_out.inl.h"

#endif // SSRJSON_DECODE_STR_COPY_TO_NEW_H

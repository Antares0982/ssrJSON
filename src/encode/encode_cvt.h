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

#ifndef SSRJSON_ENCODE_CVT_H
#define SSRJSON_ENCODE_CVT_H

#include "encode_shared.h"
#include "simd/cvt.h"
#include "simd/simd_detect.h"
#include "ssrjson.h"
#include "utils/unicode.h"

force_inline void ascii_elevate2(EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    u8 *start = ((u8 *)GET_VEC_ASCII_START(unicode_buffer_info));
    u16 *write_start = ((u16 *)GET_VEC_COMPACT_START(unicode_buffer_info));
    SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u16)(write_start, start, unicode_info->ascii_size);
}

force_inline void ascii_elevate4(EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    u8 *start = ((u8 *)GET_VEC_ASCII_START(unicode_buffer_info));
    u32 *write_start = ((u32 *)GET_VEC_COMPACT_START(unicode_buffer_info));
    SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u32)(write_start, start, unicode_info->ascii_size);
}

force_inline void ucs1_elevate2(EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t offset = unicode_info->ascii_size;
    u8 *start = ((u8 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + offset;
    u16 *write_start = ((u16 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + offset;
    SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u16)(write_start, start, unicode_info->u8_size);
}

force_inline void ucs1_elevate4(EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t offset = unicode_info->ascii_size;
    u8 *start = ((u8 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + offset;
    u32 *write_start = ((u32 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + offset;
    SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u32)(write_start, start, unicode_info->u8_size);
}

force_inline void ucs2_elevate4(EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t offset = unicode_info->ascii_size + unicode_info->u8_size;
    u16 *start = ((u16 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + offset;
    u32 *write_start = ((u32 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + offset;
    SIMD_NAME_MODIFIER(long_back_cvt_noinline_u16_u32)(write_start, start, unicode_info->u16_size);
}

force_inline void ascii_elevate1(EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    memmove(GET_VEC_COMPACT_START(unicode_buffer_info), GET_VEC_ASCII_START(unicode_buffer_info), unicode_info->ascii_size);
}

#endif // SSRJSON_ENCODE_CVT_H

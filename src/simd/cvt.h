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

#ifndef SIMD_CVT_H
#define SIMD_CVT_H

#include "simd/mask_table.h"
#include "simd_detect.h"
#include "utils/unicode.h"


SSRJSON_EXPORTED_SYMBOL void SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u16)(u16 *write_start, const u8 *read_start, usize len);


SSRJSON_EXPORTED_SYMBOL void SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u32)(u32 *write_start, const u8 *read_start, usize len);


SSRJSON_EXPORTED_SYMBOL void SIMD_NAME_MODIFIER(long_back_cvt_noinline_u16_u32)(u32 *write_start, const u16 *read_start, usize len);


SSRJSON_EXPORTED_SYMBOL void SIMD_NAME_MODIFIER(long_cvt_noinline_u8_u16)(u16 *restrict write_start, const u8 *restrict read_start, usize _len);


SSRJSON_EXPORTED_SYMBOL void SIMD_NAME_MODIFIER(long_cvt_noinline_u8_u32)(u32 *restrict write_start, const u8 *restrict read_start, usize _len);


SSRJSON_EXPORTED_SYMBOL void SIMD_NAME_MODIFIER(long_cvt_noinline_u16_u32)(u32 *restrict write_start, const u16 *restrict read_start, usize _len);


SSRJSON_EXPORTED_SYMBOL void SIMD_NAME_MODIFIER(long_cvt_noinline_u32_u16)(u16 *restrict write_start, const u32 *restrict read_start, usize _len);


SSRJSON_EXPORTED_SYMBOL void SIMD_NAME_MODIFIER(long_cvt_noinline_u32_u8)(u8 *restrict write_start, const u32 *restrict read_start, usize _len);


SSRJSON_EXPORTED_SYMBOL void SIMD_NAME_MODIFIER(long_cvt_noinline_u16_u8)(u8 *restrict write_start, const u16 *restrict read_start, usize _len);

#endif // SIMD_CVT_H

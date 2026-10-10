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

#ifndef SSRJSON_SCALAR_UTF8_H
#define SSRJSON_SCALAR_UTF8_H
#include "encode/encode_utf8_shared.h"
#define __readbefore_bytes_write_ucs1_trailing_128 0
#define __excess_bytes_write_ucs1_trailing_128 8
#define bytes_write_ucs1_trailing_128 encode_bytes_ucs1_scalar
#define __readbefore_bytes_write_ucs1_raw_utf8_trailing_128 0
#define __excess_bytes_write_ucs1_raw_utf8_trailing_128 8
#define bytes_write_ucs1_raw_utf8_trailing_128 encode_bytes_ucs1_raw_utf8_scalar
#define __readbefore_bytes_write_ucs2_trailing_128 0
#define __excess_bytes_write_ucs2_trailing_128 8
#define bytes_write_ucs2_trailing_128 encode_bytes_ucs2_scalar
#define __readbefore_bytes_write_ucs2_raw_utf8_trailing_128 0
#define __excess_bytes_write_ucs2_raw_utf8_trailing_128 8
#define bytes_write_ucs2_raw_utf8_trailing_128 encode_bytes_ucs2_raw_utf8_scalar

force_inline void ucs2_encode_2bytes_utf8_scalar(u8 *dst, vector_a_u16_128 x) {
    for (usize i = 0; i < 8; ++i) {
        *dst++ = (u8)((x[i] >> 6) | 0xc0);
        *dst++ = (u8)((x[i] & 0x3f) | 0x80);
    }
}

force_inline void ucs2_encode_3bytes_utf8_scalar(u8 *dst, vector_a_u16_128 x) {
    for (usize i = 0; i < 8; ++i) {
        *dst++ = (u8)((x[i] >> 12) | 0xe0);
        *dst++ = (u8)(((x[i] >> 6) & 0x3f) | 0x80);
        *dst++ = (u8)((x[i] & 0x3f) | 0x80);
    }
}

#define __readbefore_bytes_write_ucs4_trailing_128 0
#define __excess_bytes_write_ucs4_trailing_128 8
#define bytes_write_ucs4_trailing_128 encode_bytes_ucs4_scalar
#define __readbefore_bytes_write_ucs4_raw_utf8_trailing_128 0
#define __excess_bytes_write_ucs4_raw_utf8_trailing_128 8
#define bytes_write_ucs4_raw_utf8_trailing_128 encode_bytes_ucs4_raw_utf8_scalar

force_inline void ucs4_encode_2bytes_utf8_scalar(u8 *dst, vector_a_u32_128 x) {
    for (usize i = 0; i < 4; ++i) {
        *dst++ = (u8)((x[i] >> 6) | 0xc0);
        *dst++ = (u8)((x[i] & 0x3f) | 0x80);
    }
}

force_inline void ucs4_encode_3bytes_utf8_scalar(u8 *dst, vector_a_u32_128 x) {
    for (usize i = 0; i < 4; ++i) {
        *dst++ = (u8)((x[i] >> 12) | 0xe0);
        *dst++ = (u8)(((x[i] >> 6) & 0x3f) | 0x80);
        *dst++ = (u8)((x[i] & 0x3f) | 0x80);
    }
}
#endif

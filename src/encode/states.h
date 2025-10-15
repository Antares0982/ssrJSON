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

#ifndef SSRJSON_ENCODE_STATES_H
#define SSRJSON_ENCODE_STATES_H

#include "encode_shared.h"
#include "utils/unicode.h"

force_inline void memorize_ascii_to_ucs4(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t len = WRITER_ADDR_AS_U8(writer_addr) - (u8 *)GET_VEC_ASCII_START(unicode_buffer_info);
    unicode_info->ascii_size = len;
    u32 *new_write_ptr = ((u32 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + len;
    WRITER_ADDR_AS_U32(writer_addr) = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 0);
    unicode_info->cur_ucs_type = 4;
}

force_inline void memorize_ascii_to_ucs2(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t len = WRITER_ADDR_AS_U8(writer_addr) - (u8 *)GET_VEC_ASCII_START(unicode_buffer_info);
    unicode_info->ascii_size = len;
    u16 *new_write_ptr = ((u16 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + len;
    WRITER_ADDR_AS_U16(writer_addr) = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 0);
    unicode_info->cur_ucs_type = 2;
}

force_inline void memorize_ascii_to_ucs1(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t len = WRITER_ADDR_AS_U8(writer_addr) - (u8 *)GET_VEC_ASCII_START(unicode_buffer_info);
    unicode_info->ascii_size = len;
    u8 *new_write_ptr = ((u8 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + len;
    WRITER_ADDR_AS_U8(writer_addr) = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 0);
    unicode_info->cur_ucs_type = 1;
}

force_inline void memorize_ucs1_to_ucs2(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t diff = WRITER_ADDR_AS_U8(writer_addr) - (u8 *)GET_VEC_COMPACT_START(unicode_buffer_info);
    Py_ssize_t len = diff - unicode_info->ascii_size;
    assert(len >= 0);
    unicode_info->u8_size = len;
    u16 *new_write_ptr = ((u16 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + diff;
    WRITER_ADDR_AS_U16(writer_addr) = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 1);
    unicode_info->cur_ucs_type = 2;
}

force_inline void memorize_ucs1_to_ucs4(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t diff = WRITER_ADDR_AS_U8(writer_addr) - (u8 *)GET_VEC_COMPACT_START(unicode_buffer_info);
    Py_ssize_t len = diff - unicode_info->ascii_size;
    assert(len >= 0);
    unicode_info->u8_size = len;
    u32 *new_write_ptr = ((u32 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + diff;
    WRITER_ADDR_AS_U32(writer_addr) = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 1);
    unicode_info->cur_ucs_type = 4;
}

force_inline void memorize_ucs2_to_ucs4(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t diff = WRITER_ADDR_AS_U16(writer_addr) - (u16 *)GET_VEC_COMPACT_START(unicode_buffer_info);
    Py_ssize_t len = diff - unicode_info->ascii_size - unicode_info->u8_size;
    assert(len >= 0);
    unicode_info->u16_size = len;
    u32 *new_write_ptr = ((u32 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + diff;
    WRITER_ADDR_AS_U32(writer_addr) = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 2);
    unicode_info->cur_ucs_type = 4;
}


#endif // SSRJSON_ENCODE_STATES_H

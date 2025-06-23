#ifndef SSRJSON_ENCODE_STATES_H
#define SSRJSON_ENCODE_STATES_H

#include "encode_shared.h"
#include "utils/unicode.h"

force_inline void memorize_ascii_to_ucs4(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t len = writer_addr->writer_u8 - (u8 *)GET_VEC_ASCII_START(unicode_buffer_info);
    unicode_info->ascii_size = len;
    u32 *new_write_ptr = ((u32 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + len;
    writer_addr->writer_u32 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 0);
    unicode_info->cur_ucs_type = 4;
}

force_inline void memorize_ascii_to_ucs2(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t len = writer_addr->writer_u8 - (u8 *)GET_VEC_ASCII_START(unicode_buffer_info);
    unicode_info->ascii_size = len;
    u16 *new_write_ptr = ((u16 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + len;
    writer_addr->writer_u16 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 0);
    unicode_info->cur_ucs_type = 2;
}

force_inline void memorize_ascii_to_ucs1(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t len = writer_addr->writer_u8 - (u8 *)GET_VEC_ASCII_START(unicode_buffer_info);
    unicode_info->ascii_size = len;
    u8 *new_write_ptr = ((u8 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + len;
    writer_addr->writer_u8 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 0);
    unicode_info->cur_ucs_type = 1;
}

force_inline void memorize_ucs1_to_ucs2(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t diff = writer_addr->writer_u8 - (u8 *)GET_VEC_COMPACT_START(unicode_buffer_info);
    Py_ssize_t len = diff - unicode_info->ascii_size;
    assert(len >= 0);
    unicode_info->u8_size = len;
    u16 *new_write_ptr = ((u16 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + diff;
    writer_addr->writer_u16 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 1);
    unicode_info->cur_ucs_type = 2;
}

force_inline void memorize_ucs1_to_ucs4(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t diff = writer_addr->writer_u8 - (u8 *)GET_VEC_COMPACT_START(unicode_buffer_info);
    Py_ssize_t len = diff - unicode_info->ascii_size;
    assert(len >= 0);
    unicode_info->u8_size = len;
    u32 *new_write_ptr = ((u32 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + diff;
    writer_addr->writer_u32 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 1);
    unicode_info->cur_ucs_type = 4;
}

force_inline void memorize_ucs2_to_ucs4(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, EncodeUnicodeInfo *unicode_info) {
    Py_ssize_t diff = writer_addr->writer_u16 - (u16 *)GET_VEC_COMPACT_START(unicode_buffer_info);
    Py_ssize_t len = diff - unicode_info->ascii_size - unicode_info->u8_size;
    assert(len >= 0);
    unicode_info->u16_size = len;
    u32 *new_write_ptr = ((u32 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + diff;
    writer_addr->writer_u32 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 2);
    unicode_info->cur_ucs_type = 4;
}


#endif // SSRJSON_ENCODE_STATES_H

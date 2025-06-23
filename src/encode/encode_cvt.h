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

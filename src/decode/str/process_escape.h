#ifndef SSRJSON_DECODE_STR_ESCAPE_H
#define SSRJSON_DECODE_STR_ESCAPE_H

#include "decode/decode_shared.h"

force_inline int process_escape_ucs1_u8(
        EscapeInfo escape_info,
        u8 **u8writer_addr,
        u16 **u16writer_addr,
        u32 **u32writer_addr,
        usize *u8size_addr,
        u32 *max_escapeval_addr,
        void *temp_buffer) {
    u32 escape_val;
    usize escape_len;
    escape_val = escape_info.escape_val;
    *max_escapeval_addr = SSRJSON_MAX(*max_escapeval_addr, escape_val);
    assert(escape_val != _DECODE_UNICODE_ERR);
    if (escape_val < 0x100) {
        *(*u8writer_addr)++ = (u8)escape_val;
        return 1;
    } else if (escape_val < 0x10000) {
        // R: ucs1,ucs2 W: ucs1,ucs2
        usize u8size = (*u8writer_addr) - SSRJSON_CAST(u8 *, temp_buffer);
        *u8size_addr = u8size;
        *u8writer_addr = NULL;
        *u16writer_addr = SSRJSON_CAST(u16 *, temp_buffer) + u8size;
        *(*u16writer_addr)++ = (u16)escape_val;
        return 2;
    } else {
        usize u8size = (*u8writer_addr) - SSRJSON_CAST(u8 *, temp_buffer);
        *u8size_addr = u8size;
        *u8writer_addr = NULL;
        *u32writer_addr = SSRJSON_CAST(u32 *, temp_buffer) + u8size;
        *(*u32writer_addr)++ = escape_val;
        return 4;
    }
}

force_inline int process_escape_ucs1_u16(
        EscapeInfo escape_info,
        u16 **u16writer_addr,
        u32 **u32writer_addr,
        usize *u8size_addr,
        usize *u16size_addr,
        u32 *max_escapeval_addr,
        void *temp_buffer) {
    u32 escape_val;
    usize escape_len;
    escape_val = escape_info.escape_val;
    *max_escapeval_addr = SSRJSON_MAX(*max_escapeval_addr, escape_val);
    assert(escape_val != _DECODE_UNICODE_ERR);
    if (escape_val < 0x10000) {
        *(*u16writer_addr)++ = (u16)escape_val;
        return 2;
    } else {
        usize totalsize = (*u16writer_addr) - SSRJSON_CAST(u16 *, temp_buffer);
        *u16size_addr = totalsize - *u8size_addr;
        *u16writer_addr = NULL;
        *u32writer_addr = SSRJSON_CAST(u32 *, temp_buffer) + totalsize;
        *(*u32writer_addr)++ = escape_val;
        return 4;
    }
}

force_inline int process_escape_ucs2_u16(
        EscapeInfo escape_info,
        u16 **u16writer_addr,
        u32 **u32writer_addr,
        usize *u16size_addr,
        u32 *max_escapeval_addr,
        void *temp_buffer) {
    u32 escape_val;
    usize escape_len;
    escape_val = escape_info.escape_val;
    *max_escapeval_addr = SSRJSON_MAX(*max_escapeval_addr, escape_val);
    assert(escape_val != _DECODE_UNICODE_ERR);
    if (escape_val < 0x10000) {
        *(*u16writer_addr)++ = (u16)escape_val;
        return 2;
    } else {
        usize u16size = (*u16writer_addr) - SSRJSON_CAST(u16 *, temp_buffer);
        *u16size_addr = u16size;
        *u16writer_addr = NULL;
        *u32writer_addr = SSRJSON_CAST(u32 *, temp_buffer) + u16size;
        *(*u32writer_addr)++ = escape_val;
        return 4;
    }
}

force_inline void process_escape_to_u32(
        EscapeInfo escape_info,
        u32 **u32writer_addr,
        u32 *max_escapeval_addr) {
    u32 escape_val;
    usize escape_len;
    escape_val = escape_info.escape_val;
    *max_escapeval_addr = SSRJSON_MAX(*max_escapeval_addr, escape_val);
    assert(escape_val != _DECODE_UNICODE_ERR);
    *(*u32writer_addr)++ = escape_val;
}

#endif // SSRJSON_DECODE_STR_ESCAPE_H

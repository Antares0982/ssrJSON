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

#ifdef SSRJSON_CLANGD_DUMMY
#    ifndef COMPILE_CONTEXT_ENCODE
#        define COMPILE_CONTEXT_ENCODE
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        include "encode_shared.h"
#        include "simd/long_cvt/part_cvt.h"
#        include "simd/simd_impl.h"
#        define COMPILE_WRITE_UCS_LEVEL 2
#        include "simd/compile_feature_check.h"
#    endif
#endif

#include "compile_context/sw_in.inl.h"

#define _ELEVATE_FROM_U8_NUM_BUFFER MAKE_W_NAME(_elevate_u8_copy)

extern u8 *dragonbox_to_chars_n(double value, u8 *buffer);

/*
 * (PRIVATE)
 * Convert the u8 buffer to the buffer.
 * The space (32 * sizeof(_dst_t)) must be reserved before calling this function.
 */
#if COMPILE_WRITE_UCS_LEVEL > 1
force_inline void _ELEVATE_FROM_U8_NUM_BUFFER(_dst_t **writer_addr, const u8 *buffer, Py_ssize_t len) {
    _dst_t *writer = *writer_addr;
#    if COMPILE_WRITE_UCS_LEVEL == 2
    __partial_cvt_32_u8_u16(&writer, &buffer);
#    else // COMPILE_WRITE_UCS_LEVEL == 4
    __partial_cvt_32_u8_u32(&writer, &buffer);
#    endif
    *writer_addr += len;
}
#endif

/*
 * Write a u64 number to the buffer.
 * The space (32 * sizeof(_dst_t)) must be reserved before calling this function.
 */
force_inline void u64_to_unicode(_dst_t **writer_addr, u64 val, usize sign) {
    assert(sign <= 1);
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = *writer_addr; //_CAST_WRITER(unicode_buffer_info);
#else
    u8 _buffer[64];
    u8 *buffer = _buffer;
#endif
    if (sign) *buffer = '-';
    u8 *buffer_end = write_u64(val, buffer + sign);
#if COMPILE_WRITE_UCS_LEVEL == 1
    *writer_addr = buffer_end;
    // unicode_buffer_info->writer.writer_u8 = buffer_end;
#else
    Py_ssize_t write_len = buffer_end - buffer;
    _ELEVATE_FROM_U8_NUM_BUFFER(writer_addr, buffer, write_len);
#endif
    // assert(check_unicode_writer_valid(unicode_buffer_info));
}

/*
 * Write a f64 number to the buffer.
 * The space (32 * sizeof(_dst_t)) must be reserved before calling this function.
 */
force_inline void f64_to_unicode(_dst_t **writer_addr, u64 val_u64_repr) {
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = *writer_addr; //_CAST_WRITER(unicode_buffer_info);
#else
    u8 _buffer[32];
    u8 *buffer = _buffer;
#endif
    u8 *buffer_end = dragonbox_to_chars_n(f64_from_raw(val_u64_repr), buffer);
    // u8 *buffer_end = buffer + d2s_buffered_n(f64_from_raw(val_u64_repr), (char *)buffer);
#if COMPILE_WRITE_UCS_LEVEL == 1
    *writer_addr = buffer_end;
    // unicode_buffer_info->writer.writer_u8 = buffer_end;
#else
    Py_ssize_t write_len = buffer_end - buffer;
    _ELEVATE_FROM_U8_NUM_BUFFER(writer_addr, buffer, write_len);
#endif
}

#include "compile_context/sw_out.inl.h"

#undef _ELEVATE_FROM_U8_NUM_BUFFER

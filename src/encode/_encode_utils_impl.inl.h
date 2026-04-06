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

#ifdef SSRJSON_CLANGD_CHECKING
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

#define _cvt_up_from_u8_cnt32 make_w_name(_cvt_up_from_u8_cnt32)
#define _cvt_up_from_u8_cnt24 make_w_name(_cvt_up_from_u8_cnt24)
#define _cvt_up_from_u8_cnt16 make_w_name(_cvt_up_from_u8_cnt16)
#define _cvt_up_from_u8_cnt8 make_w_name(_cvt_up_from_u8_cnt8)
#define _cvt_up_from_u8_cnt4 make_w_name(_cvt_up_from_u8_cnt4)

extern u8 *xjb64(double value, u8 *buffer);
extern u8 *xjb32(float value, u8 *buffer);
/*
 * (PRIVATE)
 * Convert the u8 buffer to the buffer.
 * The space (cnt * sizeof(dst_t)) must be reserved before calling this function.
 */
#if COMPILE_WRITE_UCS_LEVEL > 1
force_inline ssrjson_nofail void _cvt_up_from_u8_cnt32(dst_t *writer, const u8 *buffer) {
#    if COMPILE_WRITE_UCS_LEVEL == 2
    __partial_cvt_32_u8_u16(&writer, &buffer);
#    else // COMPILE_WRITE_UCS_LEVEL == 4
    __partial_cvt_32_u8_u32(&writer, &buffer);
#    endif
}

force_inline ssrjson_nofail void _cvt_up_from_u8_cnt24(dst_t *writer, const u8 *buffer) {
#    if COMPILE_WRITE_UCS_LEVEL == 2
    __partial_cvt_16_u8_u16(&writer, &buffer);
    __partial_cvt_8_u8_u16(&writer, &buffer);
#    else // COMPILE_WRITE_UCS_LEVEL == 4
    __partial_cvt_16_u8_u32(&writer, &buffer);
    __partial_cvt_8_u8_u32(&writer, &buffer);
#    endif
}

force_inline ssrjson_nofail void _cvt_up_from_u8_cnt16(dst_t *writer, const u8 *buffer) {
#    if COMPILE_WRITE_UCS_LEVEL == 2
    __partial_cvt_16_u8_u16(&writer, &buffer);
#    else // COMPILE_WRITE_UCS_LEVEL == 4
    __partial_cvt_16_u8_u32(&writer, &buffer);
#    endif
}

force_inline ssrjson_nofail void _cvt_up_from_u8_cnt8(dst_t *writer, const u8 *buffer) {
#    if COMPILE_WRITE_UCS_LEVEL == 2
    __partial_cvt_8_u8_u16(&writer, &buffer);
#    else // COMPILE_WRITE_UCS_LEVEL == 4
    __partial_cvt_8_u8_u32(&writer, &buffer);
#    endif
}

force_inline ssrjson_nofail void _cvt_up_from_u8_cnt4(dst_t *writer, const u8 *buffer) {
#    if COMPILE_WRITE_UCS_LEVEL == 2
    __partial_cvt_4_u8_u16(&writer, &buffer);
#    else // COMPILE_WRITE_UCS_LEVEL == 4
    __partial_cvt_4_u8_u32(&writer, &buffer);
#    endif
}
#endif

/*
 * Write a u64 number to the buffer.
 * The space (32 * sizeof(dst_t)) must be reserved before calling this function.
 */
force_inline ssrjson_nofail dst_t *u64_to_unicode(register dst_t *writer, u64 val, int sign) {
    assume(sign == 0 || sign == 1);
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = writer;
#else
    //require: 20 + 1
    ssrjson_align(32) u8 _buffer[32];
    u8 *buffer = _buffer;
#endif
    *buffer = '-';
    u8 *buffer_end = write_u64(val, buffer + sign);
#if COMPILE_WRITE_UCS_LEVEL == 1
    return buffer_end;
#else
    _cvt_up_from_u8_cnt32(writer, buffer);
    return writer + (buffer_end - buffer);
#endif
}

/*
 * Write a u32 number to the buffer.
 * The space (16 * sizeof(dst_t)) must be reserved before calling this function.
 */
force_inline ssrjson_nofail dst_t *u32_to_unicode(register dst_t *writer, u32 val, int sign) {
    assume(sign == 0 || sign == 1);
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = writer;
#else
    //require: 10 + 1
    ssrjson_align(16) u8 _buffer[16];
    u8 *buffer = _buffer;
#endif
    *buffer = '-';
    u8 *buffer_end = write_u32(val, buffer + sign);
#if COMPILE_WRITE_UCS_LEVEL == 1
    return buffer_end;
#else
    _cvt_up_from_u8_cnt16(writer, buffer);
    return writer + (buffer_end - buffer);
#endif
}

/*
 * Write a u16 number to the buffer.
 * The space (8 * sizeof(dst_t)) must be reserved before calling this function.
 */
force_inline ssrjson_nofail dst_t *u16_to_unicode(register dst_t *writer, u16 val, int sign) {
    assume(sign == 0 || sign == 1);
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = writer;
#else
    //require: 5 + 1
    ssrjson_align(8) u8 _buffer[8];
    u8 *buffer = _buffer;
#endif
    *buffer = '-';
    u8 *buffer_end = write_u16(val, buffer + sign);
#if COMPILE_WRITE_UCS_LEVEL == 1
    return buffer_end;
#else
    _cvt_up_from_u8_cnt8(writer, buffer);
    return writer + (buffer_end - buffer);
#endif
}

/*
 * Write a u8 number to the buffer.
 * The space (4 * sizeof(dst_t)) must be reserved before calling this function.
 */
force_inline ssrjson_nofail dst_t *u8_to_unicode(register dst_t *writer, u8 val, int sign) {
    assume(sign == 0 || sign == 1);
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = writer;
#else
    //require: 3 + 1
    ssrjson_align(4) u8 _buffer[4];
    u8 *buffer = _buffer;
#endif
    *buffer = '-';
    u8 *buffer_end = write_u8(val, buffer + sign);
#if COMPILE_WRITE_UCS_LEVEL == 1
    return buffer_end;
#else
    _cvt_up_from_u8_cnt4(writer, buffer);
    return writer + (buffer_end - buffer);
#endif
}

/*
 * Write a f64 number to the buffer.
 * The space (ssrjson_dtoa_write_length * sizeof(dst_t)) must be reserved before calling this function.
 */
force_inline ssrjson_nofail dst_t *f64_to_unicode(register dst_t *writer, double d) {
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = writer;
#else
    u8 _buffer[ssrjson_dtoa_write_length];
    u8 *buffer = _buffer;
#endif
    u8 *buffer_end = xjb64(d, buffer);
#if COMPILE_WRITE_UCS_LEVEL == 1
    return buffer_end;
#else
    usize write_len = buffer_end - buffer;
    assert(write_len <= ssrjson_dtoa_output_maxlen);
    _cvt_up_from_u8_cnt32(writer, buffer);
    return writer + write_len;
#endif
}

/*
 * Write a f32 number to the buffer.
 * The space (ssrjson_ftoa_write_length * sizeof(dst_t)) must be reserved before calling this function.
 */
force_inline ssrjson_nofail dst_t *f32_to_unicode(register dst_t *writer, float f) {
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = writer;
#else
    u8 _buffer[ssrjson_ftoa_write_length];
    u8 *buffer = _buffer;
#endif
    u8 *buffer_end = xjb32(f, buffer);
#if COMPILE_WRITE_UCS_LEVEL == 1
    return buffer_end;
#else
    usize write_len = buffer_end - buffer;
    assert(write_len <= ssrjson_ftoa_output_maxlen);
    _cvt_up_from_u8_cnt24(writer, buffer);
    return writer + write_len;
#endif
}

/* Intended for a dtoa that does not support Inf/NaN writing, like zmij (Rust impl) */
force_inline ssrjson_nofail dst_t *inf_nan_to_unicode(register dst_t *writer, double d) {
    if (isinf(d)) {
        bool sign = d < 0;
        *writer = '-';
        static const dst_t _Inf[8] = {'I', 'n', 'f', 'i', 'n', 'i', 't', 'y'};
        memcpy(writer + sign, _Inf, sizeof(_Inf));
        writer += sign + 8;
    } else {
        static const dst_t _NaN[4] = {'N', 'a', 'N', 0};
        memcpy(writer, _NaN, sizeof(_NaN));
        writer += 3;
    }
    return writer;
}

#include "compile_context/sw_out.inl.h"

#undef _cvt_up_from_u8_cnt32
#undef _cvt_up_from_u8_cnt24
#undef _cvt_up_from_u8_cnt16
#undef _cvt_up_from_u8_cnt8
#undef _cvt_up_from_u8_cnt4

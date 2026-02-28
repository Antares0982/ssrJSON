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

#define _elevate_u8_copy MAKE_W_NAME(_elevate_u8_copy)

extern u8 *xjb64(double value, u8 *buffer);
/*
 * (PRIVATE)
 * Convert the u8 buffer to the buffer.
 * The space (32 * sizeof(_dst_t)) must be reserved before calling this function.
 */
#if COMPILE_WRITE_UCS_LEVEL > 1
force_inline ssrjson_nofail void _elevate_u8_copy(_dst_t *writer, const u8 *buffer) {
#    if COMPILE_WRITE_UCS_LEVEL == 2
    __partial_cvt_32_u8_u16(&writer, &buffer);
#    else // COMPILE_WRITE_UCS_LEVEL == 4
    __partial_cvt_32_u8_u32(&writer, &buffer);
#    endif
}
#endif

/*
 * Write a u64 number to the buffer.
 * The space (32 * sizeof(_dst_t)) must be reserved before calling this function.
 */
force_inline ssrjson_nofail _dst_t *u64_to_unicode(register _dst_t *writer, u64 val, usize sign) {
    assert(sign <= 1);
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = writer;
#else
    u8 _buffer[64];
    u8 *buffer = _buffer;
#endif
    if (sign) *buffer = '-';
    u8 *buffer_end = write_u64(val, buffer + sign);
#if COMPILE_WRITE_UCS_LEVEL == 1
    return buffer_end;
#else
    Py_ssize_t write_len = buffer_end - buffer;
    _elevate_u8_copy(writer, buffer);
    return writer + write_len;
#endif
}

/*
 * Write a f64 number to the buffer.
 * The space (32 * sizeof(_dst_t)) must be reserved before calling this function.
 */
force_inline ssrjson_nofail _dst_t *f64_to_unicode(register _dst_t *writer, double d) {
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = writer;
#else
    u8 _buffer[32];
    u8 *buffer = _buffer;
#endif
    u8 *buffer_end = xjb64(d, buffer);
#if COMPILE_WRITE_UCS_LEVEL == 1
    return buffer_end;
#else
    Py_ssize_t write_len = buffer_end - buffer;
    _elevate_u8_copy(writer, buffer);
    return writer + write_len;
#endif
}

force_inline ssrjson_nofail _dst_t *inf_nan_to_unicode(register _dst_t *writer, double d) {
    if (isinf(d)) {
        bool sign = d < 0;
        *writer = '-';
        static const _dst_t _Inf[9] = {'I', 'n', 'f', 'i', 'n', 'i', 't', 'y', 0};
        memcpy(writer + sign, _Inf, sizeof(_Inf));
        writer += sign + 8;
    } else {
        static const _dst_t _NaN[4] = {'N', 'a', 'N', 0};
        memcpy(writer, _NaN, sizeof(_NaN));
        writer += 3;
    }
    return writer;
}

#include "compile_context/sw_out.inl.h"

#undef _elevate_u8_copy

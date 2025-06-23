#ifndef SSRJSON_DECODE_STR_COPY_TO_NEW_H
#define SSRJSON_DECODE_STR_COPY_TO_NEW_H
#include "decode/decode_shared.h"
#include "pythonlib.h"
#include "simd/long_cvt.h"
#include "simd/memcpy.h"
#include "utils/unicode.h"
//
#include "simd/compile_feature_check.h"
//
#include "compile_context/s_in.inl.h"

force_inline void copy_to_new_unicode_ucs1(void **dst_addr, PyObject *ret, bool need_cvt, const u8 *src, usize count, int kind) {
    u8 *dst = need_cvt ? PYUNICODE_ASCII_START(ret) : PYUNICODE_UCS1_START(ret);
    *dst_addr = dst;
    ssrjson_memcpy(dst, src, count);
}

force_inline void copy_to_new_unicode_ucs2(void **dst_addr, PyObject *ret, bool need_cvt, const u16 *src, usize count, int kind) {
    if (!need_cvt) {
        u16 *dst = PYUNICODE_UCS2_START(ret);
        *dst_addr = dst;
        ssrjson_memcpy(dst, src, count * 2);
    } else {
        u8 *dst = (kind == 0) ? PYUNICODE_ASCII_START(ret) : PYUNICODE_UCS1_START(ret);
        *dst_addr = dst;
        MAKE_S_NAME(long_cvt_u16_u8)(dst, src, count);
    }
}

force_inline void copy_to_new_unicode_ucs4(void **dst_addr, PyObject *ret, bool need_cvt, const u32 *src, usize count, int kind) {
    if (!need_cvt) {
        u32 *dst = PYUNICODE_UCS4_START(ret);
        *dst_addr = dst;
        ssrjson_memcpy(dst, src, count * 4);
    } else {
        if (kind <= 1) {
            u8 *dst = (kind == 0) ? PYUNICODE_ASCII_START(ret) : PYUNICODE_UCS1_START(ret);
            *dst_addr = dst;
            MAKE_S_NAME(long_cvt_u32_u8)(dst, src, count);
        } else {
            // this should be unlikely, use noinline version
            u16 *dst = PYUNICODE_UCS2_START(ret);
            *dst_addr = dst;
            long_cvt_noinline_u32_u16_interface(dst, src, count);
        }
    }
}

#undef COMPILE_SIMD_BITS
#include "compile_context/s_out.inl.h"

#endif // SSRJSON_DECODE_STR_COPY_TO_NEW_H

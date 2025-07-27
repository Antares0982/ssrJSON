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

#ifndef SSRJSON_SIMD_LONG_CVT_PART_BACK_CVT_H
#define SSRJSON_SIMD_LONG_CVT_PART_BACK_CVT_H

#ifdef SSRJSON_CLANGD_DUMMY
#    include "simd/simd_impl.h"
#    include "ssrjson.h"
#    ifndef COMPILE_SIMD_BITS
#        define COMPILE_SIMD_BITS 512
#    endif
#endif


// 1
force_inline void __partial_back_cvt_1_u8_u8(u8 **dst_addr, const u8 **src_addr) {
    *--(*dst_addr) = *--(*src_addr);
}

force_inline void __partial_back_cvt_1_u16_u8(u8 **dst_addr, const u16 **src_addr) {
    *--(*dst_addr) = *--(*src_addr);
}

force_inline void __partial_back_cvt_1_u32_u8(u8 **dst_addr, const u32 **src_addr) {
    *--(*dst_addr) = *--(*src_addr);
}

force_inline void __partial_back_cvt_1_u8_u16(u16 **dst_addr, const u8 **src_addr) {
    *--(*dst_addr) = *--(*src_addr);
}

force_inline void __partial_back_cvt_1_u16_u16(u16 **dst_addr, const u16 **src_addr) {
    *--(*dst_addr) = *--(*src_addr);
}

force_inline void __partial_back_cvt_1_u32_u16(u16 **dst_addr, const u32 **src_addr) {
    *--(*dst_addr) = *--(*src_addr);
}

force_inline void __partial_back_cvt_1_u8_u32(u32 **dst_addr, const u8 **src_addr) {
    *--(*dst_addr) = *--(*src_addr);
}

force_inline void __partial_back_cvt_1_u16_u32(u32 **dst_addr, const u16 **src_addr) {
    *--(*dst_addr) = *--(*src_addr);
}

force_inline void __partial_back_cvt_1_u32_u32(u32 **dst_addr, const u32 **src_addr) {
    *--(*dst_addr) = *--(*src_addr);
}

// 2
force_inline void __partial_back_cvt_2_u8_u8(u8 **dst_addr, const u8 **src_addr) {
    __partial_back_cvt_1_u8_u8(dst_addr, src_addr);
    __partial_back_cvt_1_u8_u8(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_2_u16_u8(u8 **dst_addr, const u16 **src_addr) {
    __partial_back_cvt_1_u16_u8(dst_addr, src_addr);
    __partial_back_cvt_1_u16_u8(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_2_u32_u8(u8 **dst_addr, const u32 **src_addr) {
    __partial_back_cvt_1_u32_u8(dst_addr, src_addr);
    __partial_back_cvt_1_u32_u8(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_2_u8_u16(u16 **dst_addr, const u8 **src_addr) {
    __partial_back_cvt_1_u8_u16(dst_addr, src_addr);
    __partial_back_cvt_1_u8_u16(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_2_u16_u16(u16 **dst_addr, const u16 **src_addr) {
    __partial_back_cvt_1_u16_u16(dst_addr, src_addr);
    __partial_back_cvt_1_u16_u16(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_2_u32_u16(u16 **dst_addr, const u32 **src_addr) {
    __partial_back_cvt_1_u32_u16(dst_addr, src_addr);
    __partial_back_cvt_1_u32_u16(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_2_u8_u32(u32 **dst_addr, const u8 **src_addr) {
    __partial_back_cvt_1_u8_u32(dst_addr, src_addr);
    __partial_back_cvt_1_u8_u32(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_2_u16_u32(u32 **dst_addr, const u16 **src_addr) {
    __partial_back_cvt_1_u16_u32(dst_addr, src_addr);
    __partial_back_cvt_1_u16_u32(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_2_u32_u32(u32 **dst_addr, const u32 **src_addr) {
    __partial_back_cvt_1_u32_u32(dst_addr, src_addr);
    __partial_back_cvt_1_u32_u32(dst_addr, src_addr);
}

// 4
force_inline void __partial_back_cvt_4_u8_u8(u8 **dst_addr, const u8 **src_addr) {
    __partial_back_cvt_2_u8_u8(dst_addr, src_addr);
    __partial_back_cvt_2_u8_u8(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_4_u16_u8(u8 **dst_addr, const u16 **src_addr) {
    __partial_back_cvt_2_u16_u8(dst_addr, src_addr);
    __partial_back_cvt_2_u16_u8(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_4_u32_u8(u8 **dst_addr, const u32 **src_addr) {
    cvt_to_dst_u32_u8_128(*dst_addr, *(vector_u_u32_128 *)*src_addr);
    *dst_addr -= 4;
    *src_addr -= 4;
}

force_inline void __partial_back_cvt_4_u8_u16(u16 **dst_addr, const u8 **src_addr) {
    __partial_back_cvt_2_u8_u16(dst_addr, src_addr);
    __partial_back_cvt_2_u8_u16(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_4_u16_u16(u16 **dst_addr, const u16 **src_addr) {
    __partial_back_cvt_2_u16_u16(dst_addr, src_addr);
    __partial_back_cvt_2_u16_u16(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_4_u32_u16(u16 **dst_addr, const u32 **src_addr) {
    *dst_addr -= 4;
    *src_addr -= 4;
    cvt_to_dst_u32_u16_128(*dst_addr, *(vector_u_u32_128 *)*src_addr);
}

force_inline void __partial_back_cvt_4_u8_u32(u32 **dst_addr, const u8 **src_addr) {
    *dst_addr -= 4;
    *src_addr -= 4;
    vector_a_u8_128 x;
    memcpy(&x, *src_addr, 4);
    *(vector_u_u32_128 *)(*dst_addr) = cvt_u8_to_u32_128(x);
}

force_inline void __partial_back_cvt_4_u16_u32(u32 **dst_addr, const u16 **src_addr) {
    *dst_addr -= 4;
    *src_addr -= 4;
    vector_a_u16_128 x;
    memcpy(&x, *src_addr, 8);
    *(vector_u_u32_128 *)(*dst_addr) = cvt_u16_to_u32_128(x);
}

force_inline void __partial_back_cvt_4_u32_u32(u32 **dst_addr, const u32 **src_addr) {
    __partial_back_cvt_2_u32_u32(dst_addr, src_addr);
    __partial_back_cvt_2_u32_u32(dst_addr, src_addr);
}

// 8
force_inline void __partial_back_cvt_8_u8_u8(u8 **dst_addr, const u8 **src_addr) {
    __partial_back_cvt_4_u8_u8(dst_addr, src_addr);
    __partial_back_cvt_4_u8_u8(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_8_u16_u8(u8 **dst_addr, const u16 **src_addr) {
    *dst_addr -= 8;
    *src_addr -= 8;
    cvt_to_dst_u16_u8_128(*dst_addr, *(vector_u_u16_128 *)*src_addr);
}

force_inline void __partial_back_cvt_8_u32_u8(u8 **dst_addr, const u32 **src_addr) {
#if COMPILE_SIMD_BITS >= 256
    *dst_addr -= 8;
    *src_addr -= 8;
    vector_a_u32_256 x = *(vector_u_u32_256 *)*src_addr;
    cvt_to_dst_u32_u8_256(*dst_addr, x);
#else
    __partial_back_cvt_4_u32_u8(dst_addr, src_addr);
    __partial_back_cvt_4_u32_u8(dst_addr, src_addr);
#endif
}

force_inline void __partial_back_cvt_8_u8_u16(u16 **dst_addr, const u8 **src_addr) {
    *dst_addr -= 8;
    *src_addr -= 8;
    vector_a_u8_128 x;
    memcpy(&x, *src_addr, 8);
    *(vector_u_u16_128 *)(*dst_addr) = cvt_u8_to_u16_128(x);
}

force_inline void __partial_back_cvt_8_u16_u16(u16 **dst_addr, const u16 **src_addr) {
    __partial_back_cvt_4_u16_u16(dst_addr, src_addr);
    __partial_back_cvt_4_u16_u16(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_8_u32_u16(u16 **dst_addr, const u32 **src_addr) {
#if COMPILE_SIMD_BITS >= 256
    *dst_addr -= 8;
    *src_addr -= 8;
    cvt_to_dst_u32_u16_256(*dst_addr, *(vector_u_u32_256 *)*src_addr);
#else
    __partial_back_cvt_4_u32_u16(dst_addr, src_addr);
    __partial_back_cvt_4_u32_u16(dst_addr, src_addr);
#endif
}

force_inline void __partial_back_cvt_8_u8_u32(u32 **dst_addr, const u8 **src_addr) {
#if COMPILE_SIMD_BITS >= 256
    *dst_addr -= 8;
    *src_addr -= 8;
    vector_a_u8_128 x;
    memcpy(&x, *src_addr, 8);
    *(vector_u_u32_256 *)*dst_addr = cvt_u8_to_u32_256(x);
#else
    __partial_back_cvt_4_u8_u32(dst_addr, src_addr);
    __partial_back_cvt_4_u8_u32(dst_addr, src_addr);
#endif
}

force_inline void __partial_back_cvt_8_u16_u32(u32 **dst_addr, const u16 **src_addr) {
#if COMPILE_SIMD_BITS >= 256
    *dst_addr -= 8;
    *src_addr -= 8;
    *(vector_u_u32_256 *)*dst_addr = cvt_u16_to_u32_256(*(vector_u_u16_128 *)*src_addr);
#else
    __partial_back_cvt_4_u16_u32(dst_addr, src_addr);
    __partial_back_cvt_4_u16_u32(dst_addr, src_addr);
#endif
}

force_inline void __partial_back_cvt_8_u32_u32(u32 **dst_addr, const u32 **src_addr) {
    __partial_back_cvt_4_u32_u32(dst_addr, src_addr);
    __partial_back_cvt_4_u32_u32(dst_addr, src_addr);
}

// 16
force_inline void __partial_back_cvt_16_u8_u8(u8 **dst_addr, const u8 **src_addr) {
    __partial_back_cvt_8_u8_u8(dst_addr, src_addr);
    __partial_back_cvt_8_u8_u8(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_16_u16_u8(u8 **dst_addr, const u16 **src_addr) {
#if COMPILE_SIMD_BITS >= 256
    *dst_addr -= 16;
    *src_addr -= 16;
    cvt_to_dst_u16_u8_256(*dst_addr, *(vector_u_u16_256 *)*src_addr);

#else
    __partial_back_cvt_8_u16_u8(dst_addr, src_addr);
    __partial_back_cvt_8_u16_u8(dst_addr, src_addr);
#endif
}

force_inline void __partial_back_cvt_16_u32_u8(u8 **dst_addr, const u32 **src_addr) {
#if COMPILE_SIMD_BITS >= 512
    *dst_addr -= 16;
    *src_addr -= 16;
    cvt_to_dst_u32_u8_512(*dst_addr, *(vector_u_u16_512 *)*src_addr);
#else
    __partial_back_cvt_8_u32_u8(dst_addr, src_addr);
    __partial_back_cvt_8_u32_u8(dst_addr, src_addr);
#endif
}

force_inline void __partial_back_cvt_16_u8_u16(u16 **dst_addr, const u8 **src_addr) {
#if COMPILE_SIMD_BITS >= 256
    *dst_addr -= 16;
    *src_addr -= 16;
    *(vector_u_u16_256 *)*dst_addr = cvt_u8_to_u16_256(*(vector_u_u8_128 *)*src_addr);
#else
    __partial_back_cvt_8_u8_u16(dst_addr, src_addr);
    __partial_back_cvt_8_u8_u16(dst_addr, src_addr);
#endif
}

force_inline void __partial_back_cvt_16_u16_u16(u16 **dst_addr, const u16 **src_addr) {
    __partial_back_cvt_8_u16_u16(dst_addr, src_addr);
    __partial_back_cvt_8_u16_u16(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_16_u32_u16(u16 **dst_addr, const u32 **src_addr) {
#if COMPILE_SIMD_BITS >= 512
    *dst_addr -= 16;
    *src_addr -= 16;
    cvt_to_dst_u32_u16_512(*dst_addr, *(vector_u_u32_512 *)*src_addr);
#else
    __partial_back_cvt_8_u32_u16(dst_addr, src_addr);
    __partial_back_cvt_8_u32_u16(dst_addr, src_addr);
#endif
}

force_inline void __partial_back_cvt_16_u8_u32(u32 **dst_addr, const u8 **src_addr) {
#if COMPILE_SIMD_BITS >= 512
    *dst_addr -= 16;
    *src_addr -= 16;
    *(vector_u_u32_512 *)*dst_addr = cvt_u8_to_u32_512(*(vector_u_u8_128 *)*src_addr);
#else
    __partial_back_cvt_8_u8_u32(dst_addr, src_addr);
    __partial_back_cvt_8_u8_u32(dst_addr, src_addr);
#endif
}

force_inline void __partial_back_cvt_16_u16_u32(u32 **dst_addr, const u16 **src_addr) {
#if COMPILE_SIMD_BITS >= 512
    *dst_addr -= 16;
    *src_addr -= 16;
    *(vector_u_u32_512 *)*dst_addr = cvt_u16_to_u32_512(*(vector_u_u16_256 *)*src_addr);
#else
    __partial_back_cvt_8_u16_u32(dst_addr, src_addr);
    __partial_back_cvt_8_u16_u32(dst_addr, src_addr);
#endif
}

force_inline void __partial_back_cvt_16_u32_u32(u32 **dst_addr, const u32 **src_addr) {
    __partial_back_cvt_8_u32_u32(dst_addr, src_addr);
    __partial_back_cvt_8_u32_u32(dst_addr, src_addr);
}

// 32
force_inline void __partial_back_cvt_32_u8_u8(u8 **dst_addr, const u8 **src_addr) {
    __partial_back_cvt_16_u8_u8(dst_addr, src_addr);
    __partial_back_cvt_16_u8_u8(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_32_u16_u8(u8 **dst_addr, const u16 **src_addr) {
#if COMPILE_SIMD_BITS >= 512
    *dst_addr -= 32;
    *src_addr -= 32;
    cvt_to_dst_u16_u8_512(*dst_addr, *(vector_u_u16_512 *)*src_addr);
#else
    __partial_back_cvt_16_u16_u8(dst_addr, src_addr);
    __partial_back_cvt_16_u16_u8(dst_addr, src_addr);
#endif
}

force_inline void __partial_back_cvt_32_u32_u8(u8 **dst_addr, const u32 **src_addr) {
    __partial_back_cvt_16_u32_u8(dst_addr, src_addr);
    __partial_back_cvt_16_u32_u8(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_32_u8_u16(u16 **dst_addr, const u8 **src_addr) {
#if COMPILE_SIMD_BITS >= 512
    *dst_addr -= 32;
    *src_addr -= 32;
    *(vector_u_u16_512 *)*dst_addr = cvt_u8_to_u16_512(*(vector_u_u8_256 *)*src_addr);
#else
    __partial_back_cvt_16_u8_u16(dst_addr, src_addr);
    __partial_back_cvt_16_u8_u16(dst_addr, src_addr);
#endif
}

force_inline void __partial_back_cvt_32_u16_u16(u16 **dst_addr, const u16 **src_addr) {
    __partial_back_cvt_16_u16_u16(dst_addr, src_addr);
    __partial_back_cvt_16_u16_u16(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_32_u32_u16(u16 **dst_addr, const u32 **src_addr) {
    __partial_back_cvt_16_u32_u16(dst_addr, src_addr);
    __partial_back_cvt_16_u32_u16(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_32_u8_u32(u32 **dst_addr, const u8 **src_addr) {
    __partial_back_cvt_16_u8_u32(dst_addr, src_addr);
    __partial_back_cvt_16_u8_u32(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_32_u16_u32(u32 **dst_addr, const u16 **src_addr) {
    __partial_back_cvt_16_u16_u32(dst_addr, src_addr);
    __partial_back_cvt_16_u16_u32(dst_addr, src_addr);
}

force_inline void __partial_back_cvt_32_u32_u32(u32 **dst_addr, const u32 **src_addr) {
    __partial_back_cvt_16_u32_u32(dst_addr, src_addr);
    __partial_back_cvt_16_u32_u32(dst_addr, src_addr);
}
#endif // SSRJSON_SIMD_LONG_CVT_PART_BACK_CVT_H

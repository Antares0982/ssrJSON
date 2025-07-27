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

#ifndef SSRJSON_MEMCMP_H
#define SSRJSON_MEMCMP_H

#include "simd_impl.h"

force_inline bool __memcmp_neq_short(u8 **x_addr, u8 **y_addr, usize *size_addr, usize small_cmp_size) {
    if (*size_addr >= small_cmp_size) {
        if (memcmp(*x_addr, *y_addr, small_cmp_size)) return true;
        *size_addr -= small_cmp_size;
        *x_addr += small_cmp_size;
        *y_addr += small_cmp_size;
    }
    return false;
}

/* Compare memory blocks smaller than (or equal to) 64 bytes.
 * Return non-zero if not equal (be compatible with memcmp().) */
force_inline int ssrjson_memcmp_neq_le64(u8 *x, u8 *y, usize size) {
    assert(size <= 64);
#if COMPILE_SIMD_BITS == 512
    if (size == 64) {
        return memcmp(x, y, 64) ? 1 : 0;
    }
#endif
    if (size >= 32) {
        if (memcmp(x, y, 32)) return 1;
        x += 32;
        y += 32;
#if COMPILE_SIMD_BITS < 512
        if (size == 64) return memcmp(x, y, 32) ? 1 : 0;
#endif
        size -= 32;
    }
    if (__memcmp_neq_short(&x, &y, &size, 16)) return 1;
    if (__memcmp_neq_short(&x, &y, &size, 8)) return 1;
    if (__memcmp_neq_short(&x, &y, &size, 4)) return 1;
    if (__memcmp_neq_short(&x, &y, &size, 2)) return 1;
    if (__memcmp_neq_short(&x, &y, &size, 1)) return 1;
    return 0;
}

#endif // SSRJSON_MEMCMP_H

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

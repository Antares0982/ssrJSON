#ifndef SIMD_MASK_TABLE_H
#define SIMD_MASK_TABLE_H

#include "ssrjson.h"


extern ssrjson_align(64) const u8 _TailmaskTable_8[65][64];
extern ssrjson_align(64) const u8 _HeadmaskTable_8[65][64];
// extern ssrjson_align(64) const u8 _TailmaskReversedTable_8[65][64];
extern ssrjson_align(64) const u8 _RShiftMaskTable[16][16];

/*==============================================================================
 * Read mask from tail mask table.
 * `read_tail_mask_table_x` gives `row` zeroes in the front of the mask.
 *============================================================================*/

/* Read tail mask of u8. The result has `row` zeros at head. */
force_inline const void *read_tail_mask_table_8(Py_ssize_t row) {
    return (const void *)&_TailmaskTable_8[row][0];
}

/* Read tail mask of u16. The result has `row` zeros at head. */
force_inline const void *read_tail_mask_table_16(Py_ssize_t row) {
    return (const void *)&_TailmaskTable_8[2 * row][0];
}

/* Read tail mask of u32. The result has `row` zeros at head. */
force_inline const void *read_tail_mask_table_32(Py_ssize_t row) {
    return (const void *)&_TailmaskTable_8[4 * row][0];
}

/* Read head mask of u8. The result has `row` "0xff"s at head. */
force_inline const void *read_head_mask_table_8(Py_ssize_t row) {
    return (const void *)&_HeadmaskTable_8[row][0];
}

/* Read head mask of u16. The result has `row` "0xffff"s at head. */
force_inline const void *read_head_mask_table_16(Py_ssize_t row) {
    return (const void *)&_HeadmaskTable_8[2 * row][0];
}

/* Read head mask of u32. The result has `row` "0xffffffff"s at head. */
force_inline const void *read_head_mask_table_32(Py_ssize_t row) {
    return (const void *)&_HeadmaskTable_8[4 * row][0];
}

// /* Read tail mask of u8. The result has `row` "0xff"s at tail. */
// force_inline const void *read_tail_mask_reversed_table_8(Py_ssize_t row) {
// #if COMPILE_SIMD_BITS == 512
//     const int offset = 64;
// #elif COMPILE_SIMD_BITS == 256
//     const int offset = 32;
// #else
//     const int offset = 16;
// #endif
//     return (const void *)&_TailmaskTable_8[offset - row][0];
// }

// /* Read tail mask of u16. The result has `row` "0xffff"s at tail. */
// force_inline const void *read_tail_mask_reversed_table_16(Py_ssize_t row) {
// #if COMPILE_SIMD_BITS == 512
//     const int offset = 64;
// #elif COMPILE_SIMD_BITS == 256
//     const int offset = 32;
// #else
//     const int offset = 16;
// #endif
//     assert(offset >= 2 * row && row >= 0);
//     return (const void *)&_TailmaskTable_8[offset - 2 * row][0];
// }

// /* Read tail mask of u32. The result has `row` "0xffffffff"s at tail. */
// force_inline const void *read_tail_mask_reversed_table_32(Py_ssize_t row) {
// #if COMPILE_SIMD_BITS == 512
//     const int offset = 64;
// #elif COMPILE_SIMD_BITS == 256
//     const int offset = 32;
// #else
//     const int offset = 16;
// #endif
//     assert(offset >= 4 * row && row >= 0);
//     return (const void *)&_TailmaskTable_8[4 * row][0];
// }

force_inline const void *byte_rshift_mask_table(int row) {
    return (const void *)&_RShiftMaskTable[row][0];
}

#endif // SIMD_MASK_TABLE_H

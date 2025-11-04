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

#ifndef SIMD_MASK_TABLE_H
#define SIMD_MASK_TABLE_H

#include "ssrjson.h"


extern ssrjson_align(64) const u8 _TailmaskTable_8[65][64];
extern ssrjson_align(64) const u8 _HeadmaskTable_8[65][64];
extern ssrjson_align(64) const u8 _RShiftMaskTable[17][16];
extern ssrjson_align(16) const u8 _AVX2TrailingCvtRShiftMaskTable32to8[4][16];
extern ssrjson_align(16) const u64 _LenToMaskZTable[64];

/*==============================================================================
 * Read mask from tail mask table.
 * `read_tail_mask_table_x` gives `row` zeroes in the front of the mask.
 *============================================================================*/

/* Read tail mask of u8. The result has `row` zeros at head. */
force_inline const void *read_tail_mask_table_8(Py_ssize_t row) {
    return (const void *)&_TailmaskTable_8[row][0];
}

/* Read tail mask of u16. The result has `row` zeros at head. */
// force_inline const void *read_tail_mask_table_16(Py_ssize_t row) {
//     return (const void *)&_TailmaskTable_8[2 * row][0];
// }

/* Read tail mask of u32. The result has `row` zeros at head. */
// force_inline const void *read_tail_mask_table_32(Py_ssize_t row) {
//     return (const void *)&_TailmaskTable_8[4 * row][0];
// }

/* Read head mask of u8. The result has `row` "0xff"s at head. */
force_inline const void *read_head_mask_table_8(Py_ssize_t row) {
    return (const void *)&_HeadmaskTable_8[row][0];
}

/* Read head mask of u16. The result has `row` "0xffff"s at head. */
// force_inline const void *read_head_mask_table_16(Py_ssize_t row) {
//     return (const void *)&_HeadmaskTable_8[2 * row][0];
// }

/* Read head mask of u32. The result has `row` "0xffffffff"s at head. */
// force_inline const void *read_head_mask_table_32(Py_ssize_t row) {
//     return (const void *)&_HeadmaskTable_8[4 * row][0];
// }

force_inline const void *byte_rshift_mask_table(int row) {
    return (const void *)&_RShiftMaskTable[row][0];
}

#endif // SIMD_MASK_TABLE_H

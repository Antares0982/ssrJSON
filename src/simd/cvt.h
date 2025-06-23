#ifndef SIMD_CVT_H
#define SIMD_CVT_H

#include "simd/mask_table.h"
#include "simd_detect.h"
#include "utils/unicode.h"


void SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u16)(u16 *write_start, const u8 *read_start, usize len);


void SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u32)(u32 *write_start, const u8 *read_start, usize len);


void SIMD_NAME_MODIFIER(long_back_cvt_noinline_u16_u32)(u32 *write_start, const u16 *read_start, usize len);


void SIMD_NAME_MODIFIER(long_cvt_noinline_u8_u16)(u16 *restrict write_start, const u8 *restrict read_start, usize _len);


void SIMD_NAME_MODIFIER(long_cvt_noinline_u8_u32)(u32 *restrict write_start, const u8 *restrict read_start, usize _len);


void SIMD_NAME_MODIFIER(long_cvt_noinline_u16_u32)(u32 *restrict write_start, const u16 *restrict read_start, usize _len);


void SIMD_NAME_MODIFIER(long_cvt_noinline_u32_u16)(u16 *restrict write_start, const u32 *restrict read_start, usize _len);


void SIMD_NAME_MODIFIER(long_cvt_noinline_u32_u8)(u8 *restrict write_start, const u32 *restrict read_start, usize _len);


void SIMD_NAME_MODIFIER(long_cvt_noinline_u16_u8)(u8 *restrict write_start, const u16 *restrict read_start, usize _len);

#endif // SIMD_CVT_H

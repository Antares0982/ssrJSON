#include "cvt.h"
#include "long_cvt.h"
//
#include "compile_feature_check.h"
//
#include "compile_context/s_in.inl.h"

PyAPI_FUNC(void) SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u16)(u16 *write_start, const u8 *read_start, usize _len) {
    usize len = _len;
    const u8 *read_end = read_start + len;
    u16 *write_end = write_start + len;
    MAKE_S_NAME(long_back_cvt_u8_u16)(write_end, read_end, len);
}

PyAPI_FUNC(void) SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u32)(u32 *write_start, const u8 *read_start, usize _len) {
    usize len = _len;
    const u8 *read_end = read_start + len;
    u32 *write_end = write_start + len;
    MAKE_S_NAME(long_back_cvt_u8_u32)(write_end, read_end, len);
}

PyAPI_FUNC(void) SIMD_NAME_MODIFIER(long_back_cvt_noinline_u16_u32)(u32 *write_start, const u16 *read_start, usize _len) {
    usize len = _len;
    const u16 *read_end = read_start + len;
    u32 *write_end = write_start + len;
    MAKE_S_NAME(long_back_cvt_u16_u32)(write_end, read_end, len);
}

PyAPI_FUNC(void) SIMD_NAME_MODIFIER(long_cvt_noinline_u8_u16)(u16 *restrict write_start, const u8 *restrict read_start, usize _len) {
    usize len = _len;
    MAKE_S_NAME(long_cvt_u8_u16)(write_start, read_start, len);
}

PyAPI_FUNC(void) SIMD_NAME_MODIFIER(long_cvt_noinline_u8_u32)(u32 *restrict write_start, const u8 *restrict read_start, usize _len) {
    usize len = _len;
    MAKE_S_NAME(long_cvt_u8_u32)(write_start, read_start, len);
}

PyAPI_FUNC(void) SIMD_NAME_MODIFIER(long_cvt_noinline_u16_u32)(u32 *restrict write_start, const u16 *restrict read_start, usize _len) {
    usize len = _len;
    MAKE_S_NAME(long_cvt_u16_u32)(write_start, read_start, len);
}

PyAPI_FUNC(void) SIMD_NAME_MODIFIER(long_cvt_noinline_u32_u16)(u16 *restrict write_start, const u32 *restrict read_start, usize _len) {
    usize len = _len;
    MAKE_S_NAME(long_cvt_u32_u16)(write_start, read_start, len);
}

PyAPI_FUNC(void) SIMD_NAME_MODIFIER(long_cvt_noinline_u32_u8)(u8 *restrict write_start, const u32 *restrict read_start, usize _len) {
    usize len = _len;
    MAKE_S_NAME(long_cvt_u32_u8)(write_start, read_start, len);
}

PyAPI_FUNC(void) SIMD_NAME_MODIFIER(long_cvt_noinline_u16_u8)(u8 *restrict write_start, const u16 *restrict read_start, usize _len) {
    usize len = _len;
    MAKE_S_NAME(long_cvt_u16_u8)(write_start, read_start, len);
}

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

#ifndef SSRJSON_PYTHONLIB_H
#define SSRJSON_PYTHONLIB_H

#include "ssrjson.h"
#if SSRJSON_X86

typedef enum X86SIMDFeatureLevel {
    X86SIMDFeatureLevelSSE2 = 0,
    X86SIMDFeatureLevelSSE4_2 = 1,
    X86SIMDFeatureLevelAVX2 = 2,
    X86SIMDFeatureLevelAVX512 = 3,
    X86SIMDFeatureLevelMAX = 4,
} X86SIMDFeatureLevel;

#    define PLATFORM_SIMD_LEVEL X86SIMDFeatureLevel

force_inline X86SIMDFeatureLevel get_simd_feature(void) {
    // https://www.intel.com/content/dam/develop/external/us/en/documents/319433-024-697869.pdf

    int max_leaf = get_cpuid_max();

    if (max_leaf >= 7) {
        int info[4] = {0};
        cpuid_count(info, 7, 0);
        int ebx = info[1];
        if ((ebx & (1 << 16))    // AVX512F(ebx,16)
            && (ebx & (1 << 28)) // AVX512CD(ebx,28)
            && (ebx & (1 << 31)) // AVX512VL(ebx,31)
            && (ebx & (1 << 17)) // AVX512DQ(ebx,17)
            && (ebx & (1 << 30)) // AVX512BW(ebx,30)
        )
            return X86SIMDFeatureLevelAVX512;

        // check AVX2
        if (ebx & (1 << 5)) // AVX2(ebx,5)
            return X86SIMDFeatureLevelAVX2;
    }

    // check SSE4.2
    if (max_leaf >= 1) {
        int info[4] = {0};
        cpuid(info, 1);
        int ecx = info[2];
        if (ecx & (1 << 20)) // SSE4.2(20)
            return X86SIMDFeatureLevelSSE4_2;
    }

    return X86SIMDFeatureLevelSSE2;
}
#elif SSRJSON_AARCH

typedef enum AARCH64SIMDFeatureLevel {
    AARCH64SIMDFeatureLevelNEON = 0,
} AARCH64SIMDFeatureLevel;

#    define PLATFORM_SIMD_LEVEL AARCH64SIMDFeatureLevelNEON

force_inline AARCH64SIMDFeatureLevel get_simd_feature(void) {
    return AARCH64SIMDFeatureLevelNEON;
}
#endif

const char *_update_simd_features(void);
PyObject *ssrjson_get_current_features(PyObject *self, PyObject *args);

#if BUILD_MULTI_LIB


#    define MAKE_FORWARD_PYFASTFUNCTION_IMPL(_func_name_)                                                    \
        PyObject *_func_name_(PyObject *self, PyObject *const *args, Py_ssize_t nargsf, PyObject *kwnames) { \
            assert(SSRJSON_CONCAT2(_func_name_, interface));                                                 \
            return SSRJSON_CONCAT2(_func_name_, interface)(self, args, nargsf, kwnames);                     \
        }

#    define SET_INTERFACE(_func_name_, _feature_name_) SSRJSON_CONCAT2(_func_name_, interface) = SSRJSON_CONCAT2(_func_name_, _feature_name_)

#    define BATCH_SET_INTERFACE(_feature_name_)                   \
        SET_INTERFACE(ssrjson_Encode, _feature_name_);            \
        SET_INTERFACE(ssrjson_Decode, _feature_name_);            \
        SET_INTERFACE(ssrjson_EncodeToBytes, _feature_name_);     \
        SET_INTERFACE(long_cvt_noinline_u16_u32, _feature_name_); \
        SET_INTERFACE(long_cvt_noinline_u8_u32, _feature_name_);  \
        SET_INTERFACE(long_cvt_noinline_u8_u16, _feature_name_);  \
        SET_INTERFACE(long_cvt_noinline_u32_u16, _feature_name_); \
        SET_INTERFACE(long_cvt_noinline_u32_u8, _feature_name_);  \
        SET_INTERFACE(long_cvt_noinline_u16_u8, _feature_name_);
#    if SSRJSON_X86
#        define DECLARE_MULTILIB_PYFASTFUNCTION(_func_name_)                                                                                    \
            PyObject *SSRJSON_CONCAT2(_func_name_, avx512)(PyObject * self, PyObject *const *args, Py_ssize_t nargsf, PyObject *kwnames);       \
            PyObject *SSRJSON_CONCAT2(_func_name_, avx2)(PyObject * self, PyObject *const *args, Py_ssize_t nargsf, PyObject *kwnames);         \
            PyObject *SSRJSON_CONCAT2(_func_name_, sse4_2)(PyObject * self, PyObject *const *args, Py_ssize_t nargsf, PyObject *kwnames);       \
            typedef PyObject *(*SSRJSON_CONCAT2(_func_name_, t))(PyObject * self, PyObject *const *args, Py_ssize_t nargsf, PyObject *kwnames); \
            extern SSRJSON_CONCAT2(_func_name_, t) SSRJSON_CONCAT2(_func_name_, interface);
#        define DECLARE_MULTILIB_ANYFUNCTION(_func_name_, _ret_type_, ...)      \
            _ret_type_ SSRJSON_CONCAT2(_func_name_, avx512)(__VA_ARGS__);       \
            _ret_type_ SSRJSON_CONCAT2(_func_name_, avx2)(__VA_ARGS__);         \
            _ret_type_ SSRJSON_CONCAT2(_func_name_, sse4_2)(__VA_ARGS__);       \
            typedef _ret_type_ (*SSRJSON_CONCAT2(_func_name_, t))(__VA_ARGS__); \
            extern SSRJSON_CONCAT2(_func_name_, t) SSRJSON_CONCAT2(_func_name_, interface);

#    elif SSRJSON_AARCH
#        define DECLARE_MULTILIB_PYFASTFUNCTION(_func_name_)                                                                                    \
            PyObject *SSRJSON_CONCAT2(_func_name_, neon)(PyObject * self, PyObject *const *args, Py_ssize_t nargsf, PyObject *kwnames);         \
            typedef PyObject *(*SSRJSON_CONCAT2(_func_name_, t))(PyObject * self, PyObject *const *args, Py_ssize_t nargsf, PyObject *kwnames); \
            extern SSRJSON_CONCAT2(_func_name_, t) SSRJSON_CONCAT2(_func_name_, interface);
#        define DECLARE_MULTILIB_ANYFUNCTION(_func_name_, _ret_type_, ...)      \
            _ret_type_ SSRJSON_CONCAT2(_func_name_, neon)(__VA_ARGS__);         \
            typedef _ret_type_ (*SSRJSON_CONCAT2(_func_name_, t))(__VA_ARGS__); \
            extern SSRJSON_CONCAT2(_func_name_, t) SSRJSON_CONCAT2(_func_name_, interface);
// #        define long_cvt_noinline_u16_u32_interface long_cvt_noinline_u16_u32_neon
// #        define long_cvt_noinline_u8_u32_interface long_cvt_noinline_u8_u32_neon
// #        define long_cvt_noinline_u8_u16_interface long_cvt_noinline_u8_u16_neon
// #        define long_cvt_noinline_u32_u16_interface long_cvt_noinline_u32_u16_neon
// #        define long_cvt_noinline_u32_u8_interface long_cvt_noinline_u32_u8_neon
// #        define long_cvt_noinline_u16_u8_interface long_cvt_noinline_u16_u8_neon
#    endif // SSRJSON_X86


DECLARE_MULTILIB_PYFASTFUNCTION(ssrjson_Encode)
DECLARE_MULTILIB_PYFASTFUNCTION(ssrjson_Decode)
DECLARE_MULTILIB_PYFASTFUNCTION(ssrjson_EncodeToBytes)
DECLARE_MULTILIB_ANYFUNCTION(long_cvt_noinline_u16_u32, void, u32 *restrict write_start, const u16 *restrict read_start, usize _len)
DECLARE_MULTILIB_ANYFUNCTION(long_cvt_noinline_u8_u32, void, u32 *restrict write_start, const u8 *restrict read_start, usize _len)
DECLARE_MULTILIB_ANYFUNCTION(long_cvt_noinline_u8_u16, void, u16 *restrict write_start, const u8 *restrict read_start, usize _len)
DECLARE_MULTILIB_ANYFUNCTION(long_cvt_noinline_u32_u16, void, u16 *restrict write_start, const u32 *restrict read_start, usize _len)
DECLARE_MULTILIB_ANYFUNCTION(long_cvt_noinline_u32_u8, void, u8 *restrict write_start, const u32 *restrict read_start, usize _len)
DECLARE_MULTILIB_ANYFUNCTION(long_cvt_noinline_u16_u8, void, u8 *restrict write_start, const u16 *restrict read_start, usize _len)


#else // BUILD_MULTI_LIB
#    define long_cvt_noinline_u16_u32_interface long_cvt_noinline_u16_u32
#    define long_cvt_noinline_u8_u32_interface long_cvt_noinline_u8_u32
#    define long_cvt_noinline_u8_u16_interface long_cvt_noinline_u8_u16
#    define long_cvt_noinline_u32_u16_interface long_cvt_noinline_u32_u16
#    define long_cvt_noinline_u32_u8_interface long_cvt_noinline_u32_u8
#    define long_cvt_noinline_u16_u8_interface long_cvt_noinline_u16_u8
#endif
#endif // SSRJSON_PYTHONLIB_H

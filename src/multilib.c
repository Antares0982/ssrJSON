#include "pythonlib.h"
#include "ssrjson.h"


#if SSRJSON_X86
IMPL_MULTILIB_FUNCTION_INTERFACE(ssrjson_Encode)
IMPL_MULTILIB_FUNCTION_INTERFACE(ssrjson_Decode)
IMPL_MULTILIB_FUNCTION_INTERFACE(ssrjson_EncodeToBytes)
IMPL_MULTILIB_FUNCTION_INTERFACE(long_cvt_noinline_u16_u32)
IMPL_MULTILIB_FUNCTION_INTERFACE(long_cvt_noinline_u8_u32)
IMPL_MULTILIB_FUNCTION_INTERFACE(long_cvt_noinline_u8_u16)
IMPL_MULTILIB_FUNCTION_INTERFACE(long_cvt_noinline_u32_u16)
IMPL_MULTILIB_FUNCTION_INTERFACE(long_cvt_noinline_u32_u8)
IMPL_MULTILIB_FUNCTION_INTERFACE(long_cvt_noinline_u16_u8)

typedef enum X86SIMDFeatureLevel {
    X86SIMDFeatureLevelSSE2 = 0,
    X86SIMDFeatureLevelSSE4_2 = 1,
    X86SIMDFeatureLevelAVX2 = 2,
    X86SIMDFeatureLevelAVX512 = 3,
    X86SIMDFeatureLevelMAX = 4,
} X86SIMDFeatureLevel;

#    define PLATFORM_SIMD_LEVEL X86SIMDFeatureLevel
#elif SSRJSON_AARCH
typedef enum AArchSIMDFeatureLevel {
    AArchSIMDFeatureLevelNEON = 0,
} AArchSIMDFeatureLevel;

#    define PLATFORM_SIMD_LEVEL AArchSIMDFeatureLevel
#endif

// typedef void (*long_cvt_noinline_u16_u32)(u32 *restrict write_start, const u16 *restrict read_start, usize _len);
// typedef void (*long_cvt_noinline_u8_u32)(u32 *restrict write_start, const u8 *restrict read_start, usize _len);
// typedef void (*long_cvt_noinline_u8_u16)(u16 *restrict write_start, const u8 *restrict read_start, usize _len);
// typedef void (*long_cvt_noinline_u32_u16)(u16 *restrict write_start, const u32 *restrict read_start, usize _len);
// typedef void (*long_cvt_noinline_u32_u8)(u8 *restrict write_start, const u32 *restrict read_start, usize _len);
// typedef void (*long_cvt_noinline_u16_u8)(u8 *restrict write_start, const u16 *restrict read_start, usize _len);


int CurrentSIMDFeatureLevel = -1;

PLATFORM_SIMD_LEVEL get_simd_feature(void) {
#if SSRJSON_X86
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

    //
    return X86SIMDFeatureLevelSSE2;
#elif SSRJSON_AARCH
    return AArchSIMDFeatureLevelNEON;
#endif
}

force_inline void _update_simd_features(void) {
    if (unlikely(CurrentSIMDFeatureLevel == -1)) {
        PLATFORM_SIMD_LEVEL simd_feature = get_simd_feature();
#if SSRJSON_X86
        switch (simd_feature) {
            case X86SIMDFeatureLevelSSE2: {
                // TODO
                assert(false);
                // SET_INTERFACE(ssrjson_Encode, sse2);
                // SET_INTERFACE(ssrjson_Decode, sse2);
                // SET_INTERFACE(ssrjson_EncodeToBytes, sse2);
                // SET_INTERFACE(long_cvt_noinline_u16_u32, sse2);
                // SET_INTERFACE(long_cvt_noinline_u8_u32, sse2);
                // SET_INTERFACE(long_cvt_noinline_u8_u16, sse2);
                // SET_INTERFACE(long_cvt_noinline_u32_u16, sse2);
                // SET_INTERFACE(long_cvt_noinline_u32_u8, sse2);
                // SET_INTERFACE(long_cvt_noinline_u16_u8, sse2);
                break;
            }
            case X86SIMDFeatureLevelSSE4_2: {
                SET_INTERFACE(ssrjson_Encode, sse4_2);
                SET_INTERFACE(ssrjson_Decode, sse4_2);
                SET_INTERFACE(ssrjson_EncodeToBytes, sse4_2);
                SET_INTERFACE(long_cvt_noinline_u16_u32, sse4_2);
                SET_INTERFACE(long_cvt_noinline_u8_u32, sse4_2);
                SET_INTERFACE(long_cvt_noinline_u8_u16, sse4_2);
                SET_INTERFACE(long_cvt_noinline_u32_u16, sse4_2);
                SET_INTERFACE(long_cvt_noinline_u32_u8, sse4_2);
                SET_INTERFACE(long_cvt_noinline_u16_u8, sse4_2);
                break;
            }
            case X86SIMDFeatureLevelAVX2: {
                SET_INTERFACE(ssrjson_Encode, avx2);
                SET_INTERFACE(ssrjson_Decode, avx2);
                SET_INTERFACE(ssrjson_EncodeToBytes, avx2);
                SET_INTERFACE(long_cvt_noinline_u16_u32, avx2);
                SET_INTERFACE(long_cvt_noinline_u8_u32, avx2);
                SET_INTERFACE(long_cvt_noinline_u8_u16, avx2);
                SET_INTERFACE(long_cvt_noinline_u32_u16, avx2);
                SET_INTERFACE(long_cvt_noinline_u32_u8, avx2);
                SET_INTERFACE(long_cvt_noinline_u16_u8, avx2);
                break;
            }
            case X86SIMDFeatureLevelAVX512: {
                SET_INTERFACE(ssrjson_Encode, avx512);
                SET_INTERFACE(ssrjson_Decode, avx512);
                SET_INTERFACE(ssrjson_EncodeToBytes, avx512);
                SET_INTERFACE(long_cvt_noinline_u16_u32, avx512);
                SET_INTERFACE(long_cvt_noinline_u8_u32, avx512);
                SET_INTERFACE(long_cvt_noinline_u8_u16, avx512);
                SET_INTERFACE(long_cvt_noinline_u32_u16, avx512);
                SET_INTERFACE(long_cvt_noinline_u32_u8, avx512);
                SET_INTERFACE(long_cvt_noinline_u16_u8, avx512);
                break;
            }
            default: {
                assert(false);
            }
        }
#elif SSRJSON_AARCH
// TODO
#endif
        // mark as ready
        CurrentSIMDFeatureLevel = (int)simd_feature;
    }
}

MAKE_FORWARD_PYFUNCTION_IMPL(ssrjson_Encode)
MAKE_FORWARD_PYFUNCTION_IMPL(ssrjson_Decode)
MAKE_FORWARD_PYFUNCTION_IMPL(ssrjson_EncodeToBytes)

PyObject *ssrjson_print_current_features(PyObject *self, PyObject *args) {
    // TODO change to returning a dict with all build info
    _update_simd_features();
#if SSRJSON_X86
    switch (CurrentSIMDFeatureLevel) {
        case X86SIMDFeatureLevelSSE2: {
            printf("SIMD: SSE2\n");
            break;
        }
        case X86SIMDFeatureLevelSSE4_2: {
            printf("SIMD: SSE4.2\n");
            break;
        }
        case X86SIMDFeatureLevelAVX2: {
            printf("SIMD: AVX2\n");
            break;
        }
        case X86SIMDFeatureLevelAVX512: {
            printf("SIMD: AVX512\n");
            break;
        }
        default: {
            printf("SIMD: Unknown\n");
            break;
        }
    }
#elif SSRJSON_AARCH
    printf("SIMD: NEON\n");
#endif
    Py_RETURN_NONE;
}

PyObject *ssrjson_get_current_features(PyObject *self, PyObject *args) {
    PyObject *ret = PyDict_New();
    _update_simd_features();
    PyDict_SetItemString(ret, "MultiLib", PyBool_FromLong(true));
#if SSRJSON_X86
    switch (CurrentSIMDFeatureLevel) {
        // case X86SIMDFeatureLevelSSE2: {
        //     PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("SSE2"));
        //     break;
        // }
        case X86SIMDFeatureLevelSSE4_2: {
            PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("SSE4.2"));
            break;
        }
        case X86SIMDFeatureLevelAVX2: {
            PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("AVX2"));
            break;
        }
        case X86SIMDFeatureLevelAVX512: {
            PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("AVX512"));
            break;
        }
        default: {
            PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("Unknown"));
            break;
        }
    }
#elif SSRJSON_AARCH
    PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("NEON"));
#endif
    return ret;
}

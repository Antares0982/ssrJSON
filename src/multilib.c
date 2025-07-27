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


int CurrentSIMDFeatureLevel = -1;

const char *_update_simd_features(void) {
    const char *err = NULL;
    X86SIMDFeatureLevel simd_feature = get_simd_feature();
    switch (simd_feature) {
        case X86SIMDFeatureLevelSSE2: {
            err = "Current hardware is not supported; SSE4.2 is required.";
            break;
        }
        case X86SIMDFeatureLevelSSE4_2: {
            BATCH_SET_INTERFACE(sse4_2);
            break;
        }
        case X86SIMDFeatureLevelAVX2: {
            BATCH_SET_INTERFACE(avx2);
            break;
        }
        case X86SIMDFeatureLevelAVX512: {
            BATCH_SET_INTERFACE(avx512);
            break;
        }
        default: {
            assert(false);
        }
    }
    // mark as ready
    CurrentSIMDFeatureLevel = (int)simd_feature;
    return err;
}

MAKE_FORWARD_PYFUNCTION_IMPL(ssrjson_Encode)
MAKE_FORWARD_PYFUNCTION_IMPL(ssrjson_Decode)
MAKE_FORWARD_PYFUNCTION_IMPL(ssrjson_EncodeToBytes)

PyObject *ssrjson_print_current_features(PyObject *self, PyObject *args) {
    // TODO change to returning a dict with all build info
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
    Py_RETURN_NONE;
}

PyObject *ssrjson_get_current_features(PyObject *self, PyObject *args) {
    PyObject *ret = PyDict_New();
    PyDict_SetItemString(ret, "MultiLib", PyBool_FromLong(true));
    switch (CurrentSIMDFeatureLevel) {
        case X86SIMDFeatureLevelSSE2: {
            PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("SSE2"));
            break;
        }
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
    return ret;
}
#else // SSRJSON_X86
static_assert(false, "multilib not supported on this platform");
#endif

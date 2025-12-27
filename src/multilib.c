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

extern int ssrjson_write_utf8_cache_value;
extern int ssrjson_nonstrict_argparse;

#define IMPL_MULTILIB_FUNCTION_INTERFACE(_func_name_) SSRJSON_CONCAT2(_func_name_, t) SSRJSON_CONCAT2(_func_name_, interface);

IMPL_MULTILIB_FUNCTION_INTERFACE(ssrjson_Encode)
IMPL_MULTILIB_FUNCTION_INTERFACE(ssrjson_Decode)
IMPL_MULTILIB_FUNCTION_INTERFACE(ssrjson_EncodeToBytes)
IMPL_MULTILIB_FUNCTION_INTERFACE(long_cvt_noinline_u16_u32)
IMPL_MULTILIB_FUNCTION_INTERFACE(long_cvt_noinline_u8_u32)
IMPL_MULTILIB_FUNCTION_INTERFACE(long_cvt_noinline_u8_u16)
IMPL_MULTILIB_FUNCTION_INTERFACE(long_cvt_noinline_u32_u16)
IMPL_MULTILIB_FUNCTION_INTERFACE(long_cvt_noinline_u32_u8)
IMPL_MULTILIB_FUNCTION_INTERFACE(long_cvt_noinline_u16_u8)

#if SSRJSON_X86


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

MAKE_FORWARD_PYFASTFUNCTION_IMPL(ssrjson_Encode)
MAKE_FORWARD_PYFASTFUNCTION_IMPL(ssrjson_Decode)
MAKE_FORWARD_PYFASTFUNCTION_IMPL(ssrjson_EncodeToBytes)

PyObject *ssrjson_get_current_features(PyObject *self, PyObject *args) {
    int err;
    PyObject *ret = PyDict_New();
    if (!ret) return NULL;
    err = PyDict_SetItemString(ret, "multi_lib", Py_True);
    if (err) goto fail;
    PyObject *write_cache_bool = ssrjson_write_utf8_cache_value ? Py_True : Py_False;
    err = PyDict_SetItemString(ret, "write_utf8_cache", write_cache_bool);
    if (err) goto fail;
    PyObject *strict_argparse_bool = ssrjson_nonstrict_argparse ? Py_False : Py_True;
    err = PyDict_SetItemString(ret, "strict_arg_parse", strict_argparse_bool);
    if (err) goto fail;

#    define DICT_SET_STRING_ITEM(_k_, _v_)               \
        do {                                             \
            PyObject *val = PyUnicode_FromString((_v_)); \
            if (!val) goto fail;                         \
            err = PyDict_SetItemString(ret, (_k_), val); \
            Py_DECREF(val);                              \
            if (err) goto fail;                          \
        } while (0)
    switch (CurrentSIMDFeatureLevel) {
        case X86SIMDFeatureLevelSSE2: {
            DICT_SET_STRING_ITEM("simd", "SSE2");
            break;
        }
        case X86SIMDFeatureLevelSSE4_2: {
            DICT_SET_STRING_ITEM("simd", "SSE4.2");
            break;
        }
        case X86SIMDFeatureLevelAVX2: {
            DICT_SET_STRING_ITEM("simd", "AVX2");
            break;
        }
        case X86SIMDFeatureLevelAVX512: {
            DICT_SET_STRING_ITEM("simd", "AVX512");
            break;
        }
        default: {
            DICT_SET_STRING_ITEM("simd", "Unknown");
            break;
        }
    }
    return ret;
fail:;
    Py_DECREF(ret);
    return NULL;
}
#elif SSRJSON_AARCH // SSRJSON_X86
const char *_update_simd_features(void) {
    BATCH_SET_INTERFACE(neon);
    return NULL;
}

MAKE_FORWARD_PYFASTFUNCTION_IMPL(ssrjson_Encode)
MAKE_FORWARD_PYFASTFUNCTION_IMPL(ssrjson_Decode)
MAKE_FORWARD_PYFASTFUNCTION_IMPL(ssrjson_EncodeToBytes)

PyObject *ssrjson_get_current_features(PyObject *self, PyObject *args) {
    int err;
    PyObject *ret = PyDict_New();
    if (!ret) return NULL;
    err = PyDict_SetItemString(ret, "multi_lib", Py_True);
    if (err) goto fail;
    PyObject *write_cache_bool = ssrjson_write_utf8_cache_value ? Py_True : Py_False;
    err = PyDict_SetItemString(ret, "write_utf8_cache", write_cache_bool);
    if (err) goto fail;
    PyObject *strict_argparse_bool = ssrjson_nonstrict_argparse ? Py_False : Py_True;
    err = PyDict_SetItemString(ret, "strict_arg_parse", strict_argparse_bool);
    if (err) goto fail;
    PyObject *neon = PyUnicode_FromString("NEON");
    if (!neon) goto fail;
    err = PyDict_SetItemString(ret, "simd", neon);
    Py_DECREF(neon);
    if (err) goto fail;
    return ret;
fail:;
    Py_DECREF(ret);
    return NULL;
}

#else // SSRJSON_X86
static_assert(false, "multilib not supported on this platform");
#endif

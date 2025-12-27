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
#include "simd/simd_detect.h"
#include "ssrjson.h"

extern int ssrjson_write_utf8_cache_value;
extern int ssrjson_nonstrict_argparse;

PyObject *ssrjson_get_current_features(PyObject *self, PyObject *args) {
    int err;
    PyObject *ret = PyDict_New();
    if (!ret) return NULL;
    PyObject *write_cache_bool = ssrjson_write_utf8_cache_value ? Py_True : Py_False;
    err = PyDict_SetItemString(ret, "write_utf8_cache", write_cache_bool);
    if (err) goto fail;
    PyObject *strict_argparse_bool = ssrjson_nonstrict_argparse ? Py_False : Py_True;
    err = PyDict_SetItemString(ret, "strict_arg_parse", strict_argparse_bool);
    if (err) goto fail;

#define DICT_SET_STRING_ITEM(_k_, _v_)               \
    do {                                             \
        PyObject *val = PyUnicode_FromString((_v_)); \
        if (!val) goto fail;                         \
        err = PyDict_SetItemString(ret, (_k_), val); \
        Py_DECREF(val);                              \
        if (err) goto fail;                          \
    } while (0)

#if SSRJSON_X86
    err = PyDict_SetItemString(ret, "multi_lib", Py_False);
    if (err) goto fail;

#    if HAS_AVX512
    DICT_SET_STRING_ITEM("simd", "AVX512");
#    elif HAS_AVX2
    DICT_SET_STRING_ITEM("simd", "AVX2");
#    elif HAS_SSE4_2
    DICT_SET_STRING_ITEM("simd", "SSE4.2");
#    else
    DICT_SET_STRING_ITEM("simd", "SSE2");
#    endif
#elif SSRJSON_AARCH
    DICT_SET_STRING_ITEM("simd", "NEON");
#endif
    return ret;
fail:;
    Py_DECREF(ret);
    return NULL;
}

const char *_update_simd_features(void) {
#if SSRJSON_BUILD_NATIVE
    // if using native build, don't check for features, assume all features are available
    return NULL;
#else

    PLATFORM_SIMD_LEVEL simd_feature = get_simd_feature();
#    if SSRJSON_X86
#        if HAS_AVX512
    // compile support 512 bits
    if (simd_feature < X86SIMDFeatureLevelAVX512) {
        return "AVX512 is not supported by the current CPU, but the library was compiled with AVX512 support.";
    }
    return NULL;
#        elif HAS_AVX2
    // compile support 256 bits
    if (simd_feature < X86SIMDFeatureLevelAVX2) {
        return "AVX2 is not supported by the current CPU, but the library was compiled with AVX2 support.";
    }
    return NULL;
#        elif HAS_SSE4_2
    if (simd_feature < X86SIMDFeatureLevelSSE4_2) {
        return "SSE4.2 is not supported by the current CPU, but the library was compiled with SSE4.2 support.";
    }
    return NULL;
#        else
    return NULL;
#        endif
#    elif SSRJSON_AARCH
    return NULL;
#    else
    // unknown platform
    return NULL;
#    endif
#endif
}

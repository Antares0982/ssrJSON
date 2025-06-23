#include "ssrjson.h"

PyObject *ssrjson_print_current_features(PyObject *self, PyObject *args) {
    // TODO change to returning a dict with all build info

#if SSRJSON_X86
#    if COMPILE_SIMD_BITS == 512
    printf("SIMD: AVX512; MultiLib: False\n");
#    elif COMPILE_SIMD_BITS == 256
    printf("SIMD: AVX2; MultiLib: False\n");
// #    elif __SSE4_2__
//     printf("SIMD: SSE4.2; MultiLib: False\n");
#    else
    printf("SIMD: SSE2; MultiLib: False\n");
#    endif
#elif SSRJSON_AARCH
    printf("SIMD: NEON; MultiLib: False\n");
#endif
    Py_RETURN_NONE;
}

PyObject *ssrjson_get_current_features(PyObject *self, PyObject *args) {
    PyObject *ret = PyDict_New();

#if SSRJSON_X86
    PyDict_SetItemString(ret, "MultiLib", PyBool_FromLong(false));

#    if COMPILE_SIMD_BITS == 512
    PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("AVX512"));
#    elif COMPILE_SIMD_BITS == 256
    PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("AVX2"));
// #    elif __SSE4_2__
//     PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("SSE4.2"));
#    else
    PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("SSE2"));
#    endif
#elif SSRJSON_AARCH
    PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("NEON"));
#endif
    return ret;
}

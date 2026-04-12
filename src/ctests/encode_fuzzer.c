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


#include "test_common.h"

PyObject *encode_fuzz_module = NULL;
PyObject *fuzz_encode_func = NULL;

int LLVMFuzzerInitialize(int *argc, char ***argv) {
    initialize_cpython();
    // import ssrjson to ensure it's available
    PyObject *ssrjson_module = import_ssrjson();
    if (!ssrjson_module) goto fail;
    Py_DECREF(ssrjson_module);
    // import the encode_fuzz module
    encode_fuzz_module = PyImport_ImportModule("encode_fuzz");
    if (!encode_fuzz_module) goto fail;
    fuzz_encode_func = PyObject_GetAttrString(encode_fuzz_module, "fuzz_encode");
    if (!fuzz_encode_func) goto fail;
    return 0;
fail:;
    PyErr_Print();
    Py_XDECREF(encode_fuzz_module);
    printf("%s\n", "Cannot initialize encode fuzzer");
    __builtin_trap();
    return -1;
}

int LLVMFuzzerTestOneInput(const u8 *data, usize size) {
    if (size < 5) return 0;
    PyObject *input_bytes = PyBytes_FromStringAndSize((const char *)data, (Py_ssize_t)size);
    if (!input_bytes) {
        PyErr_Clear();
        return -1;
    }
    PyObject *ret = PyObject_Vectorcall(fuzz_encode_func, &input_bytes, 1, NULL);
    if (!ret) {
        PyErr_Print();
        Py_DECREF(input_bytes);
        assert(false);
        __builtin_trap();
    }
    Py_DECREF(ret);
    Py_DECREF(input_bytes);
    return 0;
}

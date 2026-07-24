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


#include "encode_fuzz_gen.h"
#include "test_common.h"

PyObject *encode_fuzz_module = NULL;
PyObject *verify_encode_func = NULL;

int LLVMFuzzerInitialize(int *argc, char ***argv) {
    if (!initialize_cpython()) goto fail;
    // import ssrjson to ensure it's available
    PyObject *ssrjson_module = import_ssrjson();
    if (!ssrjson_module) goto fail;
    Py_DECREF(ssrjson_module);
    // import the encode_fuzz module (verification side)
    encode_fuzz_module = PyImport_ImportModule("encode_fuzz");
    if (!encode_fuzz_module) goto fail;
    verify_encode_func = PyObject_GetAttrString(encode_fuzz_module, "verify_encode");
    if (!verify_encode_func) goto fail;
    // resolve the C generator's Python-side references (subclasses, numpy)
    if (!encode_fuzz_gen_init(encode_fuzz_module)) goto fail;
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

    // Build the object tree directly from the fuzzer bytes (fast, byte-local).
    bool has_surrogate = false;
    PyObject *obj = encode_fuzz_gen_build(data, size, &has_surrogate);
    if (!obj) {
        // Generation failure (e.g. MemoryError) is not a finding; skip.
        PyErr_Clear();
        return 0;
    }

    // Hand the object to the Python verifier: verify_encode(obj, has_surrogate).
    PyObject *args[2] = {obj, has_surrogate ? Py_True : Py_False};
    PyObject *ret = PyObject_Vectorcall(verify_encode_func, args, 2, NULL);
    if (!ret) {
        PyErr_Print();
        Py_DECREF(obj);
        assert(false);
        __builtin_trap();
    }
    Py_DECREF(ret);
    Py_DECREF(obj);
    return 0;
}

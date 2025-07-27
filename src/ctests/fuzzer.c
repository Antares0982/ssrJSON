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

typedef struct TestArgSettings {
    int valid;
    PyObject *args[16];
} TestArgSettings;

typedef struct TestResult {
    PyObject *decoded;
    PyObject *encoded;
} TestResult;

PyObject *ssrjson_module = NULL;
PyObject *decode_func = NULL;
PyObject *encode_func = NULL;
PyObject *ssrjson_fuzz_module = NULL;
PyObject *fuzz_bytes_input = NULL;
PyObject *fuzz_str_input = NULL;

TestArgSettings decode_arg_settings[16];
TestArgSettings encode_arg_settings[16];

static bool init_test_arg_settings(void) {
    PyObject *pylong_1 = NULL, *pylong_2 = NULL;
    memset(decode_arg_settings, 0, sizeof(decode_arg_settings));
    memset(encode_arg_settings, 0, sizeof(encode_arg_settings));
    pylong_1 = PyLong_FromLong(1);
    pylong_2 = PyLong_FromLong(2);
    if (!pylong_1 || !pylong_2) goto fail;
    //
    decode_arg_settings[0].valid = 1;
    //
    encode_arg_settings[0].valid = 1;
    encode_arg_settings[1].valid = 1;
    encode_arg_settings[1].args[1] = pylong_1;
    encode_arg_settings[2].valid = 1;
    encode_arg_settings[2].args[1] = pylong_2;
    return true;
fail:;
    Py_XDECREF(pylong_1);
    return false;
}

int LLVMFuzzerInitialize(int *argc, char ***argv) {
    initialize_cpython();
    ssrjson_module = import_ssrjson();
    if (!ssrjson_module) goto fail;
    decode_func = PyObject_GetAttrString(ssrjson_module, "loads");
    if (!decode_func) goto fail;
    encode_func = PyObject_GetAttrString(ssrjson_module, "dumps");
    if (!encode_func) goto fail;
    if (!init_test_arg_settings()) goto fail;
    ssrjson_fuzz_module = PyImport_ImportModule("ssrjson_fuzz");
    if (!ssrjson_fuzz_module) goto fail;
    fuzz_bytes_input = PyObject_GetAttrString(ssrjson_fuzz_module, "fuzz_bytes_input");
    if (!fuzz_bytes_input) goto fail;
    fuzz_str_input = PyObject_GetAttrString(ssrjson_fuzz_module, "fuzz_str_input");
    if (!fuzz_str_input) goto fail;
    return 0;
fail:;
    Py_XDECREF(ssrjson_module);
    Py_XDECREF(decode_func);
    Py_XDECREF(encode_func);
    Py_XDECREF(ssrjson_fuzz_module);
    printf("%s\n", "Cannot initialize");
    __builtin_trap();
    return -1;
}

static void make_ucs1(const u8 *data, usize size, PyObject **ucs1) {
    *ucs1 = NULL;
    if (!size) return;
    bool is_ascii = true;
    for (usize i = 0; i < size; ++i) {
        if (data[i] >= 128) {
            is_ascii = false;
            break;
        }
    }
    PyObject *ret_unicode = PyUnicode_New(size, is_ascii ? 127 : 255);
    if (!ret_unicode) return;
    memcpy(PyUnicode_DATA(ret_unicode), data, size);
    *ucs1 = ret_unicode;
}

static void make_ucs2(const u8 *data, usize size, PyObject **ucs2) {
    *ucs2 = NULL;
    usize ucs2_count = size / 2;
    if (!ucs2_count) return;
    const u16 *ucs2_data = (const u16 *)data;
    bool has_ucs2 = false;
    for (usize i = 0; i < ucs2_count; i++) {
        if (ucs2_data[i] >= 256) {
            has_ucs2 = true;
            break;
        }
    }
    if (!has_ucs2) return;
    PyObject *ret_unicode = PyUnicode_New(ucs2_count, 65535);
    if (!ret_unicode) return;
    memcpy(PyUnicode_DATA(ret_unicode), ucs2_data, ucs2_count * 2);
    *ucs2 = ret_unicode;
}

static void make_ucs4(const u8 *data, usize size, PyObject **ucs4) {
    *ucs4 = NULL;
    usize ucs4_count = size / 4;
    if (!ucs4_count) return;
    const u32 *ucs4_data = (const u32 *)data;
    bool has_ucs4 = false;
    for (usize i = 0; i < ucs4_count; i++) {
        if (ucs4_data[i] >= 65536) {
            has_ucs4 = true;
            break;
        }
    }
    if (!has_ucs4) return;
    u32 *buffer = (u32 *)malloc(ucs4_count * 4);
    if (!buffer) return;
    memcpy(buffer, ucs4_data, ucs4_count * 4);
    for (usize i = 0; i < ucs4_count; i++) {
        if (buffer[i] >= 1114112) {
            buffer[i] = buffer[i] % 1114112;
        }
    }
    PyObject *ret_unicode = PyUnicode_New(ucs4_count, 1114111);
    if (!ret_unicode) {
        free(buffer);
        return;
    }
    memcpy(PyUnicode_DATA(ret_unicode), buffer, ucs4_count * 4);
    free(buffer);
    *ucs4 = ret_unicode;
}

static void parse_input_to_bytes(const u8 *data, usize size, PyObject **bytes, PyObject **str, PyObject **ucs1, PyObject **ucs2, PyObject **ucs4) {
    *bytes = PyBytes_FromStringAndSize((const char *)data, (Py_ssize_t)size);
    if (!*bytes) PyErr_Clear();
    *str = PyUnicode_FromStringAndSize((const char *)data, (Py_ssize_t)size);
    if (!*str) PyErr_Clear();
    make_ucs1(data, size, ucs1);
    make_ucs2(data, size, ucs2);
    make_ucs4(data, size, ucs4);
}

int LLVMFuzzerTestOneInput(const u8 *data, usize size) {
    if (!size) return 0;
    PyObject *bytes, *str, *ucs1, *ucs2, *ucs4;
    // parse input to bytes and str
    parse_input_to_bytes(data, size, &bytes, &str, &ucs1, &ucs2, &ucs4);
    if (!bytes && !str && !ucs1 && !ucs2 && !ucs4) {
        // invalid input
        return -1;
    }
    if (bytes) {
        PyObject *ret = PyObject_Vectorcall(fuzz_bytes_input, &bytes, 1, NULL);
        if (!ret) {
            PyErr_Print();
            assert(false);
            __builtin_trap();
        }
        Py_DECREF(ret);
    }
    if (str) {
        PyObject *ret = PyObject_Vectorcall(fuzz_str_input, &str, 1, NULL);
        if (!ret) {
            PyErr_Print();
            assert(false);
            __builtin_trap();
        }
        Py_DECREF(ret);
    }
    if (ucs1) {
        PyObject *ret = PyObject_Vectorcall(fuzz_str_input, &ucs1, 1, NULL);
        if (!ret) {
            PyErr_Print();
            assert(false);
            __builtin_trap();
        }
        Py_DECREF(ret);
    }
    if (ucs2) {
        PyObject *ret = PyObject_Vectorcall(fuzz_str_input, &ucs2, 1, NULL);
        if (!ret) {
            PyErr_Print();
            assert(false);
            __builtin_trap();
        }
        Py_DECREF(ret);
    }
    if (ucs4) {
        PyObject *ret = PyObject_Vectorcall(fuzz_str_input, &ucs4, 1, NULL);
        if (!ret) {
            PyErr_Print();
            assert(false);
            __builtin_trap();
        }
        Py_DECREF(ret);
    }
    Py_XDECREF(bytes);
    Py_XDECREF(str);
    Py_XDECREF(ucs1);
    Py_XDECREF(ucs2);
    Py_XDECREF(ucs4);
    return 0;
}

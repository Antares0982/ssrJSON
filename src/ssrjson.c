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

#include "ssrjson.h"
#include "pythonlib.h"
#include "tls.h"
#include "version.h"

#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"

extern decode_cache_t DecodeKeyCache[SSRJSON_KEY_CACHE_SIZE];

PyObject *ssrjson_Encode(PyObject *self, PyObject *const *args, Py_ssize_t nargsf, PyObject *kwnames);
PyObject *ssrjson_EncodeToBytes(PyObject *self, PyObject *const *args, Py_ssize_t nargsf, PyObject *kwnames);
PyObject *ssrjson_Decode(PyObject *self, PyObject *const *args, Py_ssize_t nargsf, PyObject *kwnames);
PyObject *ssrjson_suppress_api_warning(PyObject *self, PyObject *args);
PyObject *ssrjson_strict_argparse(PyObject *self, PyObject *arg);
PyObject *ssrjson_write_utf8_cache(PyObject *self, PyObject *arg);

PyObject *JSONDecodeError = NULL;
PyObject *JSONEncodeError = NULL;

int ssrjson_invalid_arg_checked = 0;
int ssrjson_nonstrict_argparse = SSRJSON_NONSTRICT_ARGPARSE;
int ssrjson_write_utf8_cache_value = SSRJSON_WRITE_UTF8_CACHE;


static void module_free(void *m);

static int ssrjson_exec(PyObject *module);

static struct PyModuleDef_Slot ssrjson_slots[] = {
        {Py_mod_exec, (void *)ssrjson_exec}, // stage 2
#if PY_MINOR_VERSION >= 12
        {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED}, // stage 3
#endif
        {0, NULL}};

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "ssrjson",
        0,             /* m_doc */
        0,             /* m_size */
        NULL,          /* m_methods */
        ssrjson_slots, /* m_slots */
        NULL,          /* m_traverse */
        NULL,          /* m_clear */
        module_free    /* m_free */
};

static void module_free(void *m) {
    for (size_t i = 0; i < SSRJSON_KEY_CACHE_SIZE; i++) {
        Py_XDECREF(DecodeKeyCache[i].key);
        DecodeKeyCache[i].key = NULL;
    }

    if (JSONDecodeError) {
        Py_DECREF(JSONDecodeError);
        JSONDecodeError = NULL;
    }

    if (JSONEncodeError) {
        Py_DECREF(JSONEncodeError);
        JSONEncodeError = NULL;
    }

#if defined(Py_GIL_DISABLED)
    if (unlikely(!ssrjson_tls_free())) {
        printf("ssrjson: failed to free TLS\n");
    }
#endif
}

#if PY_MINOR_VERSION >= 13
PyTypeObject *PyNone_Type = NULL;
#endif

PyMethodDef dumps_func_def = {
        "dumps",
        (PyCFunction)ssrjson_Encode,
        METH_FASTCALL | METH_KEYWORDS,
        "dumps(obj, indent=None)\n--\n\nConverts arbitrary object recursively into JSON."};

PyMethodDef dumps_to_bytes_func_def = {
        "dumps_to_bytes",
        (PyCFunction)ssrjson_EncodeToBytes,
        METH_FASTCALL | METH_KEYWORDS,
        "dumps_to_bytes(obj, indent=None, is_write_cache=None)\n--\n\nConverts arbitrary object recursively into JSON."};

PyMethodDef loads_func_def = {
        "loads",
        (PyCFunction)ssrjson_Decode,
        METH_FASTCALL | METH_KEYWORDS,
        "loads(s)\n--\n\nConverts JSON as string to dict object structure."};

PyMethodDef get_current_features_func_def = {
        "get_current_features",
        (PyCFunction)ssrjson_get_current_features,
        METH_NOARGS,
        "get_current_features()\n--\n\nGet current features."};

PyMethodDef suppress_api_warning_func_def = {
        "suppress_api_warning",
        (PyCFunction)ssrjson_suppress_api_warning,
        METH_NOARGS,
        "suppress_api_warning()\n--\n\nSuppress warning when invalid arguments received."};

PyMethodDef strict_argparse_func_def = {
        "strict_argparse",
        (PyCFunction)ssrjson_strict_argparse,
        METH_O,
        "strict_argparse(value)\n--\n\nSet strict argument parsing mode. Default is False."};

PyMethodDef write_utf8_cache_func_def = {
        "write_utf8_cache",
        (PyCFunction)ssrjson_write_utf8_cache,
        METH_O,
        "write_utf8_cache(value)\n--\n\nSet whether to write UTF-8 cache when calling dumps_to_bytes(). Default is True."};

const char *_getenv_write_cache(long *out_value) {
    const char *env = getenv("SSRJSON_WRITE_UTF8_CACHE");
    if (env == NULL) {
        *out_value = SSRJSON_WRITE_UTF8_CACHE;
        return NULL;
    }
    if ((env[0] == '1' || env[0] == '0') && env[1] == 0) {
        *out_value = env[0] - '0';
        return NULL;
    }
    return "Invalid SSRJSON_WRITE_UTF8_CACHE environment variable value";
}

static int ssrjson_exec(PyObject *module) {
    int err;
    const char *err_s;
    PyObject *func;

#define RETURN_WHEN_ERR() \
    if (err < 0) {        \
        return -1;        \
    }

#define ADD_FUNC_CHECKED(_func_def_)                                   \
    {                                                                  \
        func = PyCFunction_NewEx(&_func_def_, NULL, module_string);    \
        if (!func) {                                                   \
            Py_DECREF(module_string);                                  \
            return -1;                                                 \
        }                                                              \
        err = PyModule_AddObjectRef(module, _func_def_.ml_name, func); \
        Py_DECREF(func);                                               \
        if (err < 0) {                                                 \
            Py_DECREF(module_string);                                  \
            return -1;                                                 \
        }                                                              \
    }

    err_s = _update_simd_features();
    if (err_s) {
        PyErr_SetString(PyExc_ImportError, err_s);
        return -1;
    }

    // Read environment variable SSRJSON_WRITE_UTF8_CACHE
    long _write_utf8_cache_value;
    err_s = _getenv_write_cache(&_write_utf8_cache_value);
    if (err_s) {
        PyErr_SetString(PyExc_ImportError, err_s);
        return -1;
    }
    ssrjson_write_utf8_cache_value = (int)_write_utf8_cache_value;

    if (PyModule_AddStringConstant(module, "__version__", SSRJSON_VERSION) < 0) {
        return -1;
    }

    // Add: JSONDecodeError
    if (!JSONDecodeError) {
        JSONDecodeError = PyErr_NewException("ssrjson.JSONDecodeError", PyExc_ValueError, NULL);
        if (!JSONDecodeError) {
            return -1;
        }
        // global variable needs an additional ref
        Py_INCREF(JSONDecodeError);
    }

    err = PyModule_AddObjectRef(module, "JSONDecodeError", JSONDecodeError);
    Py_DECREF(JSONDecodeError);
    RETURN_WHEN_ERR();

    // Add: JSONEncodeError
    if (!JSONEncodeError) {
        JSONEncodeError = PyErr_NewException("ssrjson.JSONEncodeError", PyExc_ValueError, NULL);
        if (!JSONEncodeError) {
            return -1;
        }
        // global variable needs an additional ref
        Py_INCREF(JSONEncodeError);
    }

    err = PyModule_AddObjectRef(module, "JSONEncodeError", JSONEncodeError);
    Py_DECREF(JSONEncodeError);
    RETURN_WHEN_ERR();

    PyObject *module_string = PyUnicode_FromString("ssrjson");
    if (!module_string) return -1;

    ADD_FUNC_CHECKED(dumps_func_def);
    ADD_FUNC_CHECKED(dumps_to_bytes_func_def);
    ADD_FUNC_CHECKED(loads_func_def);
    ADD_FUNC_CHECKED(get_current_features_func_def);
    ADD_FUNC_CHECKED(suppress_api_warning_func_def);
    ADD_FUNC_CHECKED(strict_argparse_func_def);
    ADD_FUNC_CHECKED(write_utf8_cache_func_def);

    Py_DECREF(module_string);

#if defined(Py_GIL_DISABLED)
    // TLS init.
    if (unlikely(!ssrjson_tls_init())) {
        PyErr_SetString(PyExc_ImportError, "Failed to initialize TLS");
        return -1;
    }
#endif

    // codes below should not fail.

    // do ssrjson internal init.

    memset(DecodeKeyCache, 0, sizeof(DecodeKeyCache));

#if PY_MINOR_VERSION >= 13
    PyNone_Type = Py_TYPE(Py_None);
#endif

    return 0;
#undef ADD_FUNC_CHECKED
#undef RETURN_WHEN_ERR
}

PyMODINIT_FUNC PyInit_ssrjson(void) {
    return PyModuleDef_Init(&moduledef);
}

PyObject *ssrjson_suppress_api_warning(PyObject *self, PyObject *args) {
    ssrjson_invalid_arg_checked = 1;
    Py_RETURN_NONE;
}

PyObject *ssrjson_strict_argparse(PyObject *self, PyObject *arg) {
    bool value_is_true = arg == Py_True;
    bool value_is_false = arg == Py_False;
    if (unlikely(!value_is_true && !value_is_false)) {
        PyErr_SetString(PyExc_TypeError, "strict_argparse() argument must be True or False");
        return NULL;
    }
    ssrjson_nonstrict_argparse = value_is_false ? 1 : 0;
    Py_RETURN_NONE;
}

PyObject *ssrjson_write_utf8_cache(PyObject *self, PyObject *arg) {
    bool value_is_true = arg == Py_True;
    bool value_is_false = arg == Py_False;
    if (unlikely(!value_is_true && !value_is_false)) {
        PyErr_SetString(PyExc_TypeError, "write_utf8_cache() argument must be True or False");
        return NULL;
    }
    ssrjson_write_utf8_cache_value = value_is_true ? 1 : 0;
    Py_RETURN_NONE;
}

void handle_unexpected_kw(const char *func_name, PyObject *kwname) {
    PyObject *ascii_repr = PyObject_ASCII(kwname);
    if (ascii_repr) {
        const char *ascii_repr_str = PyUnicode_AsUTF8(ascii_repr);
        if (ascii_repr_str) {
            PyErr_Format(PyExc_TypeError, "%s() got an unexpected keyword argument '%s'", func_name, ascii_repr_str);
        }
        Py_DECREF(ascii_repr);
    }
    assert(PyErr_Occurred());
}

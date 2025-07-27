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

PyObject *ssrjson_Encode(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *ssrjson_EncodeToBytes(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *ssrjson_Decode(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *ssrjson_FileEncode(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *ssrjson_DecodeFile(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *ssrjson_print_current_features(PyObject *self, PyObject *);
PyObject *ssrjson_get_current_features(PyObject *self, PyObject *);

PyObject *JSONDecodeError = NULL;
PyObject *JSONEncodeError = NULL;

static PyMethodDef ssrjson_Methods[] = {
        {"dumps", (PyCFunction)ssrjson_Encode, METH_VARARGS | METH_KEYWORDS, "dumps(obj, indent=None)\n--\n\nConverts arbitrary object recursively into JSON."},
        {"dumps_to_bytes", (PyCFunction)ssrjson_EncodeToBytes, METH_VARARGS | METH_KEYWORDS, "dumps_to_bytes(obj, indent=None)\n--\n\nConverts arbitrary object recursively into JSON."},
        {"loads", (PyCFunction)ssrjson_Decode, METH_VARARGS | METH_KEYWORDS, "loads(s)\n--\n\nConverts JSON as string to dict object structure."},
        {"print_current_features", ssrjson_print_current_features, METH_NOARGS, "print_current_features()\n--\n\nPrints current features."},
        {"get_current_features", ssrjson_get_current_features, METH_NOARGS, "get_current_features()\n--\n\nGet current features."},
        {NULL, NULL, 0, NULL} /* Sentinel */
};

static void module_free(void *m);

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "ssrjson",
        0,               /* m_doc */
        0,               /* m_size */
        ssrjson_Methods, /* m_methods */
        NULL,            /* m_slots */
        NULL,            /* m_traverse */
        NULL,            /* m_clear */
        module_free      /* m_free */
};

static void module_free(void *m) {
    for (size_t i = 0; i < SSRJSON_KEY_CACHE_SIZE; i++) {
        Py_XDECREF(DecodeKeyCache[i].key);
    }

#if defined(Py_GIL_DISABLED)
    if (unlikely(!ssrjson_tls_free())) {
        // critical
        printf("ssrjson: failed to free TLS\n");
    }
#endif

#if SSRJSON_ENABLE_TRACE
    size_t cached = 0;
    for (size_t i = 0; i < SSRJSON_KEY_CACHE_SIZE; i++) {
        if (DecodeKeyCache[i].key) cached++;
    }
    printf("key cache: %zu/%d\n", cached, SSRJSON_KEY_CACHE_SIZE);
#endif // SSRJSON_ENABLE_TRACE
}

#if PY_MINOR_VERSION >= 13
PyTypeObject *PyNone_Type = NULL;

force_inline void _init_PyNone_Type(PyTypeObject *none_type) {
    PyNone_Type = none_type;
}
#endif


PyMODINIT_FUNC PyInit_ssrjson(void) {
    PyObject *module;

    // This function is not supported in PyPy.
    if ((module = PyState_FindModule(&moduledef)) != NULL) {
        Py_INCREF(module);
        return module;
    }

    const char *err = _update_simd_features();
    if (err) {
        PyErr_SetString(PyExc_ImportError, err);
        return NULL;
    }

    module = PyModule_Create(&moduledef);
    if (module == NULL) {
        return NULL;
    }

    PyModule_AddStringConstant(module, "__version__", SSRJSON_VERSION);

    JSONDecodeError = PyErr_NewException("ssrjson.JSONDecodeError", PyExc_ValueError, NULL);
    Py_XINCREF(JSONDecodeError);
    if (PyModule_AddObject(module, "JSONDecodeError", JSONDecodeError) < 0) {
        Py_XDECREF(JSONDecodeError);
        Py_CLEAR(JSONDecodeError);
        Py_DECREF(module);
        return NULL;
    }

    JSONEncodeError = PyErr_NewException("ssrjson.JSONEncodeError", PyExc_ValueError, NULL);
    Py_XINCREF(JSONEncodeError);
    if (PyModule_AddObject(module, "JSONEncodeError", JSONEncodeError) < 0) {
        Py_XDECREF(JSONEncodeError);
        Py_CLEAR(JSONEncodeError);
        Py_DECREF(module);
        return NULL;
    }

#if defined(Py_GIL_DISABLED)
    // TLS init.
    if (unlikely(!ssrjson_tls_init())) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to initialize TLS");
        Py_XDECREF(JSONEncodeError);
        Py_CLEAR(JSONEncodeError);
        Py_DECREF(module);
        return NULL;
    }
#endif

    // do ssrjson internal init.
    memset(DecodeKeyCache, 0, sizeof(DecodeKeyCache));

#if PY_MINOR_VERSION >= 13
    _init_PyNone_Type(Py_TYPE(Py_None));
#endif

    return module;
}

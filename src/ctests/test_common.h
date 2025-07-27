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

force_inline bool initialize_cpython(void) {
    PyObject *sys_module = NULL, *path = NULL, *add_path = NULL;
    //
    Py_Initialize();
    //
    sys_module = PyImport_ImportModule("sys");
    if (!sys_module) goto fail;
    path = PyObject_GetAttrString(sys_module, "path");
    if (!path) goto fail;
    add_path = PyUnicode_FromString(".");
    if (!add_path) goto fail;
    //
    if (0 != PyList_Append(path, add_path)) goto fail;
    //
    Py_DECREF(sys_module);
    Py_DECREF(path);
    Py_DECREF(add_path);
    return true;
fail:;
    Py_XDECREF(sys_module);
    Py_XDECREF(path);
    Py_XDECREF(add_path);
    return false;
}

#ifdef _WIN32
#else
#    include <dlfcn.h>

force_inline PyObject *_make_dlopen_flag_arg(void) {
    PyObject *args = NULL;
    PyObject *flag = NULL;
    args = PyTuple_New(1);
    if (!args) return NULL;
    flag = PyLong_FromLong(RTLD_NOW | RTLD_GLOBAL);
    if (!flag) {
        Py_DECREF(args);
        return NULL;
    }
    PyTuple_SET_ITEM(args, 0, flag);
    return args;
}

force_inline bool set_dlopen_flags(void) {
    PyObject *setdlopenflags = NULL;
    PyObject *args = NULL;
    PyObject *ret = NULL;
    setdlopenflags = PySys_GetObject("setdlopenflags");
    if (!setdlopenflags) return NULL;
    args = _make_dlopen_flag_arg();
    if (!args) goto fail;
    ret = PyObject_Call(setdlopenflags, args, NULL);
    if (!ret) goto fail;
    Py_DECREF(ret);
    Py_DECREF(args);
    Py_DECREF(setdlopenflags);
    return true;
fail:;
    Py_XDECREF(ret);
    Py_XDECREF(args);
    Py_XDECREF(setdlopenflags);
    return false;
}
#endif

// returns a new reference
force_inline PyObject *import_ssrjson(void) {
#ifdef _WIN32
#else
    if (!set_dlopen_flags()) return NULL;
#endif
    PyObject *pModule = PyImport_ImportModule("ssrjson");
    return pModule;
}

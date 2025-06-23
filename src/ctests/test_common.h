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

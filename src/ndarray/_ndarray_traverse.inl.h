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

#ifdef SSRJSON_CLANGD_CHECKING
#    ifndef COMPILE_INDENT_LEVEL
#        include "encode/indent_writer.h"
#        include "ndarray_common.h"
#    endif
#endif

// macro push
#define COMPILE_WRITE_UCS_LEVEL 1
#include "compile_context/iw_in.inl.h"

/*==============================================================================
 * Per-type traverse functions (multi-include _ndarray_traverse_impl.inl.h)
 *============================================================================*/

// bool expands to _Bool in C, so pre-define the fn name to keep "bool" literal in it.
#define _ndarray_traverse_fn ssrjson_concat2(_ndarray_traverse_bool, __INDENT_NAME)
#define NDARRAY_SUFFIX bool
#define NDARRAY_ELEM_T u8
#define NDARRAY_WRITE_ELEM _write_unicode_bool_numpy_u8
#include "_ndarray_traverse_impl.inl.h"

#define NDARRAY_SUFFIX i8
#define NDARRAY_ELEM_T i8
#define NDARRAY_WRITE_ELEM ndarray_write_i8_elem
#include "_ndarray_traverse_impl.inl.h"

#define NDARRAY_SUFFIX i16
#define NDARRAY_ELEM_T i16
#define NDARRAY_WRITE_ELEM ndarray_write_i16_elem
#include "_ndarray_traverse_impl.inl.h"

#define NDARRAY_SUFFIX i32
#define NDARRAY_ELEM_T i32
#define NDARRAY_WRITE_ELEM ndarray_write_i32_elem
#include "_ndarray_traverse_impl.inl.h"

#define NDARRAY_SUFFIX i64
#define NDARRAY_ELEM_T i64
#define NDARRAY_WRITE_ELEM ndarray_write_i64_elem
#include "_ndarray_traverse_impl.inl.h"

#define NDARRAY_SUFFIX u8
#define NDARRAY_ELEM_T u8
#define NDARRAY_WRITE_ELEM ndarray_write_u8_elem
#include "_ndarray_traverse_impl.inl.h"

#define NDARRAY_SUFFIX u16
#define NDARRAY_ELEM_T u16
#define NDARRAY_WRITE_ELEM ndarray_write_u16_elem
#include "_ndarray_traverse_impl.inl.h"

#define NDARRAY_SUFFIX u32
#define NDARRAY_ELEM_T u32
#define NDARRAY_WRITE_ELEM ndarray_write_u32_elem
#include "_ndarray_traverse_impl.inl.h"

#define NDARRAY_SUFFIX u64
#define NDARRAY_ELEM_T u64
#define NDARRAY_WRITE_ELEM ndarray_write_u64_elem
#include "_ndarray_traverse_impl.inl.h"

#define NDARRAY_SUFFIX f16
#define NDARRAY_ELEM_T u16
#define NDARRAY_WRITE_ELEM ndarray_write_f16_elem
#include "_ndarray_traverse_impl.inl.h"

#define NDARRAY_SUFFIX f32
#define NDARRAY_ELEM_T float
#define NDARRAY_WRITE_ELEM ndarray_write_f32_elem
#include "_ndarray_traverse_impl.inl.h"

#define NDARRAY_SUFFIX f64
#define NDARRAY_ELEM_T double
#define NDARRAY_WRITE_ELEM ndarray_write_f64_elem
#include "_ndarray_traverse_impl.inl.h"

#include "_ndarray_reserve_cnt.inl.h"

force_inline usize get_ndarray_reserve_cnt(const PyArrayInterface *array, Py_ssize_t base_nested_depth,
                                           NDATypes ndatype, bool is_in_obj) {
    int nd = array->nd;
    Py_ssize_t *shape = array->shape;
    usize ret = 0;
    // write \n and indent for list
    if (COMPILE_INDENT_LEVEL > 0 && !is_in_obj) { ret += COMPILE_INDENT_LEVEL * base_nested_depth + 1; }
    const usize padding = 10; // reserve some extra padding for safety
    assert(padding >= 3);     // for writing bool, we write 8 bytes at once
    assert(padding >= ssrjson_dtoa_write_length - ssrjson_dtoa_output_maxlen);
    assert(padding >= ssrjson_ftoa_write_length - ssrjson_ftoa_output_maxlen);
    return ret + padding + get_ndarray_reserve_cnt_internal(nd, shape, base_nested_depth + 1, ndatype, is_in_obj);
}

force_inline ssrjson_nofail u8 *ndarray_traverse_dispatch(u8 *writer, const PyArrayInterface *array,
                                                          Py_ssize_t base_nested_depth, NDATypes ndatype) {
    switch (ndatype) {
        case NDA_f64:
            return make_i_name(_ndarray_traverse_f64)(writer, array, base_nested_depth);
        case NDA_f32:
            return make_i_name(_ndarray_traverse_f32)(writer, array, base_nested_depth);
        case NDA_f16:
            return make_i_name(_ndarray_traverse_f16)(writer, array, base_nested_depth);
        case NDA_i64:
            return make_i_name(_ndarray_traverse_i64)(writer, array, base_nested_depth);
        case NDA_i32:
            return make_i_name(_ndarray_traverse_i32)(writer, array, base_nested_depth);
        case NDA_i16:
            return make_i_name(_ndarray_traverse_i16)(writer, array, base_nested_depth);
        case NDA_i8:
            return make_i_name(_ndarray_traverse_i8)(writer, array, base_nested_depth);
        case NDA_u64:
            return make_i_name(_ndarray_traverse_u64)(writer, array, base_nested_depth);
        case NDA_u32:
            return make_i_name(_ndarray_traverse_u32)(writer, array, base_nested_depth);
        case NDA_u16:
            return make_i_name(_ndarray_traverse_u16)(writer, array, base_nested_depth);
        case NDA_u8:
            return make_i_name(_ndarray_traverse_u8)(writer, array, base_nested_depth);
        case NDA_bool:
            return make_i_name(_ndarray_traverse_bool)(writer, array, base_nested_depth);
        default:
            ssrjson_unreachable();
            return writer;
    }
}

/*==============================================================================
 * Public entry point
 *
 * Encodes a numpy ndarray (nd >= 1) into the u8 buffer as JSON.
 * Returns updated writer on success, NULL on error (PyErr set).
 *============================================================================*/

static force_noinline u8 *u8_buffer_append_ndarray(u8 *writer, EncodeUBufInfo *u_buf_info, Py_ssize_t cur_nested_depth,
                                                   PyObject *obj, bool is_in_obj) {
    PyObject *capsule_obj = PyObject_GetAttrString(obj, "__array_struct__");
    if (unlikely(!capsule_obj)) { return NULL; }
    if (unlikely(!PyCapsule_IsValid(capsule_obj, NULL))) {
        Py_DECREF(capsule_obj);
        PyErr_SetString(JSONEncodeError, "__array_struct__ is not a valid PyCapsule");
        return NULL;
    }

    PyArrayInterface *array = (PyArrayInterface *)PyCapsule_GetPointer(capsule_obj, NULL);
    if (unlikely(!array)) {
        Py_DECREF(capsule_obj);
        return NULL;
    }

    if (unlikely(array->two != 2)) {
        Py_DECREF(capsule_obj);
        PyErr_SetString(JSONEncodeError, "Malformed __array_struct__");
        return NULL;
    }

    if (unlikely(!(array->flags & NPY_ARRAY_C_CONTIGUOUS))) {
        Py_DECREF(capsule_obj);
        PyErr_SetString(JSONEncodeError, "numpy array must be C-contiguous");
        return NULL;
    }

    if (unlikely(!(array->flags & NPY_ARRAY_NOTSWAPPED))) {
        Py_DECREF(capsule_obj);
        PyErr_SetString(JSONEncodeError, "numpy array must have native byte order");
        return NULL;
    }

    if (unlikely(array->nd < 1)) {
        Py_DECREF(capsule_obj);
        PyErr_SetString(JSONEncodeError, "numpy 0-d array should not reach ndarray encoder");
        return NULL;
    }
    if (unlikely(array->nd > MAX_NDARRAY_DIMENSION)) {
        Py_DECREF(capsule_obj);
        PyErr_SetString(JSONEncodeError, "numpy array has too many dimensions");
        return NULL;
    }

    NDATypes ndatype = to_ndatype(array->typekind, array->itemsize); /* returns NDA_err if invalid */
    if (unlikely(ndatype == NDA_err)) {
        Py_DECREF(capsule_obj);
        PyErr_SetString(JSONEncodeError, "unsupported numpy dtype");
        return NULL;
    }

    usize reserve_cnt = get_ndarray_reserve_cnt(array, cur_nested_depth, ndatype, is_in_obj);
    writer = u_buf_reserve_u8(writer, u_buf_info, reserve_cnt);
    if (unlikely(!writer)) {
        Py_DECREF(capsule_obj);
        return NULL;
    }

#ifndef NDEBUG
    u8 *const writer_before_traverse = writer;
#endif

#if COMPILE_INDENT_LEVEL > 0
    if (!is_in_obj) { writer = ndarray_write_indent(writer, cur_nested_depth); }
#else
    (void)is_in_obj;
#endif

    writer = ndarray_traverse_dispatch(writer, array, cur_nested_depth, ndatype);
    assert(writer <= writer_before_traverse + reserve_cnt);
    Py_DECREF(capsule_obj);
    return writer;
}

// macro pop
#include "compile_context/iw_out.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

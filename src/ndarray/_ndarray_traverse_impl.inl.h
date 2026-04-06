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

#include "ndarray_common.h"
/*
 * Required macros:
 *   NDARRAY_SUFFIX     - function name suffix
 *   NDARRAY_ELEM_T     - C type to read from array data
 *   NDARRAY_WRITE_ELEM - element writer function name
 */

#ifdef SSRJSON_CLANGD_CHECKING
#    ifndef COMPILE_INDENT_LEVEL
#        include "encode/encode_shared.h"
#        include "encode/indent_writer.h"
#        define NDARRAY_SUFFIX i32
#        define NDARRAY_ELEM_T i32
#        define NDARRAY_WRITE_ELEM ndarray_write_i32_elem
#        define COMPILE_INDENT_LEVEL 0
#        include "compile_context/iw_in.inl.h"
#    endif
#endif

#ifndef _ndarray_traverse_fn
#    define _ndarray_traverse_fn ssrjson_concat3(_ndarray_traverse, NDARRAY_SUFFIX, __INDENT_NAME)
#endif

/*
 * Stack-based iterative traversal for a single element type.
 *
 * The buffer is pre-reserved by the caller, so all writes are ssrjson_nofail.
 *
 * Stack model:
 *   - depth 0..nd-1 maps to array dimensions
 *   - non-leaf depth: pop one child from current frame, push new frame, write '['
 *   - leaf depth (nd-1): write all elements inline, set remaining = 0
 *   - remaining == 0: close bracket, pop frame
 */
static ssrjson_nofail u8 *_ndarray_traverse_fn(
        u8 *writer,
        const PyArrayInterface *array,
        Py_ssize_t base_nested_depth) {
    NdarrayFrame stack[MAX_NDARRAY_DIMENSION];
    int nd = array->nd;
    Py_ssize_t *shape = array->shape;
    Py_ssize_t *strides = array->strides;
    int depth = 0;

    stack[0].data_ptr = (const char *)array->data;
    stack[0].remaining = shape[0];
    stack[0].stride = strides[0];

    /* outermost '[' (indent handled by caller) */
    *writer++ = '[';

    for (;;) {
        if (stack[depth].remaining == 0) {
            /* close current dimension */
            if (shape[depth] > 0) {
                /* non-empty: back over trailing comma from last child */
                writer--;
                writer = ndarray_write_indent(writer, base_nested_depth + depth);
            }
            *writer++ = ']';
            *writer++ = ',';
            if (depth == 0) break;
            depth--;
            continue;
        }

        if (depth == nd - 1) {
            /* leaf dimension: write all elements */
            const char *p = stack[depth].data_ptr;
            Py_intptr_t stride = stack[depth].stride;
            Py_intptr_t n = stack[depth].remaining;
            Py_ssize_t elem_indent = base_nested_depth + depth + 1;
            for (Py_intptr_t i = 0; i < n; i++) {
                writer = ndarray_write_indent(writer, elem_indent);
                writer = NDARRAY_WRITE_ELEM(writer, *(const NDARRAY_ELEM_T *)(p + i * stride));
            }
            stack[depth].remaining = 0;
        } else {
            /* non-leaf: take next child, push new frame */
            const char *child_data = stack[depth].data_ptr;
            stack[depth].data_ptr += stack[depth].stride;
            stack[depth].remaining--;

            depth++;
            stack[depth].data_ptr = child_data;
            stack[depth].remaining = shape[depth];
            stack[depth].stride = strides[depth];

            writer = ndarray_write_indent(writer, base_nested_depth + depth);
            *writer++ = '[';
        }
    }

    return writer;
}

#undef _ndarray_traverse_fn
#undef NDARRAY_ELEM_T
#undef NDARRAY_WRITE_ELEM
#undef NDARRAY_SUFFIX

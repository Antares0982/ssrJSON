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
#        include "ndarray_common.h"
#        define COMPILE_INDENT_LEVEL 2
#        define COMPILE_WRITE_UCS_LEVEL 1
#        include "compile_context/iw_in.inl.h"
#    endif
#endif

// Requires: iw context active, ndarray_common.h included.

force_inline usize get_1darray_reserve_cnt(usize cur_nested_depth, usize length, NDATypes ndatype) {
#if COMPILE_INDENT_LEVEL > 0
    if (unlikely(!length)) { return 3; }
    // pre indent size (with '\n')
    usize cnt = (cur_nested_depth * COMPILE_INDENT_LEVEL + 1) * (length + 1);
    // elements
    cnt += length * (get_elem_write_size(ndatype) + 1 + COMPILE_INDENT_LEVEL); // comma and extra indent
    // brackets and last comma, also remove comma of last element
    cnt += 3 - 1;
    return cnt;
#else
    // calculate branchlessly
    return length * (get_elem_write_size(ndatype) + 1) + 3 - 1 +
           !length; // elements with comma, plus brackets and last comma, also remove comma of last element
#endif
}

force_inline usize get_ndarray_reserve_cnt_internal(int nd, Py_ssize_t *shape, usize cur_nested_depth, NDATypes ndatype,
                                                    bool is_in_obj) {
    if (nd == 1) return get_1darray_reserve_cnt(cur_nested_depth, shape[0], ndatype);

    struct {
        usize length;
#if COMPILE_INDENT_LEVEL > 0
        usize depth;
#endif
    } stack[MAX_NDARRAY_DIMENSION];

    int top = 0;

    // walk forward, pushing each dimension until we reach the base case
    usize cnt;
    while (nd > 1) {
        usize length = shape[0];
        if (unlikely(!length)) {
            cnt = 3; // empty array: "[],"
            goto unwind;
        }
        stack[top].length = length;
#if COMPILE_INDENT_LEVEL > 0
        stack[top].depth = cur_nested_depth;
#endif
        top++;
        nd--;
        shape++;
        cur_nested_depth++;
    }

    // base case: 1-d array
    cnt = get_1darray_reserve_cnt(cur_nested_depth, shape[0], ndatype);
unwind:

    // unwind: accumulate each outer dimension back
    while (top > 0) {
        top--;
        usize length = stack[top].length;
#if COMPILE_INDENT_LEVEL > 0
        usize depth = stack[top].depth;
        cnt = cnt * length                                              // inner arrays
              + ((depth + 1) * COMPILE_INDENT_LEVEL + 1) * (length - 1) // indent between inner arrays
              + ((depth + 1) * COMPILE_INDENT_LEVEL + 1) + 1            // first bracket + '\n' + indent
              + (depth * COMPILE_INDENT_LEVEL + 1) + 2 - 1; // last bracket + indent + comma - last inner comma
#else
        cnt = cnt * length + 1 + 2 - 1; // inner arrays + bracket + bracket + comma - last inner comma
#endif
    }

    return cnt;
}

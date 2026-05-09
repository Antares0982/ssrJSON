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

#include "compile_context/iw_in.inl.h"
#include "ndarray/_ndarray_reserve_cnt.inl.h"

static usize test_get_ndarray_reserve_cnt_reference(int nd, Py_ssize_t *shape, usize cur_nested_depth, NDATypes ndatype,
                                                    bool is_in_obj) {
    if (nd == 1) {
#if COMPILE_INDENT_LEVEL > 0
        usize length = shape[0];
        if (!length) { return 3; }
        // pre indent size (with '\n')
        usize cnt = (cur_nested_depth * COMPILE_INDENT_LEVEL + 1) * (length + 1);
        // elements
        cnt += length * (get_elem_write_size(ndatype) + 1 + COMPILE_INDENT_LEVEL); // comma and extra indent
        // brackets and last comma, also remove comma of last element
        cnt += 3 - 1;
        return cnt;
#else
        usize length = shape[0];
        if (!length) { return 3; }
        return length * (get_elem_write_size(ndatype) + 1) + 3 -
               1; // elements with comma, plus brackets and last comma, also remove comma of last element
#endif
    } else {
#if COMPILE_INDENT_LEVEL > 0
        usize length = shape[0];
        if (!length) { return 3; }
        usize inner_cnt = test_get_ndarray_reserve_cnt_reference(
                nd - 1, shape + 1, cur_nested_depth + 1, ndatype, is_in_obj);
        usize cnt = inner_cnt * length; // inner arrays with comma
        // each indent between two inner arrays
        cnt += ((cur_nested_depth + 1) * COMPILE_INDENT_LEVEL + 1) * (length - 1);
        // first bracket, the '\n' and indent after it
        cnt += ((cur_nested_depth + 1) * COMPILE_INDENT_LEVEL + 1) + 1;
        // last bracket, last indent and comma, also remove comma of last inner array
        cnt += (cur_nested_depth * COMPILE_INDENT_LEVEL + 1) + 2 - 1;
        return cnt;
#else
        usize length = shape[0];
        if (!length) { return 3; }
        usize inner_cnt = test_get_ndarray_reserve_cnt_reference(
                nd - 1, shape + 1, cur_nested_depth + 1, ndatype, is_in_obj);
        usize cnt = inner_cnt * length; // inner arrays with comma
        // first bracket
        cnt += 1;
        // last bracket and comma, also remove comma of last inner array
        cnt += 2 - 1;
        return cnt;
#endif
    }
}

#define TEST_NDARRAY_RESERVE_ROUNDS 1000
#define TEST_NDARRAY_RESERVE_MAX_ND 8
#define TEST_NDARRAY_RESERVE_MAX_DIM_SIZE 16

int test_ndarray_reserve_cnt(void) {
    NDATypes all_types[] = {
            NDA_f64, NDA_f32, NDA_f16, NDA_i64, NDA_i32, NDA_i16, NDA_i8, NDA_u64, NDA_u32, NDA_u16, NDA_u8, NDA_bool};
    int num_types = sizeof(all_types) / sizeof(all_types[0]);

    for (int round = 0; round < TEST_NDARRAY_RESERVE_ROUNDS; round++) {
        int nd = 1 + (rand() % TEST_NDARRAY_RESERVE_MAX_ND);
        Py_ssize_t shape[TEST_NDARRAY_RESERVE_MAX_ND];
        for (int i = 0; i < nd; i++) {
            shape[i] = rand() % (TEST_NDARRAY_RESERVE_MAX_DIM_SIZE + 1); // 0 to MAX_DIM_SIZE
        }
        usize depth = rand() % 4;
        NDATypes ndatype = all_types[rand() % num_types];
        bool is_in_obj = rand() & 1;

        usize expected = test_get_ndarray_reserve_cnt_reference(nd, shape, depth, ndatype, is_in_obj);
        usize actual = get_ndarray_reserve_cnt_internal(nd, shape, depth, ndatype, is_in_obj);

        if (expected != actual) {
            printf("\n  FAIL: indent=%d nd=%d depth=%zu ndatype=%d is_in_obj=%d shape=[", COMPILE_INDENT_LEVEL, nd,
                   depth, (int)ndatype, is_in_obj);
            for (int i = 0; i < nd; i++) {
                if (i > 0) printf(",");
                printf("%zd", shape[i]);
            }
            printf("] expected=%zu actual=%zu\n", expected, actual);
            return FAILED;
        }
    }
    return PASSED;
}

#undef TEST_NDARRAY_RESERVE_ROUNDS
#undef TEST_NDARRAY_RESERVE_MAX_ND
#undef TEST_NDARRAY_RESERVE_MAX_DIM_SIZE

#include "compile_context/iw_out.inl.h"

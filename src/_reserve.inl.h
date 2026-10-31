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
#    include "encode/encode_shared.h"
#    include "utils/unicode.h"
#endif

#include "compile_context/w_in.inl.h"

force_inline dst_t *u_buf_reserve(dst_t *writer, EncodeUBufInfo *u_buf_info, usize size) {
#ifdef SSRJSON_64BIT
    if (__builtin_constant_p(size) && size <= SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE / sizeof(dst_t)) {
        // Buffers only grow from their initial capacity.
        uintptr_t limit = ssrjson_cast(uintptr_t, u_buf_info->end) - size * sizeof(dst_t);
        if (likely(ssrjson_cast(uintptr_t, writer) <= limit)) return writer;
    } else {
        Py_ssize_t avail = ssrjson_cast(u8 *, u_buf_info->end) - ssrjson_cast(u8 *, writer);
        if (likely(size <= PY_SSIZE_T_MAX / sizeof(dst_t) && ssrjson_cast(Py_ssize_t, size * sizeof(dst_t)) <= avail)) {
            return writer;
        }
    }
#endif
    usize offset = ssrjson_cast(u8 *, writer) - ssrjson_cast(u8 *, u_buf_info->head);
    usize capacity = ssrjson_cast(u8 *, u_buf_info->end) - ssrjson_cast(u8 *, u_buf_info->head);
#ifdef SSRJSON_32BIT
    if (likely(offset <= capacity && size <= (capacity - offset) / sizeof(dst_t))) { return writer; }
#endif
    usize target_size;
    if (unlikely(size_mul_add_overflow(size, sizeof(dst_t), offset, &target_size))) {
        PyErr_NoMemory();
        return NULL;
    }
    if (unlikely(target_size > PY_SSIZE_T_MAX)) {
        PyErr_NoMemory();
        return NULL;
    }
    if (unlikely(target_size > capacity)) {
        EncodeUBufInfo new_info = _u_buf_reserve(*u_buf_info, target_size);
        return_if_unlikely(!new_info.head);
        *u_buf_info = new_info;
        writer = ssrjson_cast(dst_t *, ssrjson_cast(u8 *, u_buf_info->head) + offset);
    }
    return writer;
}

#include "compile_context/w_out.inl.h"

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
#    ifndef COMPILE_CONTEXT_ENCODE
#        define COMPILE_CONTEXT_ENCODE
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        include "encode_shared.h"
#        define COMPILE_WRITE_UCS_LEVEL 1
#        include "simd/compile_feature_check.h"
#    endif
#endif

#include "compile_context/w_in.inl.h"

force_inline ssrjson_nofail dst_t *write_unicode_bool(dst_t *writer, bool is_false) {
    static const dst_t true_buf[8] = {'t', 'r', 'u', 'e', ',', 0, 0, 0};
    static const dst_t false_buf[8] = {'f', 'a', 'l', 's', 'e', ',', 0, 0};
    const dst_t *copy_from = is_false ? false_buf : true_buf;
    memcpy(writer, copy_from, _WriteBoolCopyCnt * sizeof(dst_t));
    writer += 5 + is_false;
    return writer;
}

force_inline ssrjson_nofail dst_t *write_unicode_bool_numpy(dst_t *writer, bool is_true) {
    return write_unicode_bool(writer, !is_true);
}

force_inline ssrjson_nofail dst_t *write_unicode_null(dst_t *writer) {
    // ucs case       -> 1, 2, 4
    // expected bytes -> 5,10,20
    // written bytes  -> 8,16,24/32
    // written count  -> 8, 8,6/8
    // reserve count = 8
    *writer++ = 'n';
    *writer++ = 'u';
    *writer++ = 'l';
    *writer++ = 'l';
    *writer++ = ',';
    dst_t *writer2 = writer;
#if COMPILE_WRITE_UCS_LEVEL < 4
    *writer2++ = 0;
    *writer2++ = 0;
    *writer2++ = 0;
#else // COMPILE_WRITE_UCS_LEVEL == 4
    *writer2++ = 0;
#    if SSRJSON_IS_AARCH64 || _CompileVectorBits >= 256
    *writer2++ = 0;
    *writer2++ = 0;
#    endif
#endif // COMPILE_WRITE_UCS_LEVEL
    return writer;
}

force_inline ssrjson_nofail dst_t *write_unicode_empty_arr(dst_t *writer) {
    // reserve count = 4
    *writer++ = '[';
    *writer++ = ']';
    *writer++ = ',';
#if COMPILE_WRITE_UCS_LEVEL != 4
    *writer = 0;
#endif
    return writer;
}

force_inline ssrjson_nofail dst_t *write_unicode_arr_begin(dst_t *writer) {
    // reserve count = 1
    *writer++ = '[';
    return writer;
}

force_inline ssrjson_nofail dst_t *write_unicode_empty_obj(dst_t *writer) {
    // reserve count = 4
    *writer++ = '{';
    *writer++ = '}';
    *writer++ = ',';
#if COMPILE_WRITE_UCS_LEVEL != 4
    *writer = 0;
#endif
    return writer;
}

force_inline ssrjson_nofail dst_t *write_unicode_obj_begin(dst_t *writer) {
    // reserve count = 1
    *writer++ = '{';
    return writer;
}

force_inline ssrjson_nofail dst_t *write_unicode_obj_end(dst_t *writer) {
    // reserve count = 2
    *writer++ = '}';
    *writer++ = ',';
    return writer;
}

force_inline ssrjson_nofail dst_t *write_unicode_arr_end(dst_t *writer) {
    // reserve count = 2
    *writer++ = ']';
    *writer++ = ',';
    return writer;
}

#include "compile_context/w_out.inl.h"

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
#        include "encode_shared.h"
#        include "simd/simd_detect.h"
#        include "simd/simd_impl.h"
#        include "utils/unicode.h"
#        define COMPILE_READ_UCS_LEVEL 1
#        define COMPILE_WRITE_UCS_LEVEL 1
#        undef USING_AVX512
#        define USING_AVX512 0
#        include "simd/compile_feature_check.h"
#    endif
#endif

/* Macro IN */
#include "compile_context/srw_in.inl.h"

static force_noinline ssrjson_nofail dst_t *encode_unicode_noinline(dst_t *writer, const src_t *str_data, usize len) {
    return encode_unicode_impl(writer, str_data, len);
}

// call u_buf_apd_key_rsv_idt before calling this.
force_inline ssrjson_nofail dst_t *u_buf_apd_key_impl(dst_t *writer, const src_t *str_data, usize len,
                                                      ssrjson_compiletime bool is_compact) {
    *writer++ = '"';
    if (ssrjson_consteval(!USING_AVX512 && !is_compact) && len < 16 / COMPILE_READ_UCS_LEVEL) {
        writer = encode_scalar(writer, str_data, len);
    } else {
        writer = encode_unicode_noinline(writer, str_data, len);
    }
    *writer++ = '"';
    *writer++ = ':';
    return writer;
}

// call u_buf_apd_str_rsv_idt before calling this.
force_inline ssrjson_nofail dst_t *u_buf_apd_str_impl(dst_t *writer, const src_t *str_data, usize len,
                                                      ssrjson_compiletime bool is_compact) {
    *writer++ = '"';
    if (ssrjson_consteval(!USING_AVX512 && !is_compact) && len < 16 / COMPILE_READ_UCS_LEVEL) {
        writer = encode_scalar(writer, str_data, len);
    } else {
        writer = encode_unicode_noinline(writer, str_data, len);
    }
    *writer++ = '"';
    *writer++ = ',';
    return writer;
}

#include "compile_context/srw_out.inl.h"

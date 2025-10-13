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

#ifdef SSRJSON_CLANGD_DUMMY
#    ifndef COMPILE_UCS_LEVEL
#        include "decode/str/decoder_impl_wrap.h"
#        include "simd/simd_impl.h"
#        include "simd/union_vector.h"
#        define COMPILE_UCS_LEVEL 1
#        define COMPILE_WRITE_UCS_LEVEL 1
#        include "simd/compile_feature_check.h"
#    endif
#endif

#define COMPILE_READ_UCS_LEVEL COMPILE_UCS_LEVEL
#include "compile_context/srw_in.inl.h"

force_inline int decode_str_copy_loop4(_dst_t **dst_addr, const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr, vector_a *maxvec_addr) {
    int ret;
    //
    unionvector_a_x4 vec;
    anymask_t check_mask[4];
    anymask_t check_mask_total;
    //
    _decode_str_loop4_read_src_impl(*src_addr, &vec, check_mask, &check_mask_total);
    _dst_t *const dst = *dst_addr;
    cvt_to_dst(dst + 0 * READ_BATCH_COUNT, vec.x[0]);
    cvt_to_dst(dst + 1 * READ_BATCH_COUNT, vec.x[1]);
    cvt_to_dst(dst + 2 * READ_BATCH_COUNT, vec.x[2]);
    cvt_to_dst(dst + 3 * READ_BATCH_COUNT, vec.x[3]);
    usize moved_count = _decode_str_loop4_decoder_impl(src_addr, src_end, check_mask, check_mask_total, &ret, true, maxvec_addr, vec, escapeval_addr);
    *dst_addr += moved_count;
    return ret;
}

force_inline int decode_str_copy_loop(_dst_t **dst_addr, const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr, vector_a *maxvec_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_loop_read_src_impl(*src_addr, &vec, &check_mask);
    _dst_t *const dst = *dst_addr;
    cvt_to_dst(*dst_addr, vec);
    usize moved_count = _decode_str_loop_decoder_impl(src_addr, src_end, check_mask, &ret, true, maxvec_addr, vec, escapeval_addr);
    *dst_addr += moved_count;
    return ret;
}

force_inline int decode_str_copy_trailing(_dst_t **dst_addr, const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escape_info_addr, vector_a *maxvec_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_trailing_read_src_impl(*src_addr, src_end, &vec, &check_mask);
#if SSRJSON_X86 && COMPILE_SIMD_BITS == 256
    avx2_trailing_cvt(*src_addr, src_end, *dst_addr);
#else
    cvt_to_dst(*dst_addr, vec);
#endif
    usize done_count = _decode_str_trailing_decoder_impl(src_addr, src_end, check_mask, &ret, false, maxvec_addr, vec, escape_info_addr);
    *dst_addr += done_count;
    //
    return ret;
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_READ_UCS_LEVEL

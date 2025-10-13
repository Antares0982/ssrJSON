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
#    ifndef COMPILE_READ_UCS_LEVEL
#        include "decode/decode_shared.h"
#        include "simd/union_vector.h"
//
#        define COMPILE_READ_UCS_LEVEL 1
#        include "simd/compile_feature_check.h"
//
#        include "_sr_impls.inl.h"
#    endif
#endif

#include "compile_context/sr_in.inl.h"

force_inline void unsigned_max4(vector_a *max_vec_addr, unionvector_a_x4 vec) {
    *max_vec_addr = unsigned_max(*max_vec_addr, unsigned_max(unsigned_max(vec.x[0], vec.x[1]), unsigned_max(vec.x[2], vec.x[3])));
}

/* Read src. */
force_inline void _decode_str_loop4_read_src_impl(
        const _src_t *src,
        unionvector_a_x4 *out_vec,
        anymask_t *out_check_mask_arr4,
        anymask_t *out_check_mask_total) {
    for (int i = 0; i < 4; ++i) {
        out_vec->x[i] = *(SSRJSON_CAST(vector_u *, src) + i);
    }
    for (int i = 0; i < 4; ++i) {
        out_check_mask_arr4[i] = get_escape_anymask(out_vec->x[i]);
    }
    *out_check_mask_total = (out_check_mask_arr4[0] | out_check_mask_arr4[1]) | (out_check_mask_arr4[2] | out_check_mask_arr4[3]);
}

force_inline void _decode_str_loop_read_src_impl(
        const _src_t *src,
        vector_a *out_vec,
        anymask_t *out_check_mask) {
    *out_vec = *SSRJSON_CAST(vector_u *, src);
    *out_check_mask = get_escape_anymask(*out_vec);
}

force_inline void _decode_str_trailing_read_src_impl(
        const _src_t *src,
        const _src_t *src_end,
        vector_a *out_vec,
        anymask_t *out_check_mask) {
    usize trailing_len = src_end - src;
    assert(trailing_len < READ_BATCH_COUNT);
#if SSRJSON_X86 && COMPILE_SIMD_BITS == 512
    usize maskz = len_to_maskz(src_end - src);
    *out_vec = maskz_loadu(maskz, src);
    *out_check_mask = maskz & get_escape_bitmask(*out_vec);
#elif SSRJSON_X86 && COMPILE_SIMD_BITS == 256
    vector_a vec = *(vector_u *)(src_end - READ_BATCH_COUNT);
    *out_vec = high_mask(vec, trailing_len);
    *out_check_mask = high_mask(get_escape_mask(vec), trailing_len);
#elif SSRJSON_X86
    vector_a vec = *(vector_u *)(src_end - READ_BATCH_COUNT);
    *out_vec = runtime_byte_rshift_128(vec, (READ_BATCH_COUNT - trailing_len) * sizeof(_src_t));
    *out_check_mask = low_mask(get_escape_mask(*out_vec), trailing_len);
#elif SSRJSON_AARCH
    vector_a vec = *(vector_u *)(src_end - READ_BATCH_COUNT);
    *out_vec = runtime_byte_rshift_128(vec, (READ_BATCH_COUNT - trailing_len) * sizeof(_src_t));
    *out_check_mask = low_mask(get_escape_mask(*out_vec), trailing_len);
#endif
}

/* String decoder. */
force_inline usize _decode_str_loop4_decoder_impl(
        const _src_t **src_addr,
        const _src_t *src_end,
        anymask_t *check_mask_arr4,
        anymask_t check_mask_total,
        int *ret_addr,
        bool inline_escape, // immediate
        vector_a *track_max,
        unionvector_a_x4 src_vecs,
        EscapeInfo *escapeval_addr) {
    const _src_t *src = *src_addr;
    usize done_count;
    if (testz_escape_mask(check_mask_total)) {
        src += 4 * READ_BATCH_COUNT;
        done_count = 4 * READ_BATCH_COUNT;
        *ret_addr = DECODE_LOOPSTATE_CONTINUE;
        if (track_max) { // compile time determined
            unsigned_max4(track_max, src_vecs);
        }
    } else {
        if (track_max) { // compile time determined
            done_count = joined4_escape_anymask_to_done_count_track_max(check_mask_arr4[0], check_mask_arr4[1], check_mask_arr4[2], check_mask_arr4[3], track_max, src_vecs);
        } else {
            done_count = joined4_escape_anymask_to_done_count(check_mask_arr4[0], check_mask_arr4[1], check_mask_arr4[2], check_mask_arr4[3]);
        }
        src += done_count;
        _src_t unicode = *src;
        if (unicode == _Quote) {
            *ret_addr = DECODE_LOOPSTATE_END;
        } else if (unicode == _Slash) {
            if (inline_escape) { // compile time determined
                *escapeval_addr = do_decode_escape(src, src_end);
            } else {
                *escapeval_addr = do_decode_escape_noinline(src, src_end);
            }
            bool is_invalid = escapeval_addr->escape_val == _DECODE_UNICODE_ERR;
            *ret_addr = DECODE_LOOPSTATE_ESCAPE + is_invalid;
        } else {
            assert(unicode < ControlMax);
            PyErr_SetString(JSONDecodeError, "Invalid control character in string");
            *ret_addr = DECODE_LOOPSTATE_INVALID;
        }
        assert(*ret_addr != DECODE_LOOPSTATE_INVALID || PyErr_Occurred());
    }
    *src_addr = src;
    return done_count;
}

force_inline usize _decode_str_loop_decoder_impl(
        const _src_t **src_addr,
        const _src_t *src_end,
        anymask_t check_mask,
        int *ret_addr,
        bool inline_escape, // immediate
        vector_a *track_max,
        vector_a src_vec,
        EscapeInfo *escapeval_addr) {
    usize done_count;
    const _src_t *src = *src_addr;
    if (testz_escape_mask(check_mask)) {
        done_count = READ_BATCH_COUNT;
        src += READ_BATCH_COUNT;
        *ret_addr = DECODE_LOOPSTATE_CONTINUE;
        if (track_max) { // compile time determined
            *track_max = unsigned_max(*track_max, src_vec);
        }
    } else {
        if (track_max) { // compile time determined
            done_count = escape_anymask_to_done_count_track_max(check_mask, track_max, src_vec);
        } else {
            done_count = escape_anymask_to_done_count(check_mask);
        }
        src += done_count;
        _src_t unicode = *src;
        if (unicode == _Quote) {
            *ret_addr = DECODE_LOOPSTATE_END;
        } else if (unicode == _Slash) {
            if (inline_escape) { // compile time determined
                *escapeval_addr = do_decode_escape(src, src_end);
            } else {
                *escapeval_addr = do_decode_escape_noinline(src, src_end);
            }
            bool is_invalid = escapeval_addr->escape_val == _DECODE_UNICODE_ERR;
            *ret_addr = DECODE_LOOPSTATE_ESCAPE + is_invalid;
        } else {
            assert(unicode < ControlMax);
            PyErr_SetString(JSONDecodeError, "Invalid control character in string");
            *ret_addr = DECODE_LOOPSTATE_INVALID;
        }
        assert(*ret_addr != DECODE_LOOPSTATE_INVALID || PyErr_Occurred());
    }
    *src_addr = src;
    return done_count;
}

force_inline usize _decode_str_trailing_decoder_impl(
        const _src_t **src_addr,
        const _src_t *src_end,
        anymask_t check_mask,
        int *ret_addr,
        bool inline_escape, // immediate
        vector_a *track_max,
        vector_a src_vec,
        EscapeInfo *escapeval_addr) {
#if SSRJSON_X86 && COMPILE_SIMD_BITS == 256
#    define BACK_LOAD 1
#else
#    define BACK_LOAD 0
#endif
    usize done_count;
    const _src_t *src = *src_addr;
    usize trailing_len = src_end - src;
    if (unlikely(testz_escape_mask(check_mask))) {
        // err, no string ending in src
        done_count = 0;
        src += READ_BATCH_COUNT;
        PyErr_SetString(JSONDecodeError, "Unexpected ending in string");
        *ret_addr = DECODE_LOOPSTATE_INVALID;
    } else {
        done_count = escape_anymask_to_done_count(check_mask);
        if (track_max) { // compile time determined
            *track_max = unsigned_max(*track_max, low_mask(src_vec, done_count));
        }
        if (BACK_LOAD) { // compile time determined
            done_count -= READ_BATCH_COUNT - trailing_len;
        }
        src += done_count;
        _src_t unicode = *src;
        if (unicode == _Quote) {
            *ret_addr = DECODE_LOOPSTATE_END;
        } else if (unicode == _Slash) {
            if (inline_escape) { // compile time determined
                *escapeval_addr = do_decode_escape(src, src_end);
            } else {
                *escapeval_addr = do_decode_escape_noinline(src, src_end);
            }
            bool is_invalid = escapeval_addr->escape_val == _DECODE_UNICODE_ERR;
            *ret_addr = DECODE_LOOPSTATE_ESCAPE + is_invalid;
        } else {
            assert(unicode < ControlMax);
            PyErr_SetString(JSONDecodeError, "Invalid control character in string");
            *ret_addr = DECODE_LOOPSTATE_INVALID;
        }
        assert(*ret_addr != DECODE_LOOPSTATE_INVALID || PyErr_Occurred());
    }
    *src_addr = src;
    return done_count;
#undef BACK_LOAD
}

#include "compile_context/sr_out.inl.h"

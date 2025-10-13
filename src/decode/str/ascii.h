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

#ifndef SSRJSON_DECODE_STR_ASCII_H
#define SSRJSON_DECODE_STR_ASCII_H
#include "xxhash.h"

#include "decode/decode_shared.h"
#include "decoder_impl_wrap.h"
#include "simd/cvt.h"
#include "simd/memcpy.h"
#include "simd/simd_impl.h"
#include "simd/union_vector.h"
#include "utils/unicode.h"

#define COMPILE_UCS_LEVEL 1
#define COMPILE_READ_UCS_LEVEL 1
#include "simd/compile_feature_check.h"
//
#include "compile_context/sr_in.inl.h"

force_inline PyObject *make_unicode_from_src_ascii(const _src_t *start, usize count, bool is_key) {
    PyObject *ret;
    decode_keyhash_t hash;
    bool should_cache = is_key && count <= 64;
    bool should_hash = is_key && count > 0;
    if (should_cache) {
        hash = XXH3_64bits(start, count);
        ret = get_key_cache(start, hash, count, 0);
        if (ret) {
            goto done;
        }
    }
    ret = create_empty_unicode(count, 0);
    if (likely(ret)) {
        u8 *const target = PYUNICODE_ASCII_START(ret);
        ssrjson_memcpy(target, start, count);
        if (should_cache) {
            add_key_cache(hash, ret, count, 0);
        }
        if (should_hash) {
            assert(count && SSRJSON_CAST(PyASCIIObject *, ret)->hash == -1);
            make_hash(SSRJSON_CAST(PyASCIIObject *, ret), start, count);
        }
    }
done:;
    return ret;
}

/* Slow path unicode maker, which never caches. */
force_inline PyObject *make_unicode_ucs1(const _src_t *start, usize count, bool is_key) {
    assert(count);
    PyObject *ret;
    bool should_hash = is_key;
    ret = create_empty_unicode(count, 1);
    if (likely(ret)) {
        u8 *const target = PYUNICODE_UCS1_START(ret);
        memcpy(target, start, count); // not use inline version
        if (should_hash) {
            assert(count && SSRJSON_CAST(PyASCIIObject *, ret)->hash == -1);
            make_hash(SSRJSON_CAST(PyASCIIObject *, ret), start, count);
        }
    }
done:;
    return ret;
}

/* Slow path unicode maker, which never caches. */
force_inline PyObject *make_unicode_ucs2(void *src_buffer, usize u8count, usize total_count, bool is_key) {
    assert(total_count > 0 && total_count >= u8count);
    PyObject *ret;
    bool should_hash = is_key;
    ret = create_empty_unicode(total_count, 2);
    if (likely(ret)) {
        u16 *const target = PYUNICODE_UCS2_START(ret);
        // not use inline version
        if (u8count) {
            SIMD_NAME_MODIFIER(long_cvt_noinline_u8_u16)(target, src_buffer, u8count);
        }
        memcpy(target + u8count, SSRJSON_CAST(u16 *, src_buffer) + u8count, (total_count - u8count) * 2);
        if (should_hash) {
            assert(total_count && SSRJSON_CAST(PyASCIIObject *, ret)->hash == -1);
            make_hash(SSRJSON_CAST(PyASCIIObject *, ret), target, total_count * 2);
        }
    }
done:;
    return ret;
}

/* Slow path unicode maker, which never caches. */
force_inline PyObject *make_unicode_ucs4(void *src_buffer, usize u8count, usize u16count, usize total_count, bool is_key) {
    assert(total_count > 0 && total_count >= u8count + u16count);
    PyObject *ret;
    bool should_hash = is_key;
    ret = create_empty_unicode(total_count, 4);
    if (likely(ret)) {
        u32 *const target = PYUNICODE_UCS4_START(ret);
        // not use inline version
        if (u8count) {
            SIMD_NAME_MODIFIER(long_cvt_noinline_u8_u32)(target, src_buffer, u8count);
        }
        if (u16count) {
            SIMD_NAME_MODIFIER(long_cvt_noinline_u16_u32)(target + u8count, SSRJSON_CAST(u16 *, src_buffer) + u8count, u16count);
        }
        memcpy(target + u8count + u16count, SSRJSON_CAST(u32 *, src_buffer) + u8count + u16count, (total_count - u8count - u16count) * 4);
        if (should_hash) {
            assert(total_count && SSRJSON_CAST(PyASCIIObject *, ret)->hash == -1);
            make_hash(SSRJSON_CAST(PyASCIIObject *, ret), target, total_count * 4);
        }
    }
done:;
    return ret;
}

force_inline int decode_str_fast_loop4_ascii(const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr) {
    int ret;
    //
    unionvector_a_x4 vec;
    anymask_t check_mask[4];
    anymask_t check_mask_total;
    //
    _decode_str_loop4_read_src_impl(*src_addr, &vec, check_mask, &check_mask_total);
    _decode_str_loop4_decoder_impl(src_addr, src_end, check_mask, check_mask_total, &ret, true, NULL, vec, escapeval_addr);
    //
    return ret;
}

force_inline int decode_str_fast_loop_ascii(const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_loop_read_src_impl(*src_addr, &vec, &check_mask);
    _decode_str_loop_decoder_impl(src_addr, src_end, check_mask, &ret, true, NULL, vec, escapeval_addr);
    //
    return ret;
}

force_inline int decode_str_fast_trailing_ascii(const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escape_info_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_trailing_read_src_impl(*src_addr, src_end, &vec, &check_mask);
    _decode_str_trailing_decoder_impl(src_addr, src_end, check_mask, &ret, true, NULL, vec, escape_info_addr);
    //
    return ret;
}

force_inline int decode_str_copy_loop4_ascii_u8(u8 **dst_addr, const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr) {
    int ret;
    //
    unionvector_a_x4 vec;
    anymask_t check_mask[4];
    anymask_t check_mask_total;
    //
    _decode_str_loop4_read_src_impl(*src_addr, &vec, check_mask, &check_mask_total);
    memcpy(*dst_addr, &vec, sizeof(unionvector_a_x4));
    usize moved_count = _decode_str_loop4_decoder_impl(src_addr, src_end, check_mask, check_mask_total, &ret, true, NULL, vec, escapeval_addr);
    *dst_addr += moved_count;
    return ret;
}

force_inline int decode_str_copy_loop_ascii_u8(u8 **dst_addr, const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_loop_read_src_impl(*src_addr, &vec, &check_mask);
    memcpy(*dst_addr, &vec, sizeof(vector_a));
    usize moved_count = _decode_str_loop_decoder_impl(src_addr, src_end, check_mask, &ret, true, NULL, vec, escapeval_addr);
    *dst_addr += moved_count;
    return ret;
}

force_inline int decode_str_copy_trailing_ascii_u8(u8 **dst_addr, const _src_t **src_addr, const _src_t *src_end,
                                                   EscapeInfo *escape_info_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_trailing_read_src_impl(*src_addr, src_end, &vec, &check_mask);
#if SSRJSON_X86 && COMPILE_SIMD_BITS == 256
    avx2_trailing_cvt_u8_u8(*src_addr, src_end, *dst_addr);
#else
    *(vector_u *)(*dst_addr) = vec;
#endif
    usize done_count = _decode_str_trailing_decoder_impl(src_addr, src_end, check_mask, &ret, false, NULL, vec, escape_info_addr);
    *dst_addr += done_count;
    //
    return ret;
}

force_inline int decode_str_copy_trailing_ascii_u16(u16 **dst_addr, const _src_t **src_addr, const _src_t *src_end,
                                                    EscapeInfo *escape_info_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_trailing_read_src_impl(*src_addr, src_end, &vec, &check_mask);
#if SSRJSON_X86 && COMPILE_SIMD_BITS == 256
    avx2_trailing_cvt_u8_u16(*src_addr, src_end, *dst_addr);
#else
    MAKE_S_NAME(cvt_to_dst_u8_u16)(*dst_addr, vec);
#endif
    usize done_count = _decode_str_trailing_decoder_impl(src_addr, src_end, check_mask, &ret, false, NULL, vec, escape_info_addr);
    *dst_addr += done_count;
    //
    return ret;
}

force_inline int decode_str_copy_trailing_ascii_u32(u32 **dst_addr,
                                                    const _src_t **src_addr,
                                                    const _src_t *src_end,
                                                    EscapeInfo *escape_info_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_trailing_read_src_impl(*src_addr, src_end, &vec, &check_mask);
#if SSRJSON_X86 && COMPILE_SIMD_BITS == 256
    avx2_trailing_cvt_u8_u32(*src_addr, src_end, *dst_addr);
#else
    MAKE_S_NAME(cvt_to_dst_u8_u32)(*dst_addr, vec);
#endif
    usize done_count = _decode_str_trailing_decoder_impl(src_addr, src_end, check_mask, &ret, false, NULL, vec, escape_info_addr);
    *dst_addr += done_count;
    //
    return ret;
}

force_inline int process_escape_ascii_u8(EscapeInfo escape_info, u8 **u8writer_addr, u16 **u16writer_addr, u32 **u32writer_addr, usize *u8size_addr, bool *is_ascii_addr, void *temp_buffer) {
    u32 escape_val;
    usize escape_len;
    escape_val = escape_info.escape_val;
    // src += escape_info.escape_size;
    assert(escape_val != _DECODE_UNICODE_ERR);
    if (escape_val < 0x80) {
        // ascii
        *(*u8writer_addr)++ = (u8)escape_val;
        return 0;
    } else if (escape_val < 0x100) {
        *is_ascii_addr = false;
        *(*u8writer_addr)++ = (u8)escape_val;
        return 1;
    } else if (escape_val < 0x10000) {
        // update
        usize u8size = (*u8writer_addr) - SSRJSON_CAST(u8 *, temp_buffer);
        *u8size_addr = u8size;
        *u8writer_addr = NULL;
        *u16writer_addr = SSRJSON_CAST(u16 *, temp_buffer) + u8size;
        *(*u16writer_addr)++ = (u16)escape_val;
        return 2;
    } else {
        // update
        usize u8size = (*u8writer_addr) - SSRJSON_CAST(u8 *, temp_buffer);
        *u8size_addr = u8size;
        *u8writer_addr = NULL;
        *u32writer_addr = SSRJSON_CAST(u32 *, temp_buffer) + u8size;
        *(*u32writer_addr)++ = escape_val;
        return 4;
    }
}

force_inline int decode_str_copy_loop4_ascii_u16(u16 **dst_addr, const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr) {
    int ret;
    //
    unionvector_a_x4 vec;
    anymask_t check_mask[4];
    anymask_t check_mask_total;
    //
    _decode_str_loop4_read_src_impl(*src_addr, &vec, check_mask, &check_mask_total);
    u16 *dst = *dst_addr;
    for (int i = 0; i < 4; ++i) {
        MAKE_S_NAME(cvt_to_dst_u8_u16)((dst + i * READ_BATCH_COUNT), vec.x[i]);
    }
    usize moved_count = _decode_str_loop4_decoder_impl(src_addr, src_end, check_mask, check_mask_total, &ret, false, NULL, vec, escapeval_addr);
    *dst_addr += moved_count;
    return ret;
}

force_inline int decode_str_copy_loop_ascii_u16(u16 **dst_addr, const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_loop_read_src_impl(*src_addr, &vec, &check_mask);
    MAKE_S_NAME(cvt_to_dst_u8_u16)(*dst_addr, vec);
    usize moved_count = _decode_str_loop_decoder_impl(src_addr, src_end, check_mask, &ret, false, NULL, vec, escapeval_addr);
    *dst_addr += moved_count;
    return ret;
}

force_inline int process_escape_ascii_u16(EscapeInfo escape_info, u16 **u16writer_addr, u32 **u32writer_addr, usize *u16size_addr, usize u8size, void *temp_buffer) {
    u32 escape_val;
    usize escape_len;
    escape_val = escape_info.escape_val;
    assert(escape_val != _DECODE_UNICODE_ERR);
    if (escape_val < 0x10000) {
        *(*u16writer_addr)++ = (u16)escape_val;
        return 2;
    } else {
        // update
        usize u16size_total = (*u16writer_addr) - SSRJSON_CAST(u16 *, temp_buffer);
        assert(u16size_total >= u8size);
        *u16size_addr = u16size_total - u8size;
        *u16writer_addr = NULL;
        *u32writer_addr = SSRJSON_CAST(u32 *, temp_buffer) + u16size_total;
        *(*u32writer_addr)++ = escape_val;
        return 4;
    }
}

force_inline int decode_str_copy_loop4_ascii_u32(u32 **dst_addr, const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr) {
    int ret;
    //
    unionvector_a_x4 vec;
    anymask_t check_mask[4];
    anymask_t check_mask_total;
    //
    _decode_str_loop4_read_src_impl(*src_addr, &vec, check_mask, &check_mask_total);
    u32 *dst = *dst_addr;
    for (int i = 0; i < 4; ++i) {
        MAKE_S_NAME(cvt_to_dst_u8_u32)((dst + i * READ_BATCH_COUNT), vec.x[i]);
    }
    usize moved_count = _decode_str_loop4_decoder_impl(src_addr, src_end, check_mask, check_mask_total, &ret, false, NULL, vec, escapeval_addr);
    *dst_addr += moved_count;
    return ret;
}

force_inline int decode_str_copy_loop_ascii_u32(u32 **dst_addr, const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_loop_read_src_impl(*src_addr, &vec, &check_mask);
    MAKE_S_NAME(cvt_to_dst_u8_u32)(*dst_addr, vec);
    usize moved_count = _decode_str_loop_decoder_impl(src_addr, src_end, check_mask, &ret, false, NULL, vec, escapeval_addr);
    *dst_addr += moved_count;
    return ret;
}

force_inline void process_escape_ascii_u32(EscapeInfo escape_info, u32 **u32writer_addr) {
    u32 escape_val;
    usize escape_len;
    escape_val = escape_info.escape_val;
    assert(escape_val != _DECODE_UNICODE_ERR);
    *(*u32writer_addr)++ = escape_val;
}

internal_simd_noinline PyObject *decode_str_with_escape_ascii(
        const _src_t *src_start,
        const _src_t **src_addr,
        const _src_t *src_end,
        void *temp_buffer,
        bool is_key,
        EscapeInfo in_escape_info) {
#define CAN_LOOP4() (src_end - 4 * READ_BATCH_COUNT >= src)
#define CAN_LOOP() (src_end - 1 * READ_BATCH_COUNT >= src)
    //
    const _src_t *src = *src_addr;
    //
    int decode_state_size;
    u8 *u8writer = NULL;
    u16 *u16writer = NULL;
    u32 *u32writer = NULL;
    usize u8size = 0;
    usize u16size = 0;
    bool is_ascii = false;
    // pre-process
    {
        usize pre_copy_size = src - src_start;
        u32 in_escape_val = in_escape_info.escape_val;
        if (in_escape_val < 0x100) {
            // ascii, or ucs1
            decode_state_size = 1;
            ssrjson_memcpy(temp_buffer, src_start, pre_copy_size);
            u8writer = SSRJSON_CAST(u8 *, temp_buffer) + pre_copy_size;
            *u8writer++ = (u8)in_escape_val;
            is_ascii = in_escape_val < 0x80;
        } else if (in_escape_val < 0x10000) {
            // ucs2
            decode_state_size = 2;
            SIMD_NAME_MODIFIER(long_cvt_noinline_u8_u16)(temp_buffer, src_start, pre_copy_size);
            u16writer = SSRJSON_CAST(u16 *, temp_buffer) + pre_copy_size;
            *u16writer++ = (u16)in_escape_val;
        } else {
            decode_state_size = 4;
            SIMD_NAME_MODIFIER(long_cvt_noinline_u8_u32)(temp_buffer, src_start, pre_copy_size);
            u32writer = SSRJSON_CAST(u32 *, temp_buffer) + pre_copy_size;
            *u32writer++ = in_escape_val;
        }
        src += in_escape_info.escape_size;
    }
    // start
    switch (decode_state_size) {
        case 1: {
            goto decode_loop_ucs1;
        }
        case 2: {
            goto decode_loop_ucs2;
        }
        case 4: {
            goto decode_loop_ucs4;
        }
        default: {
            SSRJSON_UNREACHABLE();
        }
    }

decode_loop_ucs1:;
    {
#define LOOP_SWITCHER(_status_code_, _on_escape_)       \
    {                                                   \
        switch ((_status_code_)) {                      \
            case DECODE_LOOPSTATE_CONTINUE: {           \
                continue;                               \
            }                                           \
            case DECODE_LOOPSTATE_END: {                \
                if (is_ascii) {                         \
                    goto done_ascii;                    \
                }                                       \
                goto done_ucs1;                         \
            }                                           \
            case DECODE_LOOPSTATE_ESCAPE: {             \
                src += escape_info.escape_size;         \
                int escape_status_code = (_on_escape_); \
                switch (escape_status_code) {           \
                    case 0:                             \
                    case 1: {                           \
                        break;                          \
                    }                                   \
                    case 2: {                           \
                        goto decode_loop_ucs2;          \
                    }                                   \
                    case 4: {                           \
                        goto decode_loop_ucs4;          \
                    }                                   \
                    default: {                          \
                        SSRJSON_UNREACHABLE();          \
                    }                                   \
                }                                       \
                continue;                               \
            }                                           \
            case DECODE_LOOPSTATE_INVALID: {            \
                assert(PyErr_Occurred());               \
                goto failed;                            \
            }                                           \
            default: {                                  \
                SSRJSON_UNREACHABLE();                  \
            }                                           \
        }                                               \
    }
#define ON_ESCAPE process_escape_ascii_u8(escape_info, &u8writer, &u16writer, &u32writer, &u8size, &is_ascii, temp_buffer)
        if (!is_key) {
            while (CAN_LOOP4()) {
                EscapeInfo escape_info;
                int state_code = decode_str_copy_loop4_ascii_u8(&u8writer, &src, src_end, &escape_info);
                LOOP_SWITCHER(state_code, ON_ESCAPE);
            }
        }
        while (CAN_LOOP()) {
            EscapeInfo escape_info;
            int state_code = decode_str_copy_loop_ascii_u8(&u8writer, &src, src_end, &escape_info);
            LOOP_SWITCHER(state_code, ON_ESCAPE);
        }
    trailing_ucs1:;
        if (likely(src < src_end)) {
            EscapeInfo escape_info;
            int status_code = decode_str_copy_trailing_ascii_u8(&u8writer, &src, src_end, &escape_info);
            switch (status_code) {
                case DECODE_LOOPSTATE_END: {
                    if (is_ascii) goto done_ascii;
                    goto done_ucs1;
                }
                case DECODE_LOOPSTATE_ESCAPE: {
                    src += escape_info.escape_size;
                    int escape_status_code = ON_ESCAPE;
                    switch (escape_status_code) {
                        case 0:
                        case 1: {
                            break;
                        }
                        case 2: {
                            goto decode_loop_ucs2;
                        }
                        case 4: {
                            goto decode_loop_ucs4;
                        }
                        default: {
                            SSRJSON_UNREACHABLE();
                        }
                    }
                    goto trailing_ucs1;
                }
                case DECODE_LOOPSTATE_INVALID: {
                    assert(PyErr_Occurred());
                    goto failed;
                }
                default: {
                    SSRJSON_UNREACHABLE();
                }
            }
        } else {
            PyErr_SetString(JSONDecodeError, "Unexpected end of string");
            goto failed;
        }
#undef ON_ESCAPE
#undef LOOP_SWITCHER
    }
decode_loop_ucs2:;
    {
#define LOOP_SWITCHER(_status_code_, _on_escape_)       \
    {                                                   \
        switch ((_status_code_)) {                      \
            case DECODE_LOOPSTATE_CONTINUE: {           \
                continue;                               \
            }                                           \
            case DECODE_LOOPSTATE_END: {                \
                goto done_ucs2;                         \
            }                                           \
            case DECODE_LOOPSTATE_ESCAPE: {             \
                src += escape_info.escape_size;         \
                int escape_status_code = (_on_escape_); \
                switch (escape_status_code) {           \
                    case 2: {                           \
                        break;                          \
                    }                                   \
                    case 4: {                           \
                        goto decode_loop_ucs4;          \
                    }                                   \
                    default: {                          \
                        SSRJSON_UNREACHABLE();          \
                    }                                   \
                }                                       \
                continue;                               \
            }                                           \
            case DECODE_LOOPSTATE_INVALID: {            \
                assert(PyErr_Occurred());               \
                goto failed;                            \
            }                                           \
            default: {                                  \
                SSRJSON_UNREACHABLE();                  \
            }                                           \
        }                                               \
    }
#define ON_ESCAPE process_escape_ascii_u16(escape_info, &u16writer, &u32writer, &u16size, u8size, temp_buffer)
        if (!is_key) {
            while (CAN_LOOP4()) {
                EscapeInfo escape_info;
                int state_code = decode_str_copy_loop4_ascii_u16(&u16writer, &src, src_end, &escape_info);
                LOOP_SWITCHER(state_code, ON_ESCAPE);
            }
        }
        while (CAN_LOOP()) {
            EscapeInfo escape_info;
            int state_code = decode_str_copy_loop_ascii_u16(&u16writer, &src, src_end, &escape_info);
            LOOP_SWITCHER(state_code, ON_ESCAPE);
        }

    trailing_ucs2:;
        if (likely(src < src_end)) {
            EscapeInfo escape_info;
            int status_code = decode_str_copy_trailing_ascii_u16(&u16writer, &src, src_end, &escape_info);
            switch (status_code) {
                case DECODE_LOOPSTATE_END: {
                    goto done_ucs2;
                }
                case DECODE_LOOPSTATE_ESCAPE: {
                    src += escape_info.escape_size;
                    int escape_status_code = ON_ESCAPE;
                    switch (escape_status_code) {
                        case 2: {
                            break;
                        }
                        case 4: {
                            goto decode_loop_ucs4;
                        }
                        default: {
                            SSRJSON_UNREACHABLE();
                        }
                    }
                    goto trailing_ucs2;
                }
                case DECODE_LOOPSTATE_INVALID: {
                    assert(PyErr_Occurred());
                    goto failed;
                }
                default: {
                    SSRJSON_UNREACHABLE();
                }
            }
        } else {
            PyErr_SetString(JSONDecodeError, "Unexpected end of string");
            goto failed;
        }
#undef ON_ESCAPE
#undef LOOP_SWITCHER
    }
decode_loop_ucs4:;
    {
#define LOOP_SWITCHER(_status_code_)            \
    {                                           \
        switch ((_status_code_)) {              \
            case DECODE_LOOPSTATE_CONTINUE: {   \
                continue;                       \
            }                                   \
            case DECODE_LOOPSTATE_END: {        \
                goto done_ucs4;                 \
            }                                   \
            case DECODE_LOOPSTATE_ESCAPE: {     \
                src += escape_info.escape_size; \
                process_escape_ascii_u32(       \
                        escape_info,            \
                        &u32writer);            \
                continue;                       \
            }                                   \
            case DECODE_LOOPSTATE_INVALID: {    \
                assert(PyErr_Occurred());       \
                goto failed;                    \
            }                                   \
            default: {                          \
                SSRJSON_UNREACHABLE();          \
            }                                   \
        }                                       \
    }
        if (!is_key) {
            while (CAN_LOOP4()) {
                EscapeInfo escape_info;
                int state_code = decode_str_copy_loop4_ascii_u32(&u32writer, &src, src_end, &escape_info);
                LOOP_SWITCHER(state_code);
            }
        }
        while (CAN_LOOP()) {
            EscapeInfo escape_info;
            int state_code = decode_str_copy_loop_ascii_u32(&u32writer, &src, src_end, &escape_info);
            LOOP_SWITCHER(state_code);
        }
    trailing_ucs4:;
        if (likely(src < src_end)) {
            EscapeInfo escape_info;
            int status_code = decode_str_copy_trailing_ascii_u32(&u32writer, &src, src_end, &escape_info);
            switch (status_code) {
                case DECODE_LOOPSTATE_END: {
                    goto done_ucs4;
                }
                case DECODE_LOOPSTATE_ESCAPE: {
                    src += escape_info.escape_size;
                    process_escape_ascii_u32(
                            escape_info,
                            &u32writer);
                    goto trailing_ucs4;
                }
                case DECODE_LOOPSTATE_INVALID: {
                    assert(PyErr_Occurred());
                    goto failed;
                }
                default: {
                    SSRJSON_UNREACHABLE();
                }
            }
        } else {
            PyErr_SetString(JSONDecodeError, "Unexpected end of string");
            goto failed;
        }
#undef LOOP_SWITCHER
    }
done_ascii:;
    {
        PyObject *ret = make_unicode_from_src_ascii(temp_buffer, u8writer - SSRJSON_CAST(u8 *, temp_buffer), is_key);
        *src_addr = src + 1;
        return ret;
    }
done_ucs1:;
    {
        PyObject *ret = make_unicode_ucs1(temp_buffer, u8writer - SSRJSON_CAST(u8 *, temp_buffer), is_key);
        *src_addr = src + 1;
        return ret;
    }
done_ucs2:;
    {
        PyObject *ret = make_unicode_ucs2(temp_buffer, u8size, u16writer - SSRJSON_CAST(u16 *, temp_buffer), is_key);
        *src_addr = src + 1;
        return ret;
    }
done_ucs4:;
    {
        PyObject *ret = make_unicode_ucs4(temp_buffer, u8size, u16size, u32writer - SSRJSON_CAST(u32 *, temp_buffer), is_key);
        *src_addr = src + 1;
        return ret;
    }
failed:;
    assert(PyErr_Occurred());
    *src_addr = src;
    return NULL;
#undef CAN_LOOP4
#undef CAN_LOOP
}

force_inline PyObject *decode_str_ascii(
        const _src_t **src_addr,
        const _src_t *const src_end,
        void *temp_buffer,
        bool is_key) {
#define CAN_LOOP4() (src_end - 4 * READ_BATCH_COUNT >= src)
#define CAN_LOOP() (src_end - 1 * READ_BATCH_COUNT >= src)
#define LOOP_SWITCHER(_status_code_)              \
    switch (status_code) {                        \
        case DECODE_LOOPSTATE_CONTINUE: {         \
            continue;                             \
        }                                         \
        case DECODE_LOOPSTATE_END: {              \
            goto done;                            \
        }                                         \
        case DECODE_LOOPSTATE_ESCAPE: {           \
            PyObject *ret =                       \
                    decode_str_with_escape_ascii( \
                            original_src,         \
                            &src, src_end,        \
                            temp_buffer,          \
                            is_key,               \
                            escape_info);         \
            *src_addr = src;                      \
            return ret;                           \
        }                                         \
        case DECODE_LOOPSTATE_INVALID: {          \
            assert(PyErr_Occurred());             \
            goto failed;                          \
        }                                         \
        default: {                                \
            SSRJSON_UNREACHABLE();                \
        }                                         \
    }


    const _src_t *src = *src_addr;
    const _src_t *const original_src = src;

    if (!is_key) {
        while (CAN_LOOP4()) {
            EscapeInfo escape_info;
            int status_code = decode_str_fast_loop4_ascii(&src, src_end, &escape_info);
            LOOP_SWITCHER(status_code);
        }
    }

    while (CAN_LOOP()) {
        EscapeInfo escape_info;
        int status_code = decode_str_fast_loop_ascii(&src, src_end, &escape_info);
        LOOP_SWITCHER(status_code);
    }

    if (likely(src < src_end)) {
        EscapeInfo escape_info;
        int status_code = decode_str_fast_trailing_ascii(&src, src_end, &escape_info);
        switch (status_code) {
            case DECODE_LOOPSTATE_END: {
                goto done;
            }
            case DECODE_LOOPSTATE_ESCAPE: {
                PyObject *ret =
                        decode_str_with_escape_ascii(
                                original_src,
                                &src, src_end,
                                temp_buffer,
                                is_key,
                                escape_info);
                *src_addr = src;
                return ret;
            }
            case DECODE_LOOPSTATE_INVALID: {
                assert(PyErr_Occurred());
                goto failed;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
    } else {
        PyErr_SetString(JSONDecodeError, "Unexpected end of string");
        goto failed;
    }

done:;
    *src_addr = src + 1; // skip the ending '"'
    return make_unicode_from_src_ascii(original_src, src - original_src, is_key);

failed:;
    *src_addr = src;
    return NULL;
#undef LOOP_SWITCHER
#undef CAN_LOOP4
#undef CAN_LOOP
}

internal_simd_noinline PyObject *decode_str_ascii_not_key(const _src_t **src_addr,
                                                          const _src_t *const src_end,
                                                          void *temp_buffer) {
    return decode_str_ascii(src_addr, src_end, temp_buffer, false);
}

#include "compile_context/sr_out.inl.h"
//
#undef COMPILE_SIMD_BITS
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_UCS_LEVEL

#endif // SSRJSON_DECODE_STR_ASCII_H

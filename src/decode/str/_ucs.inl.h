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
#        include "cache_key.h"
#        include "copy_to_new.h"
#        include "decode_str_copy.h"
#        include "process_escape.h"
#        include "pythonlib.h"
#        include "simd/long_cvt.h"
#        define COMPILE_UCS_LEVEL 1
#        include "simd/compile_feature_check.h"
#    endif
#endif

#define COMPILE_READ_UCS_LEVEL COMPILE_UCS_LEVEL
#include "compile_context/sr_in.inl.h"

// long cvt function choosing.
#if COMPILE_UCS_LEVEL == 1
#    define long_cvt_noinline_to_u8 memcpy
#    define long_cvt_noinline_to_u16 SIMD_NAME_MODIFIER(SSRJSON_CONCAT3(long_cvt_noinline, _src_t, u16))
#    define long_cvt_noinline_to_u32 SIMD_NAME_MODIFIER(SSRJSON_CONCAT3(long_cvt_noinline, _src_t, u32))
#elif COMPILE_UCS_LEVEL == 2
#    define long_cvt_noinline_to_u16(_d_, _s_, _cnt_) memcpy((_d_), (_s_), 2 * (_cnt_))
#    define long_cvt_noinline_to_u32 SIMD_NAME_MODIFIER(SSRJSON_CONCAT3(long_cvt_noinline, _src_t, u32))
#elif COMPILE_UCS_LEVEL == 4
#    define long_cvt_noinline_to_u32(_d_, _s_, _cnt_) memcpy((_d_), (_s_), 4 * (_cnt_))
#endif
#define decode_str_copy_loop4_to_u8 SSRJSON_CONCAT2(MAKE_UCS_NAME(decode_str_copy_loop4), u8)
#define decode_str_copy_loop4_to_u16 SSRJSON_CONCAT2(MAKE_UCS_NAME(decode_str_copy_loop4), u16)
#define decode_str_copy_loop4_to_u32 SSRJSON_CONCAT2(MAKE_UCS_NAME(decode_str_copy_loop4), u32)
#define decode_str_copy_loop_to_u8 SSRJSON_CONCAT2(MAKE_UCS_NAME(decode_str_copy_loop), u8)
#define decode_str_copy_loop_to_u16 SSRJSON_CONCAT2(MAKE_UCS_NAME(decode_str_copy_loop), u16)
#define decode_str_copy_loop_to_u32 SSRJSON_CONCAT2(MAKE_UCS_NAME(decode_str_copy_loop), u32)

#define decode_str_copy_trailing_to_u8 SSRJSON_CONCAT2(MAKE_UCS_NAME(decode_str_copy_trailing), u8)
#define decode_str_copy_trailing_to_u16 SSRJSON_CONCAT2(MAKE_UCS_NAME(decode_str_copy_trailing), u16)
#define decode_str_copy_trailing_to_u32 SSRJSON_CONCAT2(MAKE_UCS_NAME(decode_str_copy_trailing), u32)

force_inline int decode_str_fast_loop4(const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr, vector_a *maxvec_addr) {
    int ret;
    //
    unionvector_a_x4 vec;
    anymask_t check_mask[4];
    anymask_t check_mask_total;
    //
    _decode_str_loop4_read_src_impl(*src_addr, &vec, check_mask, &check_mask_total);
    _decode_str_loop4_decoder_impl(src_addr, src_end, check_mask, check_mask_total, &ret, true, maxvec_addr, vec, escapeval_addr);
    //
    return ret;
}

force_inline int decode_str_fast_loop(const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr, vector_a *maxvec_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_loop_read_src_impl(*src_addr, &vec, &check_mask);
    _decode_str_loop_decoder_impl(src_addr, src_end, check_mask, &ret, true, maxvec_addr, vec, escapeval_addr);
    //
    return ret;
}

force_inline int decode_str_fast_trailing(const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escape_info_addr, vector_a *maxvec_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_trailing_read_src_impl(*src_addr, src_end, &vec, &check_mask);
    _decode_str_trailing_decoder_impl(src_addr, src_end, check_mask, &ret, true, maxvec_addr, vec, escape_info_addr);
    //
    return ret;
}

// fast path unicode maker
force_inline PyObject *make_unicode_from_src(const _src_t *start, usize count, bool is_key, vector_a maxvec, void *temp_buffer) {
    const _src_t upper_bound = (COMPILE_UCS_LEVEL == 1) ? 0x7f : ((COMPILE_UCS_LEVEL == 2) ? 0xff : 0xffff);

    PyObject *ret;
    decode_keyhash_t hash;

    bool need_cvt = checkmax(maxvec, upper_bound);
    int kind = COMPILE_UCS_LEVEL;
    bool need_size_cvt = need_cvt && COMPILE_UCS_LEVEL > 1;
    if (need_cvt) {
#if COMPILE_UCS_LEVEL == 1
        kind = 0;
#elif COMPILE_UCS_LEVEL == 2
        if (checkmax(maxvec, 0x7f)) {
            kind = 0;
        } else {
            kind = 1;
        }
#else
        if (checkmax(maxvec, 0x7f)) {
            kind = 0;
        } else {
            if (checkmax(maxvec, 0xff)) {
                kind = 1;
            } else {
                kind = 2;
            }
        }
#endif
    }
    usize tpsize = kind ? kind : 1;
    bool should_cache = is_key && (count * tpsize) <= 64;
    bool should_hash = is_key && count > 0;
    if (should_cache) {
        const void *hash_string_ptr;
        usize hash_string_u8size;
        get_cache_key_hash_and_size(&hash_string_ptr, &hash_string_u8size, start, count, tpsize, need_size_cvt, temp_buffer);
        hash = XXH3_64bits(hash_string_ptr, hash_string_u8size);
        ret = get_key_cache(hash_string_ptr, hash, hash_string_u8size, kind);
        if (ret) {
            goto done;
        }
    }

    ret = create_empty_unicode(count, kind);
    if (likely(ret)) {
        // copy src to unicode.
        void *dst_void;
        if (need_size_cvt && should_cache) {
            const u8 *temp_src = temp_buffer;
            u8 *temp_dst = kind ? PYUNICODE_UCS1_START(ret) : PYUNICODE_ASCII_START(ret);
            dst_void = temp_dst;
            __ssrjson_short_memcpy_small_first(&temp_dst, &temp_src, count * tpsize, 64);
        } else {
            MAKE_UCS_NAME(copy_to_new_unicode)(&dst_void, ret, need_cvt, start, count, kind);
        }
        if (should_cache) {
            add_key_cache(hash, ret, count * tpsize, kind);
        }
        if (should_hash) {
            assert(count && SSRJSON_CAST(PyASCIIObject *, ret)->hash == -1);
            make_hash(SSRJSON_CAST(PyASCIIObject *, ret), dst_void, count * tpsize);
        }
    }
done:;
    return ret;
}

internal_simd_noinline PyObject *decode_str_with_escape(
        const _src_t *src_start,
        const _src_t **src_addr,
        const _src_t *src_end,
        void *temp_buffer,
        bool is_key,
        EscapeInfo in_escape_info,
        vector_a maxvec) {
#define CAN_LOOP4() (src_end - 4 * READ_BATCH_COUNT >= src)
#define CAN_LOOP() (src_end - 1 * READ_BATCH_COUNT >= src)
    //
    const _src_t *src = *src_addr;
    //
    int decode_state_size;
    u32 max_escape = 0;
#if COMPILE_UCS_LEVEL <= 1
    u8 *u8writer = NULL;
    usize u8size = 0;
#endif
#if COMPILE_UCS_LEVEL <= 2
    u16 *u16writer = NULL;
    usize u16size = 0;
#endif
    u32 *u32writer = NULL;
    // pre-process
    {
        usize pre_copy_size = src - src_start;
        u32 in_escape_val = in_escape_info.escape_val;
        max_escape = in_escape_val;
#if COMPILE_UCS_LEVEL <= 1
        if (in_escape_val < 0x100) {
            // ascii, or ucs1
            decode_state_size = 1;
            long_cvt_noinline_to_u8(temp_buffer, src_start, pre_copy_size);
            u8writer = SSRJSON_CAST(u8 *, temp_buffer) + pre_copy_size;
            *u8writer++ = (u8)in_escape_val;
        } else
#endif
#if COMPILE_UCS_LEVEL <= 2
                if (in_escape_val < 0x10000) {
            // ucs2
            decode_state_size = 2;
            long_cvt_noinline_to_u16(temp_buffer, src_start, pre_copy_size);
            u16writer = SSRJSON_CAST(u16 *, temp_buffer) + pre_copy_size;
            *u16writer++ = (u16)in_escape_val;
        } else
#endif
        {
            decode_state_size = 4;
            long_cvt_noinline_to_u32(temp_buffer, src_start, pre_copy_size);
            u32writer = SSRJSON_CAST(u32 *, temp_buffer) + pre_copy_size;
            *u32writer++ = in_escape_val;
        }
        src += in_escape_info.escape_size;
    }
    // start
    switch (decode_state_size) {
#if COMPILE_UCS_LEVEL <= 1
        case 1: {
            goto decode_loop_ucs1;
        }
#endif
#if COMPILE_UCS_LEVEL <= 2
        case 2: {
            goto decode_loop_ucs2;
        }
#endif
        case 4: {
            goto decode_loop_ucs4;
        }
        default: {
            SSRJSON_UNREACHABLE();
        }
    }

#if COMPILE_UCS_LEVEL <= 1
decode_loop_ucs1:;
    {
#    define LOOP_SWITCHER(_status_code_, _on_escape_)       \
        {                                                   \
            switch ((_status_code_)) {                      \
                case DECODE_LOOPSTATE_CONTINUE: {           \
                    continue;                               \
                }                                           \
                case DECODE_LOOPSTATE_END: {                \
                    goto done_ucs1;                         \
                }                                           \
                case DECODE_LOOPSTATE_ESCAPE: {             \
                    src += escape_info.escape_size;         \
                    int escape_status_code = (_on_escape_); \
                    switch (escape_status_code) {           \
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
#    define ON_ESCAPE process_escape_ucs1_u8(escape_info, &u8writer, &u16writer, &u32writer, &u8size, &max_escape, temp_buffer)

        if (!is_key) {
            while (CAN_LOOP4()) {
                EscapeInfo escape_info;
                int state_code = decode_str_copy_loop4_to_u8(&u8writer, &src, src_end, &escape_info, &maxvec);
                LOOP_SWITCHER(state_code, ON_ESCAPE);
            }
        }
        while (CAN_LOOP()) {
            EscapeInfo escape_info;
            int state_code = decode_str_copy_loop_to_u8(&u8writer, &src, src_end, &escape_info, &maxvec);
            LOOP_SWITCHER(state_code, ON_ESCAPE);
        }
    trailing_ucs1:;
        if (likely(src < src_end)) {
            EscapeInfo escape_info;
            int status_code = decode_str_copy_trailing_to_u8(&u8writer, &src, src_end, &escape_info, &maxvec);
            switch (status_code) {
                case DECODE_LOOPSTATE_END: {
                    goto done_ucs1;
                }
                case DECODE_LOOPSTATE_ESCAPE: {
                    src += escape_info.escape_size;
                    int escape_status_code = ON_ESCAPE;
                    switch (escape_status_code) {
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
#    undef ON_ESCAPE
#    undef LOOP_SWITCHER
    }
#endif
#if COMPILE_UCS_LEVEL <= 2
decode_loop_ucs2:;
    {
#    define LOOP_SWITCHER(_status_code_, _on_escape_)       \
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
#    if COMPILE_UCS_LEVEL == 2
#        define ON_ESCAPE process_escape_ucs2_u16(escape_info, &u16writer, &u32writer, &u16size, &max_escape, temp_buffer)
#    else
#        define ON_ESCAPE process_escape_ucs1_u16(escape_info, &u16writer, &u32writer, &u8size, &u16size, &max_escape, temp_buffer)
#    endif
        if (!is_key) {
            while (CAN_LOOP4()) {
                EscapeInfo escape_info;
                int state_code = decode_str_copy_loop4_to_u16(&u16writer, &src, src_end, &escape_info, &maxvec);
                LOOP_SWITCHER(state_code, ON_ESCAPE);
            }
        }
        while (CAN_LOOP()) {
            EscapeInfo escape_info;
            int state_code = decode_str_copy_loop_to_u16(&u16writer, &src, src_end, &escape_info, &maxvec);
            LOOP_SWITCHER(state_code, ON_ESCAPE);
        }

    trailing_ucs2:;
        if (likely(src < src_end)) {
            EscapeInfo escape_info;
            int status_code = decode_str_copy_trailing_to_u16(&u16writer, &src, src_end, &escape_info, &maxvec);
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
#    undef ON_ESCAPE
#    undef LOOP_SWITCHER
    }
#endif
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
                process_escape_to_u32(          \
                        escape_info,            \
                        &u32writer,             \
                        &max_escape);           \
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
                int state_code = decode_str_copy_loop4_to_u32(&u32writer, &src, src_end, &escape_info, &maxvec);
                LOOP_SWITCHER(state_code);
            }
        }
        while (CAN_LOOP()) {
            EscapeInfo escape_info;
            int state_code = decode_str_copy_loop_to_u32(&u32writer, &src, src_end, &escape_info, &maxvec);
            LOOP_SWITCHER(state_code);
        }
    trailing_ucs4:;
        if (likely(src < src_end)) {
            EscapeInfo escape_info;
            int status_code = decode_str_copy_trailing_to_u32(&u32writer, &src, src_end, &escape_info, &maxvec);
            switch (status_code) {
                case DECODE_LOOPSTATE_END: {
                    goto done_ucs4;
                }
                case DECODE_LOOPSTATE_ESCAPE: {
                    src += escape_info.escape_size;
                    process_escape_to_u32(
                            escape_info,
                            &u32writer,
                            &max_escape);
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
#if COMPILE_UCS_LEVEL <= 1
done_ucs1:;
    {
        PyObject *ret;
        // check if down to ascii
        if (max_escape <= 0x7f && checkmax(maxvec, 0x7f)) {
            // ascii
            ret = make_unicode_from_raw_ascii(temp_buffer, u8writer - SSRJSON_CAST(u8 *, temp_buffer), is_key);
        } else {
            ret = make_unicode_from_raw_ucs1(temp_buffer, u8writer - SSRJSON_CAST(u8 *, temp_buffer), is_key);
        }
        *src_addr = src + 1;
        return ret;
    }
#endif
#if COMPILE_UCS_LEVEL <= 2
done_ucs2:;
    {
        PyObject *ret;
#    if COMPILE_UCS_LEVEL == 2
        if (max_escape <= 0xff && checkmax(maxvec, 0xff)) {
            bool is_ascii = (max_escape <= 0x7f && checkmax(maxvec, 0x7f));
            ret = make_unicode_down_ucs2_u8(temp_buffer, u16writer - SSRJSON_CAST(u16 *, temp_buffer), is_key, is_ascii);
        } else
#    endif
        {
            ret = make_unicode_from_raw_ucs2(temp_buffer,
#    if COMPILE_UCS_LEVEL <= 1
                                             u8size
#    else
                                             0
#    endif
                                             ,
                                             u16writer - SSRJSON_CAST(u16 *, temp_buffer), is_key);
        }
        *src_addr = src + 1;
        return ret;
    }
#endif
done_ucs4:;
    {
        PyObject *ret;
#if COMPILE_UCS_LEVEL == 4
        if (max_escape <= 0xffff && checkmax(maxvec, 0xffff)) {
            if (max_escape <= 0xff && checkmax(maxvec, 0xff)) {
                bool is_ascii = (max_escape <= 0x7f && checkmax(maxvec, 0x7f));
                ret = make_unicode_down_ucs4_u8(temp_buffer, u32writer - SSRJSON_CAST(u32 *, temp_buffer), is_key, is_ascii);
            } else {
                ret = make_unicode_down_ucs4_ucs2(temp_buffer, u32writer - SSRJSON_CAST(u32 *, temp_buffer), is_key);
            }
        } else
#endif
        {
            ret = make_unicode_from_raw_ucs4(temp_buffer,
#if COMPILE_UCS_LEVEL <= 1
                                             u8size
#else
                                             0
#endif
                                             ,
#if COMPILE_UCS_LEVEL <= 2
                                             u16size
#else
                                             0
#endif
                                             ,
                                             u32writer - SSRJSON_CAST(u32 *, temp_buffer), is_key);
        }
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

internal_simd_noinline PyObject *decode_str(
        const _src_t **src_addr,
        const _src_t *const src_end,
        void *temp_buffer,
        bool is_key) {
#define CAN_LOOP4() (src_end - 4 * READ_BATCH_COUNT >= src)
#define CAN_LOOP() (src_end - 1 * READ_BATCH_COUNT >= src)
#define LOOP_SWITCHER(_status_code_)        \
    switch (status_code) {                  \
        case DECODE_LOOPSTATE_CONTINUE: {   \
            continue;                       \
        }                                   \
        case DECODE_LOOPSTATE_END: {        \
            goto done;                      \
        }                                   \
        case DECODE_LOOPSTATE_ESCAPE: {     \
            PyObject *ret =                 \
                    decode_str_with_escape( \
                            original_src,   \
                            &src, src_end,  \
                            temp_buffer,    \
                            is_key,         \
                            escape_info,    \
                            maxvec);        \
            *src_addr = src;                \
            return ret;                     \
        }                                   \
        case DECODE_LOOPSTATE_INVALID: {    \
            assert(PyErr_Occurred());       \
            goto failed;                    \
        }                                   \
        default: {                          \
            SSRJSON_UNREACHABLE();          \
        }                                   \
    }

    const _src_t *src = *src_addr;
    const _src_t *const original_src = src;

    vector_a maxvec = setzero();

    if (!is_key) {
        while (CAN_LOOP4()) {
            EscapeInfo escape_info;
            int status_code = decode_str_fast_loop4(&src, src_end, &escape_info, &maxvec);
            LOOP_SWITCHER(status_code);
        }
    }

    while (CAN_LOOP()) {
        EscapeInfo escape_info;
        int status_code = decode_str_fast_loop(&src, src_end, &escape_info, &maxvec);
        LOOP_SWITCHER(status_code);
    }

    if (likely(src < src_end)) {
        EscapeInfo escape_info;
        int status_code = decode_str_fast_trailing(&src, src_end, &escape_info, &maxvec);
        switch (status_code) {
            case DECODE_LOOPSTATE_END: {
                goto done;
            }
            case DECODE_LOOPSTATE_ESCAPE: {
                PyObject *ret =
                        decode_str_with_escape(
                                original_src,
                                &src, src_end,
                                temp_buffer,
                                is_key,
                                escape_info, maxvec);
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
    return make_unicode_from_src(original_src, src - original_src, is_key, maxvec, temp_buffer);

failed:;
    *src_addr = src;
    return NULL;
#undef LOOP_SWITCHER
#undef CAN_LOOP4
#undef CAN_LOOP
}

#undef long_cvt_noinline_to_u8
#undef long_cvt_noinline_to_u16
#undef long_cvt_noinline_to_u32
//
#undef decode_str_copy_loop4_to_u8
#undef decode_str_copy_loop4_to_u16
#undef decode_str_copy_loop4_to_u32
#undef decode_str_copy_loop_to_u8
#undef decode_str_copy_loop_to_u16
#undef decode_str_copy_loop_to_u32

#undef decode_str_copy_trailing_to_u8
#undef decode_str_copy_trailing_to_u16
#undef decode_str_copy_trailing_to_u32
//
#include "compile_context/sr_out.inl.h"
#undef COMPILE_READ_UCS_LEVEL

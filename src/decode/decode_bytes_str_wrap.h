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

/*
 * Decoding of JSON strings out of a `bytes` document.
 *
 * The result of decoding UTF-8 can be any of the four PyUnicode kinds, and
 * which one is only known once the whole string has been seen.
 * The decoder is therefore a small state machine over the destination width,
 * u8 -> u16 -> u32, which only ever widens.
 *
 *   - The fast path scans for the first byte that is not plain ASCII text.
 *     If that byte is the closing quote, the PyUnicode is built straight from
 *     the source bytes, with no intermediate copy at all.
 *   - Anything else jumps once into `decode_bytes_utf8`,
 *     which copies what has been scanned so far into the temporary buffer
 *     and then runs the state machine. Widening the buffer is done in-place.
 */

#ifndef SSRJSON_DECODE_DECODE_BYTES_STR_WRAP_H
#define SSRJSON_DECODE_DECODE_BYTES_STR_WRAP_H

#include "bytes/utf8_shared.h"
#include "bytes/utf8_simd128.h"
#include "decode_shared.h"
#include "simd/memcpy.h"
#include "simd/simd_impl.h"
#include "ssrjson.h"
#include "str/ascii.h"
#include "str/tools.h"
//
#include "simd/compile_feature_check.h"

/* The per-destination-width block decoders. */
#define COMPILE_WRITE_UCS_LEVEL 1
#include "bytes/_w_utf8_block.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 2
#include "bytes/_w_utf8_block.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 4
#include "bytes/_w_utf8_block.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

/* The source of a bytes document is always u8. */
#define COMPILE_READ_UCS_LEVEL 1
#include "compile_context/sr_in.inl.h"

/* The vectorized form of `char_is_ascii_stop`. */
force_inline anymask_t get_bytes_stop_anymask(vector_a x) {
#if SSRJSON_IS_X64 && _CompileVectorBits == 512
    return get_escape_bitmask(x) | unsigned_cmpge_bitmask(x, broadcast(0x80));
#elif SSRJSON_IS_X64
    // psubusb, non-zero exactly for the bytes >= 0x80
    return get_escape_mask(x) | unsigned_saturate_minus(x, broadcast(0x7f));
#else
    return get_escape_mask(x) | (x >= broadcast(0x80));
#endif
}

/*
 * Scan one register length of source. Returns false when a stop byte was found,
 * leaving src on it, and true when the whole register was plain ASCII text.
 */
force_inline bool decode_bytes_scan_loop(const u8 **src_addr) {
    vector_a vec = *ssrjson_cast(vector_u *, *src_addr);
    anymask_t check_mask = get_bytes_stop_anymask(vec);
    if (likely(testz_escape_mask(check_mask))) {
        *src_addr += READ_BATCH_COUNT;
        return true;
    }
    *src_addr += escape_anymask_to_done_count(check_mask);
    return false;
}

force_inline bool decode_bytes_scan_loop4(const u8 **src_addr) {
    const u8 *src = *src_addr;
    unionvector_a_x4 vec;
    anymask_t check_mask[4];
    for (int i = 0; i < 4; ++i) { vec.x[i] = *(ssrjson_cast(vector_u *, src) + i); }
    for (int i = 0; i < 4; ++i) { check_mask[i] = get_bytes_stop_anymask(vec.x[i]); }
    anymask_t check_mask_total = (check_mask[0] | check_mask[1]) | (check_mask[2] | check_mask[3]);
    if (likely(testz_escape_mask(check_mask_total))) {
        *src_addr = src + 4 * READ_BATCH_COUNT;
        return true;
    }
    *src_addr = src + joined4_escape_anymask_to_done_count(check_mask[0], check_mask[1], check_mask[2], check_mask[3]);
    return false;
}

internal_simd_noinline PyObject *decode_bytes_utf8(const u8 *src_start, const u8 **src_addr, const u8 *src_end,
                                                   void *temp_buffer,
                                                   bool is_key DECODER_TLS_KEYCACHE_ADDITIONAL_ARGDEF) {
    const u8 *src = *src_addr;
    u8 *u8writer = NULL;
    u16 *u16writer = NULL;
    u32 *u32writer = NULL;
    usize u8size = 0;
    usize u16size = 0;
    bool is_ascii = true;
    PyObject *ret;

    /*
     * Short and cold: noinline memcpy is enough.
     */
    {
        usize pre_copy_size = (usize)(src - src_start);
        ssrjson_memcpy_noinline(temp_buffer, src_start, pre_copy_size);
        u8writer = ssrjson_cast(u8 *, temp_buffer) + pre_copy_size;
    }

#define ON_ESCAPE_FAIL_CHECK(_escape_info_)                                           \
    do {                                                                              \
        if (unlikely((_escape_info_).escape_val == _DECODE_UNICODE_ERR)) goto failed; \
        src += (_escape_info_).escape_size;                                           \
    } while (0)

decode_loop_u8:;
    while (1) {
        int status_code = decode_bytes_block_u8(&u8writer, &src, src_end, &is_ascii);
        switch (status_code) {
            case DECODE_LOOPSTATE_CONTINUE: {
                continue;
            }
            case DECODE_LOOPSTATE_END: {
                goto done_u8;
            }
            case DECODE_LOOPSTATE_ESCAPE: {
                EscapeInfo escape_info = do_decode_escape(src, src_end);
                ON_ESCAPE_FAIL_CHECK(escape_info);
                int escape_status_code = process_escape_ascii_u8(
                        escape_info, &u8writer, &u16writer, &u32writer, &u8size, &is_ascii, temp_buffer);
                switch (escape_status_code) {
                    case 0:
                    case 1: {
                        break;
                    }
                    case 2: {
                        goto decode_loop_u16;
                    }
                    case 4: {
                        goto decode_loop_u32;
                    }
                    default: {
                        ssrjson_unreachable();
                    }
                }
                continue;
            }
            case DECODE_LOOPSTATE_PROMOTE: {
                if (utf8_lead_min_ucs_level(*src) == 2) {
                    promote_buffer_u8_to_u16(&u8writer, &u16writer, &u8size, temp_buffer);
                    goto decode_loop_u16;
                }
                promote_buffer_u8_to_u32(&u8writer, &u32writer, &u8size, temp_buffer);
                goto decode_loop_u32;
            }
            case DECODE_LOOPSTATE_INVALID: {
                assert(PyErr_Occurred());
                goto failed;
            }
            default: {
                ssrjson_unreachable();
            }
        }
    }

decode_loop_u16:;
    while (1) {
        int status_code = decode_bytes_block_u16(&u16writer, &src, src_end, &is_ascii);
        switch (status_code) {
            case DECODE_LOOPSTATE_CONTINUE: {
                continue;
            }
            case DECODE_LOOPSTATE_END: {
                goto done_u16;
            }
            case DECODE_LOOPSTATE_ESCAPE: {
                EscapeInfo escape_info = do_decode_escape(src, src_end);
                ON_ESCAPE_FAIL_CHECK(escape_info);
                int escape_status_code = process_escape_ascii_u16(
                        escape_info, &u16writer, &u32writer, &u16size, u8size, temp_buffer);
                switch (escape_status_code) {
                    case 2: {
                        break;
                    }
                    case 4: {
                        goto decode_loop_u32;
                    }
                    default: {
                        ssrjson_unreachable();
                    }
                }
                continue;
            }
            case DECODE_LOOPSTATE_PROMOTE: {
                assert(utf8_lead_min_ucs_level(*src) == 4);
                promote_buffer_u16_to_u32(&u16writer, &u32writer, &u16size, u8size, temp_buffer);
                goto decode_loop_u32;
            }
            case DECODE_LOOPSTATE_INVALID: {
                assert(PyErr_Occurred());
                goto failed;
            }
            default: {
                ssrjson_unreachable();
            }
        }
    }

decode_loop_u32:;
    while (1) {
        int status_code = decode_bytes_block_u32(&u32writer, &src, src_end, &is_ascii);
        switch (status_code) {
            case DECODE_LOOPSTATE_CONTINUE: {
                continue;
            }
            case DECODE_LOOPSTATE_END: {
                goto done_u32;
            }
            case DECODE_LOOPSTATE_ESCAPE: {
                EscapeInfo escape_info = do_decode_escape(src, src_end);
                ON_ESCAPE_FAIL_CHECK(escape_info);
                process_escape_ascii_u32(escape_info, &u32writer);
                continue;
            }
            case DECODE_LOOPSTATE_INVALID: {
                assert(PyErr_Occurred());
                goto failed;
            }
            default: {
                ssrjson_unreachable();
            }
        }
    }
#undef ON_ESCAPE_FAIL_CHECK

done_u8:;
    {
        usize count = (usize)(u8writer - ssrjson_cast(u8 *, temp_buffer));
        if (is_ascii) {
            ret = make_unicode_from_src_ascii(temp_buffer, count, is_key DECODER_TLS_KEYCACHE_ADDITIONAL_ARG);
        } else {
            ret = make_unicode_ucs1(temp_buffer, count, is_key);
        }
    }
    *src_addr = src + 1;
    return ret;

done_u16:;
    ret = make_unicode_ucs2(temp_buffer, u8size, (usize)(u16writer - ssrjson_cast(u16 *, temp_buffer)), is_key);
    *src_addr = src + 1;
    return ret;

done_u32:;
    ret = make_unicode_ucs4(
            temp_buffer, u8size, u16size, (usize)(u32writer - ssrjson_cast(u32 *, temp_buffer)), is_key);
    *src_addr = src + 1;
    return ret;

failed:;
    assert(PyErr_Occurred());
    *src_addr = src;
    return NULL;
}

/* Read a JSON string out of a bytes document. */
force_inline PyObject *loads_bytes(const u8 **ptr, u8 *write_buffer, const u8 *src_end,
                                   ssrjson_compiletime bool is_key DECODER_TLS_KEYCACHE_ADDITIONAL_ARGDEF) {
    assert(**ptr == _Quote);
    const u8 *src = *ptr + 1;
    const u8 *const src_start = src;

    /*
     * A whole register can always be read at any src <= src_end, see the
     * padding reserved by `_alloc_aligned_b_buf`.
     * Reading past the string is harmless since *src_end is 0.
     */
    if (ssrjson_consteval(!is_key)) {
        /*
         * Most string values are shorter than two registers and we don't want to 
         * pay for the four way mask join.
         */
        if (!decode_bytes_scan_loop(&src)) goto scan_stopped;
        if (!decode_bytes_scan_loop(&src)) goto scan_stopped;
        while ((usize)(src_end - src) >= 3 * READ_BATCH_COUNT) {
            if (!decode_bytes_scan_loop4(&src)) goto scan_stopped;
        }
    }
    while (1) {
        if (!decode_bytes_scan_loop(&src)) goto scan_stopped;
    }

scan_stopped:;
    {
        u8 c = *src;
        if (likely(c == _Quote)) {
            // The whole string is plain ASCII: build the PyUnicode directly
            // from the source.
            *ptr = src + 1;
            return make_unicode_from_src_ascii(
                    src_start, (usize)(src - src_start), is_key DECODER_TLS_KEYCACHE_ADDITIONAL_ARG);
        }
        if (unlikely(c < _ControlMax)) {
            if (src >= src_end) {
                PyErr_SetString(JSONDecodeError, "Unexpected end of string");
            } else {
                PyErr_SetString(JSONDecodeError, "Invalid control character in string");
            }
            *ptr = src;
            return NULL;
        }
        assert(c == _Slash || c >= 0x80);
        *ptr = src;
        return decode_bytes_utf8(src_start, ptr, src_end, write_buffer, is_key DECODER_TLS_KEYCACHE_ADDITIONAL_ARG);
    }
}

internal_simd_noinline PyObject *loads_bytes_not_key(const u8 **ptr, u8 *write_buffer,
                                                     const u8 *src_end DECODER_TLS_KEYCACHE_ADDITIONAL_ARGDEF) {
    return loads_bytes(ptr, write_buffer, src_end, false DECODER_TLS_KEYCACHE_ADDITIONAL_ARG);
}

#include "compile_context/sr_out.inl.h"
#undef COMPILE_READ_UCS_LEVEL
//
#undef _CompileVectorBits

#endif // SSRJSON_DECODE_DECODE_BYTES_STR_WRAP_H

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

#ifndef SSRJSON_ENCODE_NON_ASCII_H
#define SSRJSON_ENCODE_NON_ASCII_H

#include "encode_utf8.h"
#include "pyutils.h"
#include "ssrjson.h"

#include "simd/compile_feature_check.h"
//
#include "compile_context/s_in.inl.h"

force_inline bool write_cache_impl(const void *src_voidp, int src_pykind, usize len, const u8 **utf8_cache_out,
                                   usize *utf8_length_out, ssrjson_compiletime bool is_key,
                                   ssrjson_compiletime bool is_compact) {
    // Alloc to max size
    void *new_buffer;
    u8 *writer;
    // write UTF-8.
    switch (src_pykind) {
        case 1: {
            new_buffer = pymem_malloc_wrapped(max_utf8_bytes_per_ucs1 * len +
                                              __excess_bytes_write_ucs1_raw_utf8_trailing);
            return_if_unlikely(!new_buffer);
            writer = ssrjson_cast(u8 *, new_buffer);
            writer = bytes_write_ucs1_raw_utf8_wrapped(writer, src_voidp, len, is_key, is_compact);
            break;
        }
        case 2: {
            new_buffer = pymem_malloc_wrapped(max_utf8_bytes_per_ucs2 * len +
                                              __excess_bytes_write_ucs2_raw_utf8_trailing);
            return_if_unlikely(!new_buffer);
            writer = ssrjson_cast(u8 *, new_buffer);
            writer = bytes_write_ucs2_raw_utf8_wrapped(writer, src_voidp, len, is_key, is_compact);
            if (unlikely(!writer)) goto fail;
            break;
        }
        case 4: {
            new_buffer = pymem_malloc_wrapped(max_utf8_bytes_per_ucs4 * len +
                                              __excess_bytes_write_ucs4_raw_utf8_trailing);
            return_if_unlikely(!new_buffer);
            writer = ssrjson_cast(u8 *, new_buffer);
            writer = bytes_write_ucs4_raw_utf8_wrapped(writer, src_voidp, len, is_key, is_compact);
            if (unlikely(!writer)) goto fail;
            break;
        }
        default: {
            ssrjson_unreachable();
        }
    }
    //
    usize utf8_length = (usize)(writer - ssrjson_cast(u8 *, new_buffer));
    *utf8_length_out = utf8_length;
    // resize buffer
    void *resized_buffer = pymem_realloc_wrapped(new_buffer, utf8_length + 1);
    if (unlikely(!resized_buffer)) goto fail;
    //
    u8 *final_buffer = ssrjson_cast(u8 *, resized_buffer);
    final_buffer[utf8_length] = 0;
    *utf8_cache_out = final_buffer;
    return true;
fail:;
    pymem_free_wrapped(new_buffer);
    return false;
}

force_inline u8 *b_buf_apd_nonascii_str_write_cache(u8 *writer, const void *src_voidp, usize len, int src_pykind,
                                                    PyObject *str, ssrjson_compiletime bool is_compact) {
    const u8 *utf8_cache;
    usize utf8_length;
    get_utf8_cache(str, &utf8_cache, &utf8_length);
    if (!utf8_cache) {
        if (!USING_AVX512 && len < 6) {
            // For short strings, directly encode without caching
            // Why is 6: we assume that each character encodes to 3 bytes in most cases,
            // and 3 * 6 = 18 >= 16.
            goto no_cache_encode;
        }
        if (unlikely(!write_cache_impl(src_voidp, src_pykind, len, &utf8_cache, &utf8_length, false, is_compact)))
            return NULL;
        set_cache(str, &utf8_cache, utf8_length);
    }
    assert(utf8_cache);

    // Also see comment in b_buf_apd_ascii_key
    if (USING_AVX512 || utf8_length >= 16) {
        // is_compact is not needed here.
        *writer++ = '"';
        writer = bytes_write_ascii_noinline(writer, utf8_cache, utf8_length);
        *writer++ = '"';
        *writer++ = ',';
        return writer;
    } else {
    no_cache_encode:;
        *writer++ = '"';
        switch (src_pykind) {
            case 1: {
                writer = bytes_write_ucs1_str(writer, src_voidp, len, is_compact);
                break;
            }
            case 2: {
                writer = bytes_write_ucs2_str(writer, src_voidp, len, is_compact);
                if (unlikely(!writer)) return NULL;
                break;
            }
            case 4: {
                writer = bytes_write_ucs4_str(writer, src_voidp, len, is_compact);
                if (unlikely(!writer)) return NULL;
                break;
            }
            default: {
                ssrjson_unreachable();
            }
        }
        *writer++ = '"';
        *writer++ = ',';
        return writer;
    }
}

force_inline u8 *b_buf_apd_nonascii_str_no_write_cache(u8 *writer, const void *src_voidp, usize len, int src_pykind,
                                                       PyObject *str, ssrjson_compiletime bool is_compact) {
    const u8 *utf8_cache;
    usize utf8_length;
    get_utf8_cache(str, &utf8_cache, &utf8_length);
    // Also see comment in b_buf_apd_ascii_key
    if (utf8_cache && (USING_AVX512 || utf8_length >= 16)) {
        // is_compact is not needed here.
        *writer++ = '"';
        writer = bytes_write_ascii_noinline(writer, utf8_cache, utf8_length);
        *writer++ = '"';
        *writer++ = ',';
        return writer;
    } else {
        *writer++ = '"';
        switch (src_pykind) {
            case 1: {
                writer = bytes_write_ucs1_str(writer, src_voidp, len, is_compact);
                break;
            }
            case 2: {
                writer = bytes_write_ucs2_str(writer, src_voidp, len, is_compact);
                if (unlikely(!writer)) return NULL;
                break;
            }
            case 4: {
                writer = bytes_write_ucs4_str(writer, src_voidp, len, is_compact);
                if (unlikely(!writer)) return NULL;
                break;
            }
            default: {
                ssrjson_unreachable();
            }
        }
        *writer++ = '"';
        *writer++ = ',';
        return writer;
    }
}

#include "compile_context/s_out.inl.h"
#undef _CompileVectorBits

#endif // SSRJSON_ENCODE_NON_ASCII_H

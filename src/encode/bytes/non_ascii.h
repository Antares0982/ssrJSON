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

force_inline bool write_cache_impl(const void *src_voidp, int src_pykind, usize len, const u8 **utf8_cache_out, usize *utf8_length_out, bool is_key) {
    // Alloc to max size
    void *new_buffer;
    u8 *writer;
    u8 **writer_addr;
    // write UTF-8.
    switch (src_pykind) {
        case 1: {
            new_buffer = pymem_malloc_wrapped(max_utf8_bytes_per_ucs1 * len + __excess_bytes_write_ucs1_raw_utf8_trailing);
            RETURN_ON_UNLIKELY_ERR(!new_buffer);
            writer = SSRJSON_CAST(u8 *, new_buffer);
            writer_addr = &writer;
            bytes_write_ucs1_raw_utf8(writer_addr, src_voidp, len, is_key);
            break;
        }
        case 2: {
            new_buffer = pymem_malloc_wrapped(max_utf8_bytes_per_ucs2 * len + __excess_bytes_write_ucs2_raw_utf8_trailing);
            RETURN_ON_UNLIKELY_ERR(!new_buffer);
            writer = SSRJSON_CAST(u8 *, new_buffer);
            writer_addr = &writer;
            if (unlikely(!bytes_write_ucs2_raw_utf8(writer_addr, src_voidp, len, is_key))) goto fail;
            break;
        }
        case 4: {
            new_buffer = pymem_malloc_wrapped(max_utf8_bytes_per_ucs4 * len + __excess_bytes_write_ucs4_raw_utf8_trailing);
            RETURN_ON_UNLIKELY_ERR(!new_buffer);
            writer = SSRJSON_CAST(u8 *, new_buffer);
            writer_addr = &writer;
            if (unlikely(!bytes_write_ucs4_raw_utf8(writer_addr, src_voidp, len, is_key))) goto fail;
            break;
        }
        default: {
            SSRJSON_UNREACHABLE();
        }
    }
    //
    usize utf8_length = (usize)(writer - SSRJSON_CAST(u8 *, new_buffer));
    *utf8_length_out = utf8_length;
    // resize buffer
    void *resized_buffer = pymem_realloc_wrapped(new_buffer, utf8_length + 1);
    if (unlikely(!resized_buffer)) goto fail;
    //
    u8 *final_buffer = SSRJSON_CAST(u8 *, resized_buffer);
    final_buffer[utf8_length] = 0;
    *utf8_cache_out = final_buffer;
    return true;
fail:;
    pymem_free_wrapped(new_buffer);
    return false;
}

static force_noinline bool write_key_cache_impl(const void *src_voidp, int src_pykind, usize len, const u8 **utf8_cache_out, usize *utf8_length_out) {
    return write_cache_impl(src_voidp, src_pykind, len, utf8_cache_out, utf8_length_out, true);
}

static force_noinline bool write_str_cache_impl(const void *src_voidp, int src_pykind, usize len, const u8 **utf8_cache_out, usize *utf8_length_out) {
    return write_cache_impl(src_voidp, src_pykind, len, utf8_cache_out, utf8_length_out, false);
}

static force_noinline bool bytes_buffer_append_nonascii_str_write_cache(u8 **writer_addr, int src_pykind, const void *src_voidp, usize len, PyObject *str) {
    assert(SSRJSON_PYASCII_CAST(str)->state.compact);
    const u8 *utf8_cache;
    usize utf8_length;
    get_utf8_cache(str, &utf8_cache, &utf8_length);
    if (!utf8_cache) {
        if (unlikely(!write_str_cache_impl(src_voidp, src_pykind, len, &utf8_cache, &utf8_length))) return false;
        set_cache(str, &utf8_cache, &utf8_length);
    }
    assert(utf8_cache);

    // Also see comment in bytes_write_utf8
    if (USING_AVX512 || utf8_length >= 16) {
        bytes_write_utf8(writer_addr, utf8_cache, utf8_length, false);
        u8 *writer = *writer_addr;
        *writer++ = '"';
        *writer++ = ',';
        *writer_addr = writer;
        return true;
    } else {
        // return bytes_write_nonascii_str(writer_addr, src_pykind, src_voidp, len);
        switch (src_pykind) {
            case 1: {
                bytes_write_ucs1(writer_addr, src_voidp, len, false);
                break;
            }
            case 2: {
                if (unlikely(!bytes_write_ucs2(writer_addr, src_voidp, len, false))) return false;
                break;
            }
            case 4: {
                if (unlikely(!bytes_write_ucs4(writer_addr, src_voidp, len, false))) return false;
                break;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
        u8 *writer = *writer_addr;
        *writer++ = '"';
        *writer++ = ',';
        *writer_addr = writer;
        return true;
    }
}

static force_noinline bool bytes_buffer_append_nonascii_str_no_write_cache(u8 **writer_addr, int src_pykind, const void *src_voidp, usize len, PyObject *str) {
    assert(SSRJSON_CAST(PyASCIIObject *, str)->state.compact);
    const u8 *utf8_cache;
    usize utf8_length;
    get_utf8_cache(str, &utf8_cache, &utf8_length);
    // Also see comment in bytes_write_utf8
    if (utf8_cache && (USING_AVX512 || utf8_length >= 16)) {
        bytes_write_utf8(writer_addr, utf8_cache, utf8_length, false);
        u8 *writer = *writer_addr;
        *writer++ = '"';
        *writer++ = ',';
        *writer_addr = writer;
        return true;
    } else {
        switch (src_pykind) {
            case 1: {
                bytes_write_ucs1(writer_addr, src_voidp, len, false);
                break;
            }
            case 2: {
                if (unlikely(!bytes_write_ucs2(writer_addr, src_voidp, len, false))) return false;
                break;
            }
            case 4: {
                if (unlikely(!bytes_write_ucs4(writer_addr, src_voidp, len, false))) return false;
                break;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
        u8 *writer = *writer_addr;
        *writer++ = '"';
        *writer++ = ',';
        *writer_addr = writer;
        return true;
    }
}

#include "compile_context/s_out.inl.h"
#undef COMPILE_SIMD_BITS

#endif // SSRJSON_ENCODE_NON_ASCII_H

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

#ifndef SSRJSON_DECODE_STR_CACHE_KEY_H
#define SSRJSON_DECODE_STR_CACHE_KEY_H
#include "simd/long_cvt.h"
//
#include "simd/compile_feature_check.h"
//
#include "compile_context/s_in.inl.h"

force_inline void get_cache_key_hash_and_size_ucs1(const void **hash_string_ptr_addr, usize *hash_string_u8size_addr, const u8 *src, usize count, usize tpsize, bool need_size_cvt, void *temp_buffer) {
    // inplace case
    *hash_string_ptr_addr = src;
    *hash_string_u8size_addr = count * 1;
}

force_inline void get_cache_key_hash_and_size_ucs2(const void **hash_string_ptr_addr, usize *hash_string_u8size_addr, const u16 *src, usize count, usize tpsize, bool need_size_cvt, void *temp_buffer) {
    // complicated case when ucs level > 1
    if (need_size_cvt) {
        // do convert
        {
            assert(tpsize == 1);
            assert(count <= 64);
            u8 *temp_dst = temp_buffer;
            const u16 *temp_src = src;
            // use __partial_cvt to reduce some branches, also utilize SIMD registers
            // note: we always assume 32 bytes before src is readable
            // also, 128 bytes before temp_buffer is writable
            // i.e. we can read 16 u16 once
            usize temp_count = size_align_up(count, 16); //0,16,32,48 or 64
            usize more = temp_count - count;
            assert(temp_count <= 64);
            assert(more < 16);
            temp_dst -= more;
            temp_src -= more;
            usize cnt = temp_count / 16;
            for (usize i = 0; i < cnt; ++i) {
                __partial_cvt_16_u16_u8(&temp_dst, &temp_src);
            }
        }
        *hash_string_ptr_addr = temp_buffer;
        *hash_string_u8size_addr = count * tpsize;
    } else {
        // inplace case
        *hash_string_ptr_addr = src;
        *hash_string_u8size_addr = count * 2;
    }
}

force_inline void get_cache_key_hash_and_size_ucs4(const void **hash_string_ptr_addr, usize *hash_string_u8size_addr, const u32 *src, usize count, usize tpsize, bool need_size_cvt, void *temp_buffer) {
    // complicated case when ucs level > 1
    if (need_size_cvt) {
        // do inplace zip
        if (tpsize == 2) {
            // count <= 32
            assert(count <= 32);
            u16 *temp_dst = temp_buffer;
            const u32 *temp_src = src;
            // use __partial_cvt to reduce some branches, also utilize SIMD registers
            // note: we always assume 32 bytes before src is readable
            // also, 128 bytes before temp_buffer is writable
            // i.e. we can read 8 u32 once
            usize temp_count = size_align_up(count, 8); //0,8,16,24 or 32
            usize more = temp_count - count;
            assert(temp_count <= 32);
            assert(more < 8);
            temp_dst -= more;
            temp_src -= more;
            usize cnt = temp_count / 8;
            for (usize i = 0; i < cnt; ++i) {
                __partial_cvt_8_u32_u16(&temp_dst, &temp_src);
            }
        } else {
            assert(tpsize == 1);
            assert(count <= 64);
            u8 *temp_dst = temp_buffer;
            const u32 *temp_src = src;
            // use __partial_cvt, same as above
            usize temp_count = size_align_up(count, 8); //0,8,16,24,32,40,48,56 or 64
            usize more = temp_count - count;
            assert(temp_count <= 64);
            assert(more < 8);
            temp_dst -= more;
            temp_src -= more;
            usize cnt = temp_count / 8;
            for (usize i = 0; i < cnt; ++i) {
                __partial_cvt_8_u32_u8(&temp_dst, &temp_src);
            }
        }
        *hash_string_ptr_addr = temp_buffer;
        *hash_string_u8size_addr = count * tpsize;
    } else {
        // inplace case
        *hash_string_ptr_addr = src;
        *hash_string_u8size_addr = count * 4;
    }
}

#include "compile_context/s_out.inl.h"
#undef COMPILE_SIMD_BITS

#endif // SSRJSON_DECODE_STR_CACHE_KEY_H

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
#    include "encode/encode_shared.h"
#    include "utils/unicode.h"
#endif

#include "compile_context/w_in.inl.h"

force_inline bool unicode_buffer_reserve(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, usize size) {
    _dst_t *target_ptr = (*writer_addr) + size;
    if (unlikely(target_ptr > SSRJSON_CAST(_dst_t *, unicode_buffer_info->end))) {
        u8 *old_head = (u8 *)unicode_buffer_info->head;
        _dst_t *cur_writer = *writer_addr;
        usize target_size = SSRJSON_CAST(u8 *, target_ptr) - SSRJSON_CAST(u8 *, unicode_buffer_info->head);
        bool ret = _unicode_buffer_reserve(unicode_buffer_info, target_size);
        RETURN_ON_UNLIKELY_ERR(!ret);
        usize u8offset = SSRJSON_CAST(u8 *, cur_writer) - old_head;
        *writer_addr = SSRJSON_CAST(_dst_t *, SSRJSON_CAST(u8 *, unicode_buffer_info->head) + u8offset);
    }
    return true;
}

#include "compile_context/w_out.inl.h"

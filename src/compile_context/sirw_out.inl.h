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

#undef SSRJSON_COMPILE_CONTEXT_SIRW
//
#include "iw_out.inl.h"
#include "srw_out.inl.h"
//
#undef MAKE_SIRW_NAME
//
#undef unicode_buffer_append_key_internal
#undef unicode_buffer_append_str_internal
//
#undef STR_WRITER_IMPL
#undef KEY_WRITER_IMPL
//
#undef MAKE_IU_NAME
//
#undef prepare_unicode_write
#undef unicode_buffer_append_key
#undef unicode_buffer_append_key_distribute2
#undef unicode_buffer_append_key_distribute4
#undef unicode_buffer_append_str
#undef unicode_buffer_append_str_distribute2
#undef unicode_buffer_append_str_distribute4
#undef unicode_buffer_append_long
#undef write_unicode_false
#undef unicode_buffer_append_false
#undef write_unicode_true
#undef unicode_buffer_append_true
#undef write_unicode_null
#undef unicode_buffer_append_null
#undef unicode_buffer_append_float
#undef write_unicode_empty_arr
#undef unicode_buffer_append_empty_arr
#undef write_unicode_arr_begin
#undef unicode_buffer_append_arr_begin
#undef write_unicode_arr_end
#undef unicode_buffer_append_arr_end
#undef write_unicode_empty_obj
#undef unicode_buffer_append_empty_obj
#undef write_unicode_obj_begin
#undef unicode_buffer_append_obj_begin
#undef write_unicode_obj_end
#undef unicode_buffer_append_obj_end
#undef ssrjson_dumps_obj
//
#undef encode_process_val

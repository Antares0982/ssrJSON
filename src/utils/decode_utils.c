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

#include "decode/decode_shared.h"

#define BIGINT_IMPL 1
#include "decode/float/bigint.h"

bool _decode_obj_stack_resize(
        decode_obj_stack_ptr_t *decode_obj_writer_addr,
        decode_obj_stack_ptr_t *decode_obj_stack_addr,
        decode_obj_stack_ptr_t *decode_obj_stack_end_addr) {
    decode_obj_stack_ptr_t decode_obj_writer = *decode_obj_writer_addr;
    decode_obj_stack_ptr_t decode_obj_stack = *decode_obj_stack_addr;
    decode_obj_stack_ptr_t decode_obj_stack_end = *decode_obj_stack_end_addr;
    if (likely(SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE == decode_obj_stack_end - decode_obj_stack)) {
        void *new_buffer_void = malloc(sizeof(pyobj_ptr_t) * (SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1));
        decode_obj_stack_ptr_t new_buffer = new_buffer_void;
        if (unlikely(!new_buffer)) {
            PyErr_NoMemory();
            return false;
        }
        memcpy(new_buffer, decode_obj_stack, sizeof(pyobj_ptr_t) * SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE);
        *decode_obj_stack_addr = new_buffer;
        *decode_obj_writer_addr = new_buffer + SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE;
        *decode_obj_stack_end_addr = new_buffer + (SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1);
    } else {
        usize old_capacity = decode_obj_stack_end - decode_obj_stack;
        if (unlikely((PY_SSIZE_T_MAX >> 1) < old_capacity)) {
            PyErr_NoMemory();
            return false;
        }
        usize new_capacity = old_capacity << 1;
        void *new_buffer_void = realloc(decode_obj_stack, sizeof(PyObject *) * new_capacity);
        decode_obj_stack_ptr_t new_buffer = new_buffer_void;
        if (unlikely(!new_buffer)) {
            PyErr_NoMemory();
            return false;
        }
        *decode_obj_stack_addr = new_buffer;
        *decode_obj_writer_addr = new_buffer + old_capacity;
        *decode_obj_stack_end_addr = new_buffer + new_capacity;
    }
    return true;
}

#if !defined(Py_GIL_DISABLED)
ssrjson_align(64) u8 _DecodeTempBuffer[SSRJSON_STRING_BUFFER_SIZE];
decode_cache_t DecodeKeyCache[SSRJSON_KEY_CACHE_SIZE];
ssrjson_align(64) u8 _DecodeBytesSrcBuffer[SSRJSON_STRING_BUFFER_SIZE];
DecodeCtnWithSize _DecodeCtnBuffer[SSRJSON_DECODE_MAX_RECURSION];
PyObject *_DecodeObjBuffer[SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE];
#endif

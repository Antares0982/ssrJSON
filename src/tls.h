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

#ifndef SSRJSON_TLS_H
#define SSRJSON_TLS_H

#include "ssrjson_config.h"

#if defined(Py_GIL_DISABLED)

#    include <threads.h>
#    if defined(_POSIX_THREADS)
#        include <pthread.h>
#        define TLS_KEY_TYPE pthread_key_t
#    elif defined(NT_THREADS)
#        define WIN32_LEAN_AND_MEAN
#        include <windows.h>
#        define TLS_KEY_TYPE DWORD
#    else
#        error "Unknown thread model"
#    endif

/*==============================================================================
 * TLS related macros
 *============================================================================*/
#    if defined(_POSIX_THREADS)
#        define SSRJSON_DECLARE_TLS_GETTER(_key, _getter_name) \
            force_inline void *_getter_name(void) {            \
                return pthread_getspecific((_key));            \
            }
#        define SSRJSON_DECLARE_TLS_SETTER(_key, _setter_name) \
            force_inline bool _setter_name(void *ptr) {        \
                return 0 == pthread_setspecific(_key, ptr);    \
            }
#    else
#        define SSRJSON_DECLARE_TLS_GETTER(_key, _getter_name) \
            force_inline void *_getter_name(void) {            \
                return FlsGetValue((_key));                    \
            }
#        define SSRJSON_DECLARE_TLS_SETTER(_key, _setter_name) \
            force_inline bool _setter_name(void *ptr) {        \
                return FlsSetValue(_key, ptr);                 \
            }
#    endif

/*==============================================================================
 * TLS related API
 *============================================================================*/
bool ssrjson_tls_init(void);
bool ssrjson_tls_free(void);


/*==============================================================================
 * Thread Local Encode buffer
 *============================================================================*/
/* Encode TLS buffer key. */
extern TLS_KEY_TYPE _EncodeObjStackBuffer_Key;

/* The underlying data type to be stored. */


SSRJSON_DECLARE_TLS_GETTER(_EncodeObjStackBuffer_Key, _get_encode_obj_stack_buffer_pointer)
SSRJSON_DECLARE_TLS_SETTER(_EncodeObjStackBuffer_Key, _set_encode_obj_stack_buffer_pointer)

force_inline EncodeCtnWithIndex *get_encode_obj_stack_buffer(void) {
    void *value = _get_encode_obj_stack_buffer_pointer();
    if (unlikely(value == NULL)) {
        value = malloc(SSRJSON_ENCODE_MAX_RECURSION * sizeof(EncodeCtnWithIndex));
        if (unlikely(value == NULL)) return NULL;
        bool succ = _set_encode_obj_stack_buffer_pointer(value);
        if (unlikely(!succ)) {
            free(value);
            return NULL;
        }
    }
    return (EncodeCtnWithIndex *)value;
}

/*==============================================================================
 * Thread Local Decode buffer
 *============================================================================*/
/* Decode TLS buffer key. */
extern TLS_KEY_TYPE _DecodeCtnStackBuffer_Key;

typedef struct DecodeCtnWithSize {
    Py_ssize_t raw;
} DecodeCtnWithSize;

SSRJSON_DECLARE_TLS_GETTER(_DecodeCtnStackBuffer_Key, _get_decode_ctn_stack_buffer_pointer)
SSRJSON_DECLARE_TLS_SETTER(_DecodeCtnStackBuffer_Key, _set_decode_ctn_stack_buffer_pointer)

force_inline DecodeCtnWithSize *get_decode_ctn_stack_buffer(void) {
    void *value = _get_decode_ctn_stack_buffer_pointer();
    if (unlikely(value == NULL)) {
        value = malloc(SSRJSON_DECODE_MAX_RECURSION * sizeof(DecodeCtnWithSize));
        if (unlikely(value == NULL)) return NULL;
        bool succ = _set_decode_ctn_stack_buffer_pointer(value);
        if (unlikely(!succ)) {
            free(value);
            return NULL;
        }
    }
    return (DecodeCtnWithSize *)value;
}

/*==============================================================================
 * Thread Local Decode buffer
 *============================================================================*/
/* Decode TLS buffer key. */
extern TLS_KEY_TYPE _DecodeObjStackBuffer_Key;

// typedef struct DecodeCtnWithSize {
//     Py_ssize_t raw;
// } DecodeCtnWithSize;

SSRJSON_DECLARE_TLS_GETTER(_DecodeObjStackBuffer_Key, _get_decode_obj_stack_buffer_pointer)
SSRJSON_DECLARE_TLS_SETTER(_DecodeObjStackBuffer_Key, _set_decode_obj_stack_buffer_pointer)

force_inline decode_obj_stack_ptr_t get_decode_obj_stack_buffer(void) {
    void *value = _get_decode_obj_stack_buffer_pointer();
    if (unlikely(value == NULL)) {
        value = malloc(SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE * sizeof(PyObject *));
        if (unlikely(value == NULL)) return NULL;
        bool succ = _set_decode_obj_stack_buffer_pointer(value);
        if (unlikely(!succ)) {
            free(value);
            return NULL;
        }
    }
    return (decode_obj_stack_ptr_t)value;
}

#endif // defined(Py_GIL_DISABLED)

#endif // SSRJSON_TLS_H

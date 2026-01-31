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

#include "tls.h"
#include <assert.h>

/*==============================================================================
 * Thread local macros
 *============================================================================*/
#if defined(_POSIX_THREADS)
#    define DO_TLS_INIT(_key_, _destructor_) success = success && (0 == pthread_key_create(&_key_, _destructor_))
#    define DO_TLS_FREE(_key_) success = success && (0 == pthread_key_delete(_key_))
#else
#    define DO_TLS_INIT(_key_, _destructor_)         \
        if (success) _key_ = FlsAlloc(_destructor_); \
        if (_key_ == FLS_OUT_OF_INDEXES) success = false
#    define DO_TLS_FREE(_key_) success = success && FlsFree(_key_)
#endif

/*==============================================================================
 * Thread local key definition
 *============================================================================*/
TLS_KEY_TYPE _DecoderBufferLinkedList_Key;
#if !SSRJSON_GIL_ENABLED
TLS_KEY_TYPE _EncoderContainerBuffer_Key;
TLS_KEY_TYPE _DecoderKeyCache_Key;
#endif

/*==============================================================================
 * Thread local destructors
 *============================================================================*/
void _tls_decode_buffer_destructor(void *ptr) {
    if (ptr) {
#if !SSRJSON_GIL_ENABLED
        DecoderTLSData *tls_data_ptr = (DecoderTLSData *)ptr;
        if (tls_data_ptr->cur_buffer) {
            if (tls_data_ptr->cur_buffer->next) {
                SSRJSON_ALIGNED_FREE(tls_data_ptr->cur_buffer->next);
                tls_data_ptr->cur_buffer->next = NULL;
            }
            SSRJSON_ALIGNED_FREE(tls_data_ptr->cur_buffer);
            tls_data_ptr->cur_buffer = NULL;
        }
#endif
        free(ptr);
    }
}

#if !SSRJSON_GIL_ENABLED
void _tls_decode_key_cache_destructor(void *ptr) {
    if (ptr) {
        PyThreadState *tstate = PyThreadState_GetUnchecked();
        if (unlikely(tstate) && Py_IsInitialized()) {
            decode_cache_t *key_cache_arr = ptr;
            for (usize i = 0; i < SSRJSON_KEY_CACHE_SIZE; ++i) {
                Py_XDECREF(key_cache_arr[i].key);
            }
        }
        free(ptr);
    }
}
#endif

/*==============================================================================
 * Thread local init/free
 *============================================================================*/
bool ssrjson_tls_init(void) {
    bool success = true;
    DO_TLS_INIT(_DecoderBufferLinkedList_Key, _tls_decode_buffer_destructor);
#if !SSRJSON_GIL_ENABLED
    DO_TLS_INIT(_EncoderContainerBuffer_Key, free);
    DO_TLS_INIT(_DecoderKeyCache_Key, _tls_decode_key_cache_destructor);
#endif
    return success;
}

bool ssrjson_tls_free(void) {
    bool success = true;
    DO_TLS_FREE(_DecoderBufferLinkedList_Key);
#if !SSRJSON_GIL_ENABLED
    DO_TLS_FREE(_EncoderContainerBuffer_Key);
    DO_TLS_FREE(_DecoderKeyCache_Key);
#endif
    return success;
}

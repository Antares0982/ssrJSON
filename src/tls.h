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
//
#include "decode/decode_shared.h"
#if !SSRJSON_GIL_ENABLED
#    include "encode/encode_typedefs.h"
#endif

#if defined(_POSIX_THREADS)
#    include <pthread.h>
#    define TLS_KEY_TYPE pthread_key_t
#elif defined(NT_THREADS)
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#    define TLS_KEY_TYPE DWORD
#else
#    error "Unknown thread model"
#endif
// malloc, free. Default to standard library.
#ifndef SSRJSON_TLS_MALLOC
#    define SSRJSON_TLS_MALLOC(size) malloc((size))
#endif
#ifndef SSRJSON_TLS_CALLOC
#    define SSRJSON_TLS_CALLOC(count, size) calloc((count), (size))
#endif
#ifndef SSRJSON_TLS_FREE
#    define SSRJSON_TLS_FREE(ptr) free((ptr))
#endif

/*==============================================================================
 * TLS related macros
 *============================================================================*/
#if defined(_POSIX_THREADS)
#    define SSRJSON_DECLARE_TLS_GETTER(_key, _getter_name) \
        force_inline void *_getter_name(void) { return pthread_getspecific((_key)); }
#    define SSRJSON_DECLARE_TLS_SETTER(_key, _setter_name) \
        force_inline bool _setter_name(void *ptr) { return 0 == pthread_setspecific(_key, ptr); }
#else
#    define SSRJSON_DECLARE_TLS_GETTER(_key, _getter_name) \
        force_inline void *_getter_name(void) { return FlsGetValue((_key)); }
#    define SSRJSON_DECLARE_TLS_SETTER(_key, _setter_name) \
        force_inline bool _setter_name(void *ptr) { return FlsSetValue(_key, ptr); }
#endif

/*==============================================================================
 * TLS related API
 *============================================================================*/
bool _ssrjson_library_tls_init(void);
bool _ssrjson_library_tls_free(void);

/*==============================================================================
 * Thread local decode buffer linked list, for object_hook
 *============================================================================*/
extern TLS_KEY_TYPE _DecoderBufferLinkedList_Key;

SSRJSON_DECLARE_TLS_GETTER(_DecoderBufferLinkedList_Key, _get_decoder_buffer_linked_list_pointer)
SSRJSON_DECLARE_TLS_SETTER(_DecoderBufferLinkedList_Key, _set_decoder_buffer_linked_list_pointer)

force_inline DecoderTLSData *tls_get_decoder_data(void) {
    void *value = _get_decoder_buffer_linked_list_pointer();
    if (unlikely(value == NULL)) {
        value = SSRJSON_TLS_MALLOC(sizeof(DecoderTLSData));
        if (unlikely(value == NULL)) return NULL;
        memset(value, 0, sizeof(DecoderTLSData));
        bool succ = _set_decoder_buffer_linked_list_pointer(value);
        if (unlikely(!succ)) {
            SSRJSON_TLS_FREE(value);
            return NULL;
        }
    }
    return (DecoderTLSData *)value;
}

/*==============================================================================
 * Thread local encoder container buffer
 *============================================================================*/
#if !SSRJSON_GIL_ENABLED

extern TLS_KEY_TYPE _EncoderContainerBuffer_Key;

SSRJSON_DECLARE_TLS_GETTER(_EncoderContainerBuffer_Key, _get_encoder_buffer_pointer)
SSRJSON_DECLARE_TLS_SETTER(_EncoderContainerBuffer_Key, _set_encoder_buffer_pointer)

force_inline EncodeCtnWithIndex *get_encode_obj_stack_buffer(void) {
    void *value = _get_encoder_buffer_pointer();
    if (unlikely(value == NULL)) {
        value = SSRJSON_TLS_MALLOC(sizeof(EncodeCtnWithIndex) * SSRJSON_ENCODE_MAX_RECURSION);
        if (unlikely(value == NULL)) return NULL;
        // memset(value, 0, sizeof(DecoderTLSData));
        bool succ = _set_encoder_buffer_pointer(value);
        if (unlikely(!succ)) {
            SSRJSON_TLS_FREE(value);
            return NULL;
        }
    }
    return (EncodeCtnWithIndex *)value;
}
#endif

/*==============================================================================
 * Thread local decoder key cache
 *============================================================================*/
#if !SSRJSON_GIL_ENABLED
extern TLS_KEY_TYPE _DecoderKeyCache_Key;

SSRJSON_DECLARE_TLS_GETTER(_DecoderKeyCache_Key, _get_decoder_key_cache_pointer)
SSRJSON_DECLARE_TLS_SETTER(_DecoderKeyCache_Key, _set_decoder_key_cache_pointer)

force_inline decode_cache_t *get_tls_decoder_key_cache(void) {
    void *value = _get_decoder_key_cache_pointer();
    if (unlikely(value == NULL)) {
        value = SSRJSON_TLS_CALLOC(SSRJSON_KEY_CACHE_SIZE, sizeof(decode_cache_t));
        if (unlikely(value == NULL)) return NULL;
        bool succ = _set_decoder_key_cache_pointer(value);
        if (unlikely(!succ)) {
            SSRJSON_TLS_FREE(value);
            return NULL;
        }
    }
    return (decode_cache_t *)value;
}
#endif

#endif // SSRJSON_TLS_H

#include "tls.h"
#include "assert.h"

#if defined(Py_GIL_DISABLED)

TLS_KEY_TYPE _EncodeObjStackBuffer_Key;
TLS_KEY_TYPE _DecodeObjStackBuffer_Key;
TLS_KEY_TYPE _DecodeCtnStackBuffer_Key;

void _tls_buffer_destructor(void *ptr) {
    if (ptr) free(ptr);
}

bool ssrjson_tls_init(void) {
    bool success = true;
#    if defined(_POSIX_THREADS)
    success = success && (0 == pthread_key_create(&_EncodeObjStackBuffer_Key, _tls_buffer_destructor));
    success = success && (0 == pthread_key_create(&_DecodeObjStackBuffer_Key, _tls_buffer_destructor));
    success = success && (0 == pthread_key_create(&_DecodeCtnStackBuffer_Key, _tls_buffer_destructor));
#    else
    if (success) _EncodeObjStackBuffer_Key = FlsAlloc(_tls_buffer_destructor);
    if (_EncodeObjStackBuffer_Key == FLS_OUT_OF_INDEXES) success = false;
    if (success) _DecodeObjStackBuffer_Key = FlsAlloc(_tls_buffer_destructor);
    if (_DecodeObjStackBuffer_Key == FLS_OUT_OF_INDEXES) success = false;
    if (success) _DecodeCtnStackBuffer_Key = FlsAlloc(_tls_buffer_destructor);
    if (_DecodeCtnStackBuffer_Key == FLS_OUT_OF_INDEXES) success = false;
#    endif
    return success;
}

bool ssrjson_tls_free(void) {
    bool success = true;
#    if defined(_POSIX_THREADS)
    success = success && (0 == pthread_key_delete(_EncodeObjStackBuffer_Key));
    success = success && (0 == pthread_key_delete(_DecodeObjStackBuffer_Key));
    success = success && (0 == pthread_key_delete(_DecodeCtnStackBuffer_Key));
#    else
    success = success && FlsFree(_EncodeObjStackBuffer_Key);
    success = success && FlsFree(_DecodeObjStackBuffer_Key);
    success = success && FlsFree(_DecodeCtnStackBuffer_Key);
#    endif
    return success;
}
#endif // defined(Py_GIL_DISABLED)

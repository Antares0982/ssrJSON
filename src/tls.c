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

TLS_KEY_TYPE _DecoderBufferLinkedList_Key;

void _tls_buffer_destructor(void *ptr) {
    if (ptr) free(ptr);
}

bool ssrjson_tls_init(void) {
    bool success = true;
#if defined(_POSIX_THREADS)
    success = success && (0 == pthread_key_create(&_DecoderBufferLinkedList_Key, _tls_buffer_destructor));
#else
    if (success) _DecoderBufferLinkedList_Key = FlsAlloc(_tls_buffer_destructor);
    if (_DecoderBufferLinkedList_Key == FLS_OUT_OF_INDEXES) success = false;
#endif
    return success;
}

bool ssrjson_tls_free(void) {
    bool success = true;
#if defined(_POSIX_THREADS)
    success = success && (0 == pthread_key_delete(_DecoderBufferLinkedList_Key));
#else
    success = success && FlsFree(_DecoderBufferLinkedList_Key);
#endif
    return success;
}

// #endif // defined(Py_GIL_DISABLED)

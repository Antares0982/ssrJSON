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

#ifndef SSRJSON_SCALAR_FULL_H
#define SSRJSON_SCALAR_FULL_H
#include "common.h"
#define COMPILE_READ_UCS_LEVEL 1
#include "simd/neon/checker/_sr_escape.inl.h"
#undef COMPILE_READ_UCS_LEVEL
#define COMPILE_READ_UCS_LEVEL 2
#include "simd/neon/checker/_sr_escape.inl.h"
#undef COMPILE_READ_UCS_LEVEL
#define COMPILE_READ_UCS_LEVEL 4
#include "simd/neon/checker/_sr_escape.inl.h"
#undef COMPILE_READ_UCS_LEVEL
#if defined(COMPILE_CONTEXT_DECODE)
#    define COMPILE_READ_UCS_LEVEL 1
#    include "decode.inl.h"
#    include "simd/neon/decode/_r_decode_tools.inl.h"
#    undef COMPILE_READ_UCS_LEVEL
#    define COMPILE_READ_UCS_LEVEL 2
#    include "decode.inl.h"
#    include "simd/neon/decode/_r_decode_tools.inl.h"
#    undef COMPILE_READ_UCS_LEVEL
#    define COMPILE_READ_UCS_LEVEL 4
#    include "decode.inl.h"
#    include "simd/neon/decode/_r_decode_tools.inl.h"
#    undef COMPILE_READ_UCS_LEVEL
#endif
#if defined(COMPILE_CONTEXT_ENCODE)
#    include "utf8.h"
#    define COMPILE_READ_UCS_LEVEL 1
#    define COMPILE_WRITE_UCS_LEVEL 1
#    include "encode.inl.h"
#    undef COMPILE_READ_UCS_LEVEL
#    undef COMPILE_WRITE_UCS_LEVEL
#    define COMPILE_READ_UCS_LEVEL 1
#    define COMPILE_WRITE_UCS_LEVEL 2
#    include "encode.inl.h"
#    undef COMPILE_READ_UCS_LEVEL
#    undef COMPILE_WRITE_UCS_LEVEL
#    define COMPILE_READ_UCS_LEVEL 1
#    define COMPILE_WRITE_UCS_LEVEL 4
#    include "encode.inl.h"
#    undef COMPILE_READ_UCS_LEVEL
#    undef COMPILE_WRITE_UCS_LEVEL
#    define COMPILE_READ_UCS_LEVEL 2
#    define COMPILE_WRITE_UCS_LEVEL 2
#    include "encode.inl.h"
#    undef COMPILE_READ_UCS_LEVEL
#    undef COMPILE_WRITE_UCS_LEVEL
#    define COMPILE_READ_UCS_LEVEL 2
#    define COMPILE_WRITE_UCS_LEVEL 4
#    include "encode.inl.h"
#    undef COMPILE_READ_UCS_LEVEL
#    undef COMPILE_WRITE_UCS_LEVEL
#    define COMPILE_READ_UCS_LEVEL 4
#    define COMPILE_WRITE_UCS_LEVEL 4
#    include "encode.inl.h"
#    undef COMPILE_READ_UCS_LEVEL
#    undef COMPILE_WRITE_UCS_LEVEL
#endif
#endif

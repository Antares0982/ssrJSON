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

#ifndef SSRJSON_DECODE_DECODE_BYTES_ROOT_WRAP_H
#define SSRJSON_DECODE_DECODE_BYTES_ROOT_WRAP_H

#include "decode_shared.h"
#include "decode_str_wrap.h"
#include "str/ascii.h"
#include "str/tools.h"
//
#include "simd/compile_feature_check.h"

#define READ_ROOT_IMPL read_bytes_root_pretty
#define DECODE_READ_PRETTY 1
#include "bytes/_decode_bytes_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL

#define READ_ROOT_IMPL read_bytes_root_minify
#define DECODE_READ_PRETTY 0
#include "bytes/_decode_bytes_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL

#undef COMPILE_SIMD_BITS

#endif // SSRJSON_DECODE_DECODE_BYTES_ROOT_WRAP_H

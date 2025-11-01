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

#ifndef ENCODE_BYTES_IMPL_WRAP_H
#define ENCODE_BYTES_IMPL_WRAP_H

#include "encode/encode_impl_wrap.h"
#include "encode/encode_shared.h"
#include "non_ascii.h"
#include "pyutils.h"
#include "ssrjson.h"
#include "tls.h"
#include "utils/unicode.h"

#define COMPILE_INDENT_LEVEL 0
#include "_encode_bytes_impl.inl.h"
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 2
#include "_encode_bytes_impl.inl.h"
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 4
#include "_encode_bytes_impl.inl.h"
#undef COMPILE_INDENT_LEVEL

#endif // ENCODE_BYTES_IMPL_WRAP_H

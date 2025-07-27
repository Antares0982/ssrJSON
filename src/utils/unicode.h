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

#ifndef SSRJSON_UNICODE_UNICODE_H
#define SSRJSON_UNICODE_UNICODE_H

#include "ssrjson.h"

#define PYUNICODE_ASCII_START(_obj_) SSRJSON_CAST(u8 *, SSRJSON_CAST(PyASCIIObject *, (_obj_)) + 1)
#define PYUNICODE_UCS1_START(_obj_) SSRJSON_CAST(u8 *, SSRJSON_CAST(PyCompactUnicodeObject *, (_obj_)) + 1)
#define PYUNICODE_UCS2_START(_obj_) SSRJSON_CAST(u16 *, SSRJSON_CAST(PyCompactUnicodeObject *, (_obj_)) + 1)
#define PYUNICODE_UCS4_START(_obj_) SSRJSON_CAST(u32 *, SSRJSON_CAST(PyCompactUnicodeObject *, (_obj_)) + 1)

force_noinline void init_pyunicode_noinline(void *, Py_ssize_t size, int kind);

#endif // SSRJSON_UNICODE_UNICODE_H

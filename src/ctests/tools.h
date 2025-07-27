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

#ifndef SSRJSON_CTESTS_TOOLS_H
#define SSRJSON_CTESTS_TOOLS_H

#include "test.h"

uintptr_t find_extension_symbol(const char *symbol_name);

int check_ascii_ascii(u8 *input, u8 *output, int count);
int check_ucs1_2bytes(u8 *input, u8 *output, int count);
int check_ucs1_ascii(u8 *input, u8 *output, int count);
int check_ucs2_3bytes(u16 *input, u8 *output, int count);
int check_ucs2_2bytes(u16 *input, u8 *output, int count);
int check_ucs2_ascii(u16 *input, u8 *output, int count);
int check_ucs4_4bytes(u32 *input, u8 *output, int count);
int check_ucs4_3bytes(u32 *input, u8 *output, int count);
int check_ucs4_2bytes(u32 *input, u8 *output, int count);
int check_ucs4_ascii(u32 *input, u8 *output, int count);

#endif // SSRJSON_CTESTS_TOOLS_H

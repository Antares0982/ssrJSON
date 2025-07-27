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

#include "test.h"
#ifdef _WIN32
#    include <windows.h>
#else
#    include <dlfcn.h>
#endif

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define U16_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN
#define U32_TO_BINARY_PATTERN U16_TO_BINARY_PATTERN U16_TO_BINARY_PATTERN
#define BYTE_TO_BINARY(_u) ((_u) & 0x80 ? '1' : '0'), \
                           ((_u) & 0x40 ? '1' : '0'), \
                           ((_u) & 0x20 ? '1' : '0'), \
                           ((_u) & 0x10 ? '1' : '0'), \
                           ((_u) & 0x08 ? '1' : '0'), \
                           ((_u) & 0x04 ? '1' : '0'), \
                           ((_u) & 0x02 ? '1' : '0'), \
                           ((_u) & 0x01 ? '1' : '0')
#define U16_TO_BINARY(_u) ((_u) & 0x8000 ? '1' : '0'), \
                          ((_u) & 0x4000 ? '1' : '0'), \
                          ((_u) & 0x2000 ? '1' : '0'), \
                          ((_u) & 0x1000 ? '1' : '0'), \
                          ((_u) & 0x800 ? '1' : '0'),  \
                          ((_u) & 0x400 ? '1' : '0'),  \
                          ((_u) & 0x200 ? '1' : '0'),  \
                          ((_u) & 0x100 ? '1' : '0'),  \
                          ((_u) & 0x80 ? '1' : '0'),   \
                          ((_u) & 0x40 ? '1' : '0'),   \
                          ((_u) & 0x20 ? '1' : '0'),   \
                          ((_u) & 0x10 ? '1' : '0'),   \
                          ((_u) & 0x08 ? '1' : '0'),   \
                          ((_u) & 0x04 ? '1' : '0'),   \
                          ((_u) & 0x02 ? '1' : '0'),   \
                          ((_u) & 0x01 ? '1' : '0')
#define U32_TO_BINARY(_u) ((_u) & 0x80000000 ? '1' : '0'), \
                          ((_u) & 0x40000000 ? '1' : '0'), \
                          ((_u) & 0x20000000 ? '1' : '0'), \
                          ((_u) & 0x10000000 ? '1' : '0'), \
                          ((_u) & 0x8000000 ? '1' : '0'),  \
                          ((_u) & 0x4000000 ? '1' : '0'),  \
                          ((_u) & 0x2000000 ? '1' : '0'),  \
                          ((_u) & 0x1000000 ? '1' : '0'),  \
                          ((_u) & 0x800000 ? '1' : '0'),   \
                          ((_u) & 0x400000 ? '1' : '0'),   \
                          ((_u) & 0x200000 ? '1' : '0'),   \
                          ((_u) & 0x100000 ? '1' : '0'),   \
                          ((_u) & 0x80000 ? '1' : '0'),    \
                          ((_u) & 0x40000 ? '1' : '0'),    \
                          ((_u) & 0x20000 ? '1' : '0'),    \
                          ((_u) & 0x10000 ? '1' : '0'),    \
                          ((_u) & 0x8000 ? '1' : '0'),     \
                          ((_u) & 0x4000 ? '1' : '0'),     \
                          ((_u) & 0x2000 ? '1' : '0'),     \
                          ((_u) & 0x1000 ? '1' : '0'),     \
                          ((_u) & 0x800 ? '1' : '0'),      \
                          ((_u) & 0x400 ? '1' : '0'),      \
                          ((_u) & 0x200 ? '1' : '0'),      \
                          ((_u) & 0x100 ? '1' : '0'),      \
                          ((_u) & 0x80 ? '1' : '0'),       \
                          ((_u) & 0x40 ? '1' : '0'),       \
                          ((_u) & 0x20 ? '1' : '0'),       \
                          ((_u) & 0x10 ? '1' : '0'),       \
                          ((_u) & 0x08 ? '1' : '0'),       \
                          ((_u) & 0x04 ? '1' : '0'),       \
                          ((_u) & 0x02 ? '1' : '0'),       \
                          ((_u) & 0x01 ? '1' : '0')
#define TEST_STRINGIZE_EX(_x) #_x
#define TEST_STRINGIZE(_x) TEST_STRINGIZE_EX(_x)

uintptr_t find_extension_symbol(const char *symbol_name) {
#ifdef _WIN32
    static HMODULE handle = NULL;
    if (!handle) handle = GetModuleHandle(NULL);
    if (!handle) return 0;
    uintptr_t ret = (uintptr_t)GetProcAddress(handle, symbol_name);
    return ret;
#else
    static void *handle = NULL;
    if (!handle) handle = dlopen(NULL, RTLD_NOW);
    if (!handle) return 0;
    uintptr_t ret = (uintptr_t)dlsym(handle, symbol_name);
    return ret;
#endif
}

bool check_unicode_encode(u32 origin_unicode, u8 *bytes_rep, int index_for_print) {
    bool same;
    u8 u[4] = {0};
    int size;
    if (origin_unicode <= 0x7f) {
        size = 1;
        u[0] = (u8)origin_unicode;
        same = !memcmp(bytes_rep, u, 1);
    } else if (origin_unicode <= 0x7ff) {
        size = 2;
        u[0] = (u8)((origin_unicode >> 6) | 0xc0);
        u[1] = (u8)((origin_unicode & 0x3f) | 0x80);
        same = !memcmp(bytes_rep, u, 2);
    } else if (origin_unicode <= 0xffff) {
        size = 3;
        u[0] = (u8)((origin_unicode >> 12) | 0xe0);
        u[1] = (u8)(((origin_unicode >> 6) & 0x3f) | 0x80);
        u[2] = (u8)((origin_unicode & 0x3f) | 0x80);
        same = !memcmp(bytes_rep, u, 3);
    } else {
        size = 4;
        u[0] = (u8)((origin_unicode >> 18) | 0xf0);
        u[1] = (u8)(((origin_unicode >> 12) & 0x3f) | 0x80);
        u[2] = (u8)(((origin_unicode >> 6) & 0x3f) | 0x80);
        u[3] = (u8)((origin_unicode & 0x3f) | 0x80);
        same = !memcmp(bytes_rep, u, 4);
    }
    if (!same) {
        u8 cp[4] = {0};
        memcpy(cp, bytes_rep, size);
        printf("at index: %d, origin_unicode: %u, byte size: %d\n", index_for_print, origin_unicode, size);
        printf("original unicode encodes to: " BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(u[0]), BYTE_TO_BINARY(u[1]), BYTE_TO_BINARY(u[2]), BYTE_TO_BINARY(u[3]));
        printf("while the output is        : " BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN " " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(cp[0]), BYTE_TO_BINARY(cp[1]), BYTE_TO_BINARY(cp[2]), BYTE_TO_BINARY(cp[3]));
    }
    return same;
}

int check_ascii_ascii(u8 *input, u8 *output, int count) {
    for (int i = 0; i < count; ++i) {
        CHECK(check_unicode_encode(input[i], output + 1 * i, i));
    }
    return PASSED;
}

int check_ucs1_2bytes(u8 *input, u8 *output, int count) {
    for (int i = 0; i < count; ++i) {
        CHECK(check_unicode_encode(input[i], output + 2 * i, i));
    }
    return PASSED;
}

int check_ucs1_ascii(u8 *input, u8 *output, int count) {
    for (int i = 0; i < count; ++i) {
        CHECK(check_unicode_encode(input[i], output + 1 * i, i));
    }
    return PASSED;
}

int check_ucs2_3bytes(u16 *input, u8 *output, int count) {
    for (int i = 0; i < count; ++i) {
        CHECK(check_unicode_encode(input[i], output + 3 * i, i));
    }
    return PASSED;
}

int check_ucs2_2bytes(u16 *input, u8 *output, int count) {
    for (int i = 0; i < count; ++i) {
        CHECK(check_unicode_encode(input[i], output + 2 * i, i));
    }
    return PASSED;
}

int check_ucs2_ascii(u16 *input, u8 *output, int count) {
    for (int i = 0; i < count; ++i) {
        CHECK(check_unicode_encode(input[i], output + 1 * i, i));
    }
    return PASSED;
}

int check_ucs4_4bytes(u32 *input, u8 *output, int count) {
    for (int i = 0; i < count; ++i) {
        CHECK(check_unicode_encode(input[i], output + 4 * i, i));
    }
    return PASSED;
}

int check_ucs4_3bytes(u32 *input, u8 *output, int count) {
    for (int i = 0; i < count; ++i) {
        CHECK(check_unicode_encode(input[i], output + 3 * i, i));
    }
    return PASSED;
}

int check_ucs4_2bytes(u32 *input, u8 *output, int count) {
    for (int i = 0; i < count; ++i) {
        CHECK(check_unicode_encode(input[i], output + 2 * i, i));
    }
    return PASSED;
}

int check_ucs4_ascii(u32 *input, u8 *output, int count) {
    for (int i = 0; i < count; ++i) {
        CHECK(check_unicode_encode(input[i], output + 1 * i, i));
    }
    return PASSED;
}

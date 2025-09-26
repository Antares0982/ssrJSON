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

#ifndef SSRJSON_CTESTS_TEST_H
#define SSRJSON_CTESTS_TEST_H
#include "simd/mask_table.h"
#include "simd/simd_impl.h"

/* Helper macros. */

#define TEST_STRINGIZE_EX(_x) #_x
#define TEST_STRINGIZE(_x) TEST_STRINGIZE_EX(_x)

#define CHECK(_x)                   \
    do {                            \
        bool _check_result_ = (_x); \
        if (!_check_result_) {      \
            return FAILED;          \
        }                           \
    } while (0)

#define RANDOM_FILL(_x)                        \
    do {                                       \
        fill_random_buffer(&(_x), sizeof(_x)); \
    } while (0)

#define RANDOM_FILLPTR_WITH_SIZE(_x, _u8size_) \
    do {                                       \
        fill_random_buffer((_x), (_u8size_));  \
    } while (0)

#define GARBAGE_FILL(_x)                 \
    do {                                 \
        memset(&(_x), 0xfa, sizeof(_x)); \
    } while (0)

#define GARBAGE_FILLPTR_WITH_SIZE(_x, _u8size_) \
    do {                                        \
        memset((_x), 0xfa, (_u8size_));         \
    } while (0)

#define ZERO_FILL(_x)                 \
    do {                              \
        memset(&(_x), 0, sizeof(_x)); \
    } while (0)

#define ZERO_FILLPTR_WITH_SIZE(_x, _u8size_) \
    do {                                     \
        memset((_x), 0, (_u8size_));         \
    } while (0)

static const int INVALID = -1;
static const int FAILED = 0;
static const int PASSED = 1;
static const int SKIPPED = 2;

extern bool _SupportAVX512;
extern bool _SupportAVX2;

/* Helper functions. */

force_inline void fill_random_buffer(void *_buffer, usize length) {
    u8 *buffer = (u8 *)_buffer;
    for (usize i = 0; i < length; i++) {
        buffer[i] = rand() & 0xff;
    }
}

force_inline u32 get_random_in_range(int a, int b) {
    if (a > b) {
        int tmp = a;
        a = b;
        b = tmp;
    }
    int r = rand();
    int range_size = b - a;
    r = (r % range_size);
    return r + a;
}

force_inline u8 get_random_ascii_u8(void) {
    return (u8)get_random_in_range(0, 0x80);
}

force_inline u16 get_random_2bytes_u16(void) {
    return (u16)get_random_in_range(0x80, 0x800);
}

force_inline u16 get_random_3bytes_u16(void) {
    return (u16)get_random_in_range(0x800, 0x10000);
}

force_inline u32 get_random_4bytes_u32(void) {
    return (u32)get_random_in_range(0x10000, 0x110000);
}

/* DECLARE_TEST macro. */
#if BUILD_MULTI_LIB && SSRJSON_X86
#    define DECLARE_TEST(_name)   \
        int _name##_sse4_2(void); \
        int _name##_avx2(void);   \
        int _name##_avx512(void);
#elif BUILD_MULTI_LIB && SSRJSON_AARCH
#    define DECLARE_TEST(_name) int _name##_neon(void);
#else
#    define DECLARE_TEST(_name) int _name(void);
#endif

/* Declare tests. */

DECLARE_TEST(test_cvt_u8_to_u16)
DECLARE_TEST(test_cvt_u8_to_u32)
DECLARE_TEST(test_cvt_u16_to_u32)
DECLARE_TEST(test_ucs2_encode_3bytes_utf8)
DECLARE_TEST(test_ucs2_encode_2bytes_utf8)
DECLARE_TEST(test_ucs4_encode_3bytes_utf8)
DECLARE_TEST(test_ucs4_encode_2bytes_utf8)
DECLARE_TEST(test_long_back_cvt_u8_u16)
DECLARE_TEST(test_long_cvt)

#endif // SSRJSON_CTESTS_TEST_H

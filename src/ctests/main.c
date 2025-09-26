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


#include "ssrjson.h"
#include "test.h"
#include "test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if SSRJSON_X86
bool _SupportAVX512 = false;
bool _SupportAVX2 = false;

void check_avx512(void) {
    int info[4];
    cpuid_count(info, 7, 0);
    int ebx = info[1];
    _SupportAVX512 = (ebx & (1 << 16)) && (ebx & (1 << 30));
}

void check_avx2(void) {
    int info[4];
    cpuid_count(info, 7, 0);
    int ebx = info[1];
    _SupportAVX2 = ebx & (1 << 5);
}
#endif

#define _RED "\033[31m"
#define _GREEN "\033[32m"
#define _YELLOW "\033[33m"
#define _CLEAR "\033[0m"

typedef struct TestCounter {
    int total_count;
    int skipped_count;
    int passed_count;
} TestCounter;

bool wrap_run_test(int (*func)(void), const char *name, TestCounter *counter) {
    int ret = func();
    if (ret == INVALID) return true;
    printf("RUNNING TEST: %-50s", name);
    counter->total_count++;
    if (ret == PASSED) {
        printf(" " _GREEN "PASSED" _CLEAR "\n");
        counter->passed_count++;
    } else if (ret == SKIPPED) {
        printf(" " _YELLOW "SKIPPED" _CLEAR "\n");
        counter->skipped_count++;
    } else {
        printf(" " _RED "!!!!!FAILED" _CLEAR "\n");
    }
    return ret;
}

#define RUN_ONE_TEST(_name) wrap_run_test(_name, #_name, &counter)
#if BUILD_MULTI_LIB && SSRJSON_X86
#    define RUN_TESTS(_name)              \
        do {                              \
            RUN_ONE_TEST(_name##_avx512); \
            RUN_ONE_TEST(_name##_avx2);   \
            RUN_ONE_TEST(_name##_sse4_2); \
        } while (0)
#elif BUILD_MULTI_LIB && SSRJSON_AARCH
#    define RUN_TESTS(_name)            \
        do {                            \
            RUN_ONE_TEST(_name##_neon); \
        } while (0)
#else
#    define RUN_TESTS(_name) RUN_ONE_TEST(_name);
#endif

bool show_test_counter(TestCounter *counter) {
    int failed = counter->total_count - (counter->passed_count + counter->skipped_count);
    bool success = !failed;
    printf("==================================================================================\n");
    if (success) {
        printf(_GREEN "Summary: ALL PASSED, %d tests in total, %d passed, %d skipped." _CLEAR "\n", counter->total_count, counter->passed_count, counter->skipped_count);
    } else {
        printf(_RED "Summary: %d tests in total, %d passed, %d skipped, %d failed." _CLEAR "\n", counter->total_count, counter->passed_count, counter->skipped_count, failed);
    }
    return success;
}

bool run_c_tests(void) {
#if SSRJSON_X86
    bool support_avx512 = _SupportAVX512;
    bool support_avx2 = _SupportAVX2;
#endif
    TestCounter counter;
    ZERO_FILL(counter);

    RUN_TESTS(test_cvt_u8_to_u16);
    RUN_TESTS(test_cvt_u8_to_u32);
    RUN_TESTS(test_cvt_u16_to_u32);
    RUN_TESTS(test_ucs2_encode_3bytes_utf8);
    RUN_TESTS(test_ucs2_encode_2bytes_utf8);
    RUN_TESTS(test_ucs4_encode_3bytes_utf8);
    RUN_TESTS(test_ucs4_encode_2bytes_utf8);
    RUN_TESTS(test_long_back_cvt_u8_u16);
    RUN_TESTS(test_long_cvt);

    return show_test_counter(&counter);
}

int main(int argc, char **argv) {
    int ret = 0;
    PyObject *pModule = NULL;
    if (!initialize_cpython()) {
        fprintf(stderr, "Fail to initialize");
        return 1;
    }
    pModule = import_ssrjson();
    if (!pModule) {
        fprintf(stderr, "Fail to import ssrjson");
        ret = 1;
        goto done;
    }
    srand((u32)time(NULL));
#if SSRJSON_X86
    check_avx2();
    check_avx512();
#endif
    //
    //
    bool run_test_result = run_c_tests();
    if (!run_test_result) ret = 1;

done:
    Py_XDECREF(pModule);
    Py_Finalize();
    return ret;
}

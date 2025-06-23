#define COMPILE_CONTEXT_ENCODE
#define COMPILE_CONTEXT_DECODE
#include "test.h"
#include "simd/simd_detect.h"
#include "simd/vector_types.h"
#include "tools.h"
//
#include "simd/compile_feature_check.h"
//
#include "compile_context/s_in.inl.h"

#if BUILD_MULTI_LIB && SSRJSON_X86
#    if COMPILE_SIMD_BITS == 512
#        define GUARDED_SIMD                         \
            do {                                     \
                if (!_SupportAVX512) return SKIPPED; \
            } while (0)
#    elif COMPILE_SIMD_BITS == 256
#        define GUARDED_SIMD                       \
            do {                                   \
                if (!_SupportAVX2) return SKIPPED; \
            } while (0)
#    else
#        define GUARDED_SIMD ((void)0)
#    endif
#else
#    define GUARDED_SIMD ((void)0)
#endif


int SIMD_NAME_MODIFIER(test_cvt_u8_to_u16)(void) {
#if SSRJSON_AARCH
    return INVALID;
#elif SSRJSON_X86
#    if COMPILE_SIMD_BITS == 512
    GUARDED_SIMD;
    u8 input[32];
    u16 dst[32];
#    elif COMPILE_SIMD_BITS == 256
    GUARDED_SIMD;
    u8 input[16];
    u16 dst[16];
#    else
    u8 input[16];
    u16 dst[8];
#    endif
#endif
    usize loop_count = 16;
    for (usize _ = 0; _ < loop_count; _++) {
        //
        RANDOM_FILL(input);
        GARBAGE_FILL(dst);
//
#if COMPILE_SIMD_BITS == 512
        *(vector_u_u16_512 *)dst = cvt_u8_to_u16(*(vector_u_u8_256 *)input);
#elif COMPILE_SIMD_BITS == 256
        *(vector_u_u16_256 *)dst = cvt_u8_to_u16(*(vector_u_u8_128 *)input);
#else
        *(vector_u_u16_128 *)dst = cvt_u8_to_u16(*(vector_u_u8_128 *)input);
#endif
        for (usize i = 0; i < COUNT_OF(dst); i++) {
            CHECK(dst[i] == input[i]);
        }
    }
    return PASSED;
}

int SIMD_NAME_MODIFIER(test_cvt_u8_to_u32)(void) {
#if SSRJSON_AARCH
    return INVALID;
#elif SSRJSON_X86
#    if COMPILE_SIMD_BITS == 512
    GUARDED_SIMD;
    u8 input[16];
    u32 dst[16];
#    elif COMPILE_SIMD_BITS == 256
    GUARDED_SIMD;
    u8 input[16];
    u32 dst[8];
#    else
    u8 input[16];
    u32 dst[4];
#    endif
    usize loop_count = 16;
    for (usize _ = 0; _ < loop_count; _++) {
        //
        RANDOM_FILL(input);
        GARBAGE_FILL(dst);
        //
#    if COMPILE_SIMD_BITS == 512
        *(vector_u_u32_512 *)dst = cvt_u8_to_u32(*(vector_u_u8_128 *)input);
#    elif COMPILE_SIMD_BITS == 256
        *(vector_u_u32_256 *)dst = cvt_u8_to_u32(*(vector_u_u8_128 *)input);
#    else
        *(vector_u_u32_128 *)dst = cvt_u8_to_u32(*(vector_u_u8_128 *)input);
#    endif
        for (usize i = 0; i < COUNT_OF(dst); i++) {
            CHECK(dst[i] == input[i]);
        }
    }
    return PASSED;
#endif
}

int SIMD_NAME_MODIFIER(test_cvt_u16_to_u32)(void) {
#if SSRJSON_AARCH
    return INVALID;
#elif SSRJSON_X86

#    if COMPILE_SIMD_BITS == 512
    GUARDED_SIMD;
    u16 input[16];
    u32 dst[16];
#    elif COMPILE_SIMD_BITS == 256
    GUARDED_SIMD;
    u16 input[8];
    u32 dst[8];
#    else
    u16 input[8];
    u32 dst[4];
#    endif

    usize loop_count = 16;
    for (usize _ = 0; _ < loop_count; _++) {
        //
        RANDOM_FILL(input);
        GARBAGE_FILL(dst);
        //
#    if COMPILE_SIMD_BITS == 512
        *(vector_u_u32_512 *)dst = cvt_u16_to_u32(*(vector_u_u16_256 *)input);
#    elif COMPILE_SIMD_BITS == 256
        *(vector_u_u32_256 *)dst = cvt_u16_to_u32(*(vector_u_u16_128 *)input);
#    else
        *(vector_u_u32_128 *)dst = cvt_u16_to_u32(*(vector_u_u16_128 *)input);
#    endif
        for (usize i = 0; i < COUNT_OF(dst); i++) {
            CHECK(dst[i] == input[i]);
        }
    }
    return PASSED;
#endif
}

#if __SSSE3__
force_inline int _test_ucs2_encode_ssse3(void) {
    u16 input[8];
    u8 output[24];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_3bytes_u16();
    }
    ucs2_encode_3bytes_utf8_ssse3(output, (vector_a_u8_128) * (vector_u_u8_128 *)input);
    return check_ucs2_3bytes(input, output, COUNT_OF(input));
}
#endif

int SIMD_NAME_MODIFIER(test_ucs2_encode_3bytes_utf8)(void) {
#if SSRJSON_AARCH
    return INVALID;
#else
#    if __AVX512F__ && __AVX512CD__ && __AVX512BW__ && __AVX512VL__ && __AVX512DQ__
    GUARDED_SIMD;
    u16 input[32];
    u8 output[96];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_3bytes_u16();
    }
    ucs2_encode_3bytes_utf8_avx512(output, (vector_a_u16_512) * (vector_u_u16_512 *)input);
    return check_ucs2_3bytes(input, output, COUNT_OF(input));
#    elif __AVX2__
    GUARDED_SIMD;
    {
        u16 input[16];
        u8 output[48];
        for (int i = 0; i < COUNT_OF(input); ++i) {
            input[i] = get_random_3bytes_u16();
        }
        ucs2_encode_3bytes_utf8_avx2(output, (vector_a_u8_256) * (vector_u_u8_256 *)input);
        CHECK(check_ucs2_3bytes(input, output, COUNT_OF(input)));
    }
    {
        // also check ssse3 encoder
        return _test_ucs2_encode_ssse3();
    }
#    elif __SSSE3__
    return _test_ucs2_encode_ssse3();
#    else
    return INVALID;
#    endif
#endif
}

int SIMD_NAME_MODIFIER(test_ucs2_encode_2bytes_utf8)(void) {
#if SSRJSON_AARCH
    return INVALID;
#else
#    if __AVX512F__ && __AVX512CD__ && __AVX512BW__ && __AVX512VL__ && __AVX512DQ__
    GUARDED_SIMD;
    u16 input[32];
    u8 output[64];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_2bytes_u16();
    }
    ucs2_encode_2bytes_utf8_avx512(output, (vector_a_u16_512) * (vector_u_u16_512 *)input);
    return check_ucs2_2bytes(input, output, COUNT_OF(input));
#    elif __AVX2__
    GUARDED_SIMD;
    u16 input[16];
    u8 output[32];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_2bytes_u16();
    }
    ucs2_encode_2bytes_utf8_avx2(output, (vector_a_u8_256) * (vector_u_u8_256 *)input);
    return check_ucs2_2bytes(input, output, COUNT_OF(input));
#    else
    u16 input[8];
    u8 output[16];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_2bytes_u16();
    }
    ucs2_encode_2bytes_utf8_sse2(output, (vector_a_u8_128) * (vector_u_u8_128 *)input);
    return check_ucs2_2bytes(input, output, COUNT_OF(input));
#    endif
#endif
}

#if __SSSE3__
force_inline int _test_ucs4_encode_ssse3(void) {
    u32 input[4];
    u8 output[12];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_3bytes_u16();
    }
    ucs4_encode_3bytes_utf8_ssse3(output, (vector_a_u32_128) * (vector_u_u32_128 *)input);
    return check_ucs4_3bytes(input, output, COUNT_OF(input));
}
#endif

int SIMD_NAME_MODIFIER(test_ucs4_encode_3bytes_utf8)(void) {
#if SSRJSON_AARCH
    return INVALID;
#else
#    if __AVX512F__ && __AVX512CD__ && __AVX512BW__ && __AVX512VL__ && __AVX512DQ__
    GUARDED_SIMD;
    u32 input[16];
    u8 output[48];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_3bytes_u16();
    }
    ucs4_encode_3bytes_utf8_avx512(output, (vector_a_u32_512) * (vector_u_u32_512 *)input);
    return check_ucs4_3bytes(input, output, COUNT_OF(input));
#    elif __AVX2__
    GUARDED_SIMD;
    {
        u32 input[8];
        u8 output[24];
        for (int i = 0; i < COUNT_OF(input); ++i) {
            input[i] = get_random_3bytes_u16();
        }
        ucs4_encode_3bytes_utf8_avx2(output, (vector_a_u8_256) * (vector_u_u8_256 *)input);
        CHECK(check_ucs4_3bytes(input, output, COUNT_OF(input)));
    }
    return _test_ucs4_encode_ssse3();
#    elif __SSSE3__
    return _test_ucs4_encode_ssse3();
#    else
    return INVALID;
#    endif
#endif
}

int SIMD_NAME_MODIFIER(test_ucs4_encode_2bytes_utf8)(void) {
#if SSRJSON_AARCH
    return INVALID;
#else
#    if __AVX512F__ && __AVX512CD__ && __AVX512BW__ && __AVX512VL__ && __AVX512DQ__
    GUARDED_SIMD;
    u32 input[16];
    u8 output[32];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_2bytes_u16();
    }
    ucs4_encode_2bytes_utf8_avx512(output, (vector_a_u16_512) * (vector_u_u16_512 *)input);
    return check_ucs4_2bytes(input, output, COUNT_OF(input));
#    elif __AVX2__
    GUARDED_SIMD;
    u32 input[8];
    u8 output[16];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_2bytes_u16();
    }
    ucs4_encode_2bytes_utf8_avx2(output, (vector_a_u8_256) * (vector_u_u8_256 *)input);
    return check_ucs4_2bytes(input, output, COUNT_OF(input));
#    else
    u32 input[4];
    u8 output[8];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_2bytes_u16();
    }
    ucs4_encode_2bytes_utf8_sse2(output, (vector_a_u8_128) * (vector_u_u8_128 *)input);
    return check_ucs4_2bytes(input, output, COUNT_OF(input));
#    endif
#endif
}

int SIMD_NAME_MODIFIER(test_long_cvt_u8_u16)(void) {
    GUARDED_SIMD;
    for (usize _ = 0; _ < 10; _++) {
        static const usize buffer_len = (1 << 11);
        ssrjson_align(64) u8 buffer[buffer_len];
        ssrjson_align(64) u8 buffer_reference[buffer_len];
        GARBAGE_FILL(buffer);
        usize random_u8_start_index = (rand() % buffer_len) & (~(usize)1);
        usize out_u16_length = (usize)rand() % ((buffer_len - random_u8_start_index) / 2);
        // initialize random content
        fill_random_buffer(buffer + random_u8_start_index, out_u16_length);
        // find target func
        uintptr_t _func = find_extension_symbol(TEST_STRINGIZE(SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u16)));
        if (!_func) return FAILED;
        typedef void (*TestFuncType)(u16 *, u8 *, Py_ssize_t);
        TestFuncType func = (TestFuncType)_func;
        // backup memory for reference
        memcpy(buffer_reference, buffer, sizeof(buffer));
        // call target func
        func((u16 *)(buffer + random_u8_start_index), buffer + random_u8_start_index, out_u16_length);
        // check garbage content
        u8 *ref_start = buffer_reference + random_u8_start_index;
        u16 *start = (u16 *)(buffer + random_u8_start_index);
        for (usize i = 0; i < random_u8_start_index; ++i) {
            CHECK(buffer[i] == 0xfa);
            CHECK(buffer_reference[i] == 0xfa);
        }
        for (usize i = (random_u8_start_index / 2) + out_u16_length; i < buffer_len / 2; i++) {
            CHECK(((u16 *)buffer)[i] == 0xfafa);
        }
        for (usize i = random_u8_start_index + out_u16_length; i < buffer_len; ++i) {
            CHECK(buffer_reference[i] == 0xfa);
        }
        // check content
        for (usize i = 0; i < out_u16_length; ++i) {
            CHECK(ref_start[i] == start[i]);
        }
    }
    return PASSED;
}

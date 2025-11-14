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

#ifndef SSRJSON_H
#define SSRJSON_H


#include "ssrjson_config.h"

/*==============================================================================
 * Macros
 *============================================================================*/
/** compiler attribute check (since gcc 5.0, clang 2.9, icc 17) */
#ifndef ssrjson_has_attribute
#    ifdef __has_attribute
#        define ssrjson_has_attribute(x) __has_attribute(x)
#    else
#        define ssrjson_has_attribute(x) 0
#    endif
#endif


/** compiler builtin check (since gcc 10.0, clang 2.6, icc 2021) */
#ifndef ssrjson_has_builtin
#    ifdef __has_builtin
#        define ssrjson_has_builtin(x) __has_builtin(x)
#    else
#        define ssrjson_has_builtin(x) 0
#    endif
#endif

/** compiler version (GCC) */
#ifdef __GNUC__
#    define SSRJSON_GCC_VER __GNUC__
#    if defined(__GNUC_PATCHLEVEL__)
#        define ssrjson_gcc_available(major, minor, patch) \
            ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) >= (major * 10000 + minor * 100 + patch))
#    else
#        define ssrjson_gcc_available(major, minor, patch) \
            ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= (major * 10000 + minor * 100 + patch))
#    endif
#else
#    define SSRJSON_GCC_VER 0
#    define ssrjson_gcc_available(major, minor, patch) 0
#endif

/* gcc builtin */
#if ssrjson_has_builtin(__builtin_clz)
#    define GCC_HAS_CLZ 1
#else
#    define GCC_HAS_CLZ 0
#endif

#if ssrjson_has_builtin(__builtin_clzll) || ssrjson_gcc_available(3, 4, 0)
#    define GCC_HAS_CLZLL 1
#else
#    define GCC_HAS_CLZLL 0
#endif

#if ssrjson_has_builtin(__builtin_ctz)
#    define GCC_HAS_CTZ 1
#else
#    define GCC_HAS_CTZ 0
#endif

#if ssrjson_has_builtin(__builtin_ctzll) || ssrjson_gcc_available(3, 4, 0)
#    define GCC_HAS_CTZLL 1
#else
#    define GCC_HAS_CTZLL 0
#endif

/* export symbol */
#if defined(_WIN32) || defined(__CYGWIN__)
#    ifdef SSRJSON_EXPORTS
#        define SSRJSON_EXPORTED_SYMBOL __declspec(dllexport)
#    else
#        define SSRJSON_EXPORTED_SYMBOL __declspec(dllimport)
#    endif
#elif ssrjson_has_attribute(visibility)
#    define SSRJSON_EXPORTED_SYMBOL __attribute__((visibility("default")))
#endif

/** C version (STDC) */
#if defined(__STDC__) && (__STDC__ >= 1) && defined(__STDC_VERSION__)
#    define SSRJSON_STDC_VER __STDC_VERSION__
#else
#    define SSRJSON_STDC_VER 0
#endif

/* C++ version */
#if defined(__cplusplus)
#    define SSRJSON_CPP_VER __cplusplus
#else
#    define SSRJSON_CPP_VER 0
#endif

/** inline for compiler */
#ifndef ssrjson_inline
#    if defined(_MSC_VER) && _MSC_VER >= 1200
#        define ssrjson_inline __forceinline
#    elif defined(_MSC_VER)
#        define ssrjson_inline __inline
#    elif ssrjson_has_attribute(always_inline) || SSRJSON_GCC_VER >= 4
#        define ssrjson_inline __inline__ __attribute__((always_inline))
#    elif defined(__clang__) || defined(__GNUC__)
#        define ssrjson_inline __inline__
#    elif defined(__cplusplus) || SSRJSON_STDC_VER >= 199901L
#        define ssrjson_inline inline
#    else
#        define ssrjson_inline
#    endif
#endif

/** compiler version (MSVC) */
#ifdef _MSC_VER
#    define SSRJSON_MSC_VER _MSC_VER
#else
#    define SSRJSON_MSC_VER 0
#endif


/* msvc intrinsic */
#if SSRJSON_MSC_VER >= 1400
#    include <intrin.h>
#    if defined(_M_AMD64) || defined(_M_ARM64)
#        define MSC_HAS_BIT_SCAN_64 1
#        pragma intrinsic(_BitScanForward64)
#        pragma intrinsic(_BitScanReverse64)
#    else
#        define MSC_HAS_BIT_SCAN_64 0
#    endif
#    if defined(_M_AMD64) || defined(_M_ARM64) || \
            defined(_M_IX86) || defined(_M_ARM)
#        define MSC_HAS_BIT_SCAN 1
#        pragma intrinsic(_BitScanForward)
#        pragma intrinsic(_BitScanReverse)
#    else
#        define MSC_HAS_BIT_SCAN 0
#    endif
#    if defined(_M_AMD64)
#        define MSC_HAS_UMUL128 1
#        pragma intrinsic(_umul128)
#    else
#        define MSC_HAS_UMUL128 0
#    endif
#else
#    define MSC_HAS_BIT_SCAN_64 0
#    define MSC_HAS_BIT_SCAN 0
#    define MSC_HAS_UMUL128 0
#endif

/** noinline for compiler */
#ifndef ssrjson_noinline
#    if SSRJSON_MSC_VER >= 1400
#        define ssrjson_noinline __declspec(noinline)
#    elif ssrjson_has_attribute(noinline) || SSRJSON_GCC_VER >= 4
#        define ssrjson_noinline __attribute__((noinline))
#    else
#        define ssrjson_noinline
#    endif
#endif

/** real gcc check */
#if !defined(__clang__) && !defined(__INTEL_COMPILER) && !defined(__ICC) && \
        defined(__GNUC__)
#    define SSRJSON_IS_REAL_GCC 1
#else
#    define SSRJSON_IS_REAL_GCC 0
#endif

/** likely for compiler */
#ifndef ssrjson_likely
#    if ssrjson_has_builtin(__builtin_expect) || \
            (SSRJSON_GCC_VER >= 4 && SSRJSON_GCC_VER != 5)
#        define ssrjson_likely(expr) __builtin_expect(!!(expr), 1)
#    else
#        define ssrjson_likely(expr) (expr)
#    endif
#endif

/** unlikely for compiler */
#ifndef ssrjson_unlikely
#    if ssrjson_has_builtin(__builtin_expect) || \
            (SSRJSON_GCC_VER >= 4 && SSRJSON_GCC_VER != 5)
#        define ssrjson_unlikely(expr) __builtin_expect(!!(expr), 0)
#    else
#        define ssrjson_unlikely(expr) (expr)
#    endif
#endif

#define force_inline static ssrjson_inline
#define force_noinline ssrjson_noinline
/* For functions only used in one SIMD compile unit. */
#if DISABLE_INTERNAL_NOINLINE
#    define internal_simd_noinline static_assert(0, "internal_simd_noinline is disabled");
#else
#    define internal_simd_noinline static ssrjson_noinline
#endif
#define likely ssrjson_likely
#define unlikely ssrjson_unlikely

/* assume for compiler */
#ifdef NDEBUG
#    if defined(__GNUC__) || defined(__clang__)
#        define assume(cond)                          \
            do {                                      \
                if (!(cond)) __builtin_unreachable(); \
            } while (0)
#    elif defined(_MSC_VER)
#        define assume(cond) __assume(cond)
#    else
#        define assume(cond) ((void)0)
#    endif
#else
#    define assume(cond) assert(cond)
#endif

/* x86: check cpu features */
#if SSRJSON_X86
#    if defined(_MSC_VER)
#        define cpuid_count(info, leaf, count) __cpuidex(info, (leaf), (count))
#        define cpuid(info, x) __cpuid(info, (x))
#    else
#        include <cpuid.h>

force_inline void cpuid_count(int *info, int leaf, int count) {
    __cpuid_count(leaf, count, info[0], info[1], info[2], info[3]);
}

force_inline void cpuid(int *info, int x) {
    __cpuid(x, info[0], info[1], info[2], info[3]);
}
#    endif

force_inline int get_cpuid_max(void) {
    int info[4];
    cpuid(info, 0);
    return info[0];
}
#endif

/** repeat utils */
#define REPEAT_2(x) x, x,
#define REPEAT_4(x) REPEAT_2(x) REPEAT_2(x)
#define REPEAT_8(x) REPEAT_4(x) REPEAT_4(x)
#define REPEAT_16(x) REPEAT_8(x) REPEAT_8(x)
#define REPEAT_32(x) REPEAT_16(x) REPEAT_16(x)
#define REPEAT_64(x) REPEAT_32(x) REPEAT_32(x)

#if defined(SSRJSON_COVERAGE) || defined(NDEBUG)
#    define SSRJSON_UNREACHABLE() __builtin_unreachable()
#else
#    define SSRJSON_UNREACHABLE() assert(false)
#endif

#define SSRJSON_CAST(type, expr) ((type)(expr))
#define SSRJSON_PYASCII_CAST(expr) SSRJSON_CAST(PyASCIIObject *, (expr))
#define SSRJSON_PYCOMPACTUNICODE_CAST(expr) SSRJSON_CAST(PyCompactUnicodeObject *, (expr))
#define SSRJSON_PYUNICODE_CAST(expr) SSRJSON_CAST(PyUnicodeObject *, (expr))
/*==============================================================================
 * Macros
 *============================================================================*/

/* Macros used for loop unrolling and other purpose. */
// #define repeat2(x)  { x x }
// #define repeat3(x)  { x x x }
#define REPEAT_CALL_4(x) {x x x x}
// #define repeat8(x)  { x x x x x x x x }
#define REPEAT_CALL_16(x) {x x x x x x x x x x x x x x x x}

#define COUNT_OF(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

// #define repeat2_incr(x)   { x(0)  x(1) }
// #define repeat4_incr(x)   { x(0)  x(1)  x(2)  x(3) }
// #define repeat8_incr(x)   { x(0)  x(1)  x(2)  x(3)  x(4)  x(5)  x(6)  x(7)  }
#define REPEAT_INCR_16(x) {x(0) x(1) x(2) x(3) x(4) x(5) x(6) x(7) \
                                   x(8) x(9) x(10) x(11) x(12) x(13) x(14) x(15)}

#define REPEAT_INCR_IN_1_18(x) {x(1) x(2) x(3) x(4) x(5) x(6) x(7) x(8)                \
                                        x(9) x(10) x(11) x(12) x(13) x(14) x(15) x(16) \
                                                x(17) x(18)}


/** align for compiler */
#ifndef ssrjson_align
#    if defined(_MSC_VER) && _MSC_VER >= 1300
#        define ssrjson_align(x) __declspec(align(x))
#    elif ssrjson_has_attribute(aligned) || defined(__GNUC__)
#        define ssrjson_align(x) __attribute__((aligned(x)))
#    elif SSRJSON_CPP_VER >= 201103L
#        define ssrjson_align(x) alignas(x)
#    else
#        define ssrjson_align(x)
#    endif
#endif


/* Concat macros */
#define SSRJSON_CONCAT2_EX(a, b) a##_##b
#define SSRJSON_CONCAT2(a, b) SSRJSON_CONCAT2_EX(a, b)

#define SSRJSON_CONCAT3_EX(a, b, c) a##_##b##_##c
#define SSRJSON_CONCAT3(a, b, c) SSRJSON_CONCAT3_EX(a, b, c)

#define SSRJSON_CONCAT4_EX(a, b, c, d) a##_##b##_##c##_##d
#define SSRJSON_CONCAT4(a, b, c, d) SSRJSON_CONCAT4_EX(a, b, c, d)

#define SSRJSON_CONCAT5_EX(a, b, c, d, e) a##_##b##_##c##_##d##_##e
#define SSRJSON_CONCAT5(a, b, c, d, e) SSRJSON_CONCAT5_EX(a, b, c, d, e)

#define SSRJSON_SIMPLE_CONCAT2_EX(a, b) a##b
#define SSRJSON_SIMPLE_CONCAT2(a, b) SSRJSON_SIMPLE_CONCAT2_EX(a, b)

#define SSRJSON_SIMPLE_CONCAT3_EX(a, b, c) a##b##c
#define SSRJSON_SIMPLE_CONCAT3(a, b, c) SSRJSON_SIMPLE_CONCAT3_EX(a, b, c)

#define SSRJSON_SIMPLE_CONCAT4_EX(a, b, c, d) a##b##c##d
#define SSRJSON_SIMPLE_CONCAT4(a, b, c, d) SSRJSON_SIMPLE_CONCAT4_EX(a, b, c, d)

#define SSRJSON_SIMPLE_CONCAT5_EX(a, b, c, d, e) a##b##c##d##e
#define SSRJSON_SIMPLE_CONCAT5(a, b, c, d, e) SSRJSON_SIMPLE_CONCAT3_EX(a, b, c, d, e)

#define SSRJSON_MAX(x, y) ((x) > (y) ? (x) : (y))
#define SSRJSON_MIN(x, y) ((x) < (y) ? (x) : (y))

#ifdef _MSC_VER
#    define SSRJSON_ALIGNED_ALLOC(_align, _size) _aligned_malloc(_size, _align)
#    define SSRJSON_ALIGNED_FREE(_ptr) _aligned_free(_ptr)
#else
#    define SSRJSON_ALIGNED_ALLOC(_align, _size) aligned_alloc(_align, _size)
#    define SSRJSON_ALIGNED_FREE(_ptr) free(_ptr)
#endif

/* String type macros */
#define SSRJSON_STRING_TYPE_ASCII 0
#define SSRJSON_STRING_TYPE_LATIN1 1
#define SSRJSON_STRING_TYPE_UCS2 2
#define SSRJSON_STRING_TYPE_UCS4 4

#ifndef SSRJSON_HAS_IEEE_754
/* IEEE 754 floating-point binary representation */
#    if defined(DOUBLE_IS_LITTLE_ENDIAN_IEEE754) || defined(DOUBLE_IS_BIG_ENDIAN_IEEE754) || defined(DOUBLE_IS_ARM_MIXED_ENDIAN_IEEE754) || _PY_SHORT_FLOAT_REPR == 1
#        define SSRJSON_HAS_IEEE_754 1
#    elif (FLT_RADIX == 2) && (DBL_MANT_DIG == 53) && (DBL_DIG == 15) && \
            (DBL_MIN_EXP == -1021) && (DBL_MAX_EXP == 1024) &&           \
            (DBL_MIN_10_EXP == -307) && (DBL_MAX_10_EXP == 308)
#        define SSRJSON_HAS_IEEE_754 1
#    else
#        define SSRJSON_HAS_IEEE_754 0
#    endif
#endif

static_assert(SSRJSON_HAS_IEEE_754, "Current platform does not support IEEE 754");

/* int128 type */
#if defined(__SIZEOF_INT128__) && (__SIZEOF_INT128__ == 16) && \
        (defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER))
#    define SSRJSON_HAS_INT128 1
/** 128-bit integer, used by floating-point number reader and writer. */
__extension__ typedef __int128 i128;
__extension__ typedef unsigned __int128 u128;
#else
#    define SSRJSON_HAS_INT128 0
#endif


/* Helper for quickly write an err handle. */
#define RETURN_ON_UNLIKELY_ERR(x) \
    do {                          \
        if (unlikely((x))) {      \
            return false;         \
        }                         \
    } while (0)

/* Some constants. */
#define _Quote (34)
#define _Slash (92)
#define _MinusOne (-1)
#define ControlMax (32)

/* Default padding. */
#define TAIL_PADDING (512 / 8)

/* Tool macros for calculating how long the buffer should be reserved to. */
#define max_json_bytes_per_unicode (6) // per unicode can be JSON encoded to at most 6 bytes
#define max_utf8_bytes_per_ucs1 (2)    // per UCS1 can be UTF-8 encoded to at most 2 bytes
#define max_utf8_bytes_per_ucs2 (3)    // per UCS2 can be UTF-8 encoded to at most 3 bytes
#define max_utf8_bytes_per_ucs4 (4)    // per UCS4 can be UTF-8 encoded to at most 4 bytes

/*==============================================================================
 * 128-bit Integer Utils
 * These functions are used by the floating-point number reader and writer.
 *============================================================================*/

/** Multiplies two 64-bit unsigned integers (a * b),
    returns the 128-bit result as 'hi' and 'lo'. */
force_inline void u128_mul(u64 a, u64 b, u64 *hi, u64 *lo) {
#if SSRJSON_HAS_INT128
    u128 m = (u128)a * b;
    *hi = (u64)(m >> 64);
    *lo = (u64)(m);
#elif MSC_HAS_UMUL128
    *lo = _umul128(a, b, hi);
#else
    u32 a0 = (u32)(a), a1 = (u32)(a >> 32);
    u32 b0 = (u32)(b), b1 = (u32)(b >> 32);
    u64 p00 = (u64)a0 * b0, p01 = (u64)a0 * b1;
    u64 p10 = (u64)a1 * b0, p11 = (u64)a1 * b1;
    u64 m0 = p01 + (p00 >> 32);
    u32 m00 = (u32)(m0), m01 = (u32)(m0 >> 32);
    u64 m1 = p10 + m00;
    u32 m10 = (u32)(m1), m11 = (u32)(m1 >> 32);
    *hi = p11 + m01 + m11;
    *lo = ((u64)m10 << 32) | (u32)p00;
#endif
}

/** Multiplies two 64-bit unsigned integers and add a value (a * b + c),
    returns the 128-bit result as 'hi' and 'lo'. */
force_inline void u128_mul_add(u64 a, u64 b, u64 c, u64 *hi, u64 *lo) {
#if SSRJSON_HAS_INT128
    u128 m = (u128)a * b + c;
    *hi = (u64)(m >> 64);
    *lo = (u64)(m);
#else
    u64 h, l, t;
    u128_mul(a, b, &h, &l);
    t = l + c;
    h += (u64)(((t < l) | (t < c)));
    *hi = h;
    *lo = t;
#endif
}

/* Used to write u64 literal for C89 which doesn't support "ULL" suffix. */
#undef U64
#define U64(hi, lo) ((((u64)hi##UL) << 32U) + lo##UL)

#define U8MAX (255)

/*==============================================================================
 * Power10 Lookup Table
 * These data are used by the floating-point number reader and writer.
 *============================================================================*/

/** Minimum decimal exponent in pow10_sig_table. */
#define POW10_SIG_TABLE_MIN_EXP -343

/** Maximum decimal exponent in pow10_sig_table. */
#define POW10_SIG_TABLE_MAX_EXP 324

/** Minimum exact decimal exponent in pow10_sig_table */
#define POW10_SIG_TABLE_MIN_EXACT_EXP 0

/** Maximum exact decimal exponent in pow10_sig_table */
#define POW10_SIG_TABLE_MAX_EXACT_EXP 55

/** Normalized significant 128 bits of pow10, no rounded up (size: 10.4KB).
    This lookup table is used by both the double number reader and writer.
    (generate with misc/make_tables.c) */
extern const u64 pow10_sig_table[];

/**
 Get the cached pow10 value from pow10_sig_table.
 @param exp10 The exponent of pow(10, e). This value must in range
              POW10_SIG_TABLE_MIN_EXP to POW10_SIG_TABLE_MAX_EXP.
 @param hi    The highest 64 bits of pow(10, e).
 @param lo    The lower 64 bits after `hi`.
 */
force_inline void pow10_table_get_sig(i32 exp10, u64 *hi, u64 *lo) {
    i32 idx = exp10 - (POW10_SIG_TABLE_MIN_EXP);
    *hi = pow10_sig_table[idx * 2];
    *lo = pow10_sig_table[idx * 2 + 1];
}

/**
 Get the exponent (base 2) for highest 64 bits significand in pow10_sig_table.
 */
force_inline void pow10_table_get_exp(i32 exp10, i32 *exp2) {
    /* e2 = floor(log2(pow(10, e))) - 64 + 1 */
    /*    = floor(e * log2(10) - 63)         */
    *exp2 = (exp10 * 217706 - 4128768) >> 16;
}

/*==============================================================================
 * Digit Character Matcher
 *============================================================================*/

/** Digit type */
typedef u8 digi_type;

/** Digit: '0'. */
static const digi_type DIGI_TYPE_ZERO = 1 << 0;

/** Digit: [1-9]. */
static const digi_type DIGI_TYPE_NONZERO = 1 << 1;

/** Plus sign (positive): '+'. */
static const digi_type DIGI_TYPE_POS = 1 << 2;

/** Minus sign (negative): '-'. */
static const digi_type DIGI_TYPE_NEG = 1 << 3;

/** Decimal point: '.' */
static const digi_type DIGI_TYPE_DOT = 1 << 4;

/** Exponent sign: 'e, 'E'. */
static const digi_type DIGI_TYPE_EXP = 1 << 5;


/** Whitespace character: ' ', '\\t', '\\n', '\\r'. */
static const u8 CHAR_TYPE_SPACE = 1 << 0;

/** Number character: '-', [0-9]. */
static const u8 CHAR_TYPE_NUMBER = 1 << 1;

/** JSON Escaped character: '"', '\', [0x00-0x1F]. */
static const u8 CHAR_TYPE_ESC_ASCII = 1 << 2;

/** Non-ASCII character: [0x80-0xFF]. */
static const u8 CHAR_TYPE_NON_ASCII = 1 << 3;

/** JSON container character: '{', '['. */
static const u8 CHAR_TYPE_CONTAINER = 1 << 4;

/** Comment character: '/'. */
static const u8 CHAR_TYPE_COMMENT = 1 << 5;

/** Line end character: '\\n', '\\r', '\0'. */
static const u8 CHAR_TYPE_LINE_END = 1 << 6;

/** Hexadecimal numeric character: [0-9a-fA-F]. */
static const u8 CHAR_TYPE_HEX = 1 << 7;

/** Digit type table (generate with misc/make_tables.c) */
extern const u8 digi_table[256];

/** Match a character with specified type. */
force_inline bool digi_is_type(u8 d, u8 type) {
    return (digi_table[d] & type) != 0;
}

/** Match a sign: '+', '-' */
force_inline bool _digi_is_sign(u8 d) {
    return digi_is_type(d, (u8)(DIGI_TYPE_POS | DIGI_TYPE_NEG));
}

/** Match a none zero digit: [1-9] */
force_inline bool _digi_is_nonzero(u8 d) {
    return digi_is_type(d, (u8)DIGI_TYPE_NONZERO);
}

/** Match a digit: [0-9] */
force_inline bool _digi_is_digit(u8 d) {
    return digi_is_type(d, (u8)(DIGI_TYPE_ZERO | DIGI_TYPE_NONZERO));
}

/** Match an exponent sign: 'e', 'E'. */
force_inline bool _digi_is_exp(u8 d) {
    return digi_is_type(d, (u8)DIGI_TYPE_EXP);
}

/** Match a floating point indicator: '.', 'e', 'E'. */
force_inline bool _digi_is_fp(u8 d) {
    return digi_is_type(d, (u8)(DIGI_TYPE_DOT | DIGI_TYPE_EXP));
}

/** Match a digit or floating point indicator: [0-9], '.', 'e', 'E'. */
force_inline bool _digi_is_digit_or_fp(u8 d) {
    return digi_is_type(d, (u8)(DIGI_TYPE_ZERO | DIGI_TYPE_NONZERO |
                                DIGI_TYPE_DOT | DIGI_TYPE_EXP));
}

/** Returns the number of leading 0-bits in value (input should not be 0). */
force_inline u32 u32_lz_bits(u32 v) {
    assert(v);
#if GCC_HAS_CLZ
    return (u32)__builtin_clz(v);
#elif MSC_HAS_BIT_SCAN
    unsigned long r;
    _BitScanReverse(&r, v);
    return (u32)31 - (u32)r;
#else
    /*
     branchless, use de Bruijn sequences
     see: https://www.chessprogramming.org/BitScan
     */
    const u8 table[64] = {
            63, 16, 62, 7, 15, 36, 61, 3, 6, 14, 22, 26, 35, 47, 60, 2,
            9, 5, 28, 11, 13, 21, 42, 19, 25, 31, 34, 40, 46, 52, 59, 1,
            17, 8, 37, 4, 23, 27, 48, 10, 29, 12, 43, 20, 32, 41, 53, 18,
            38, 24, 49, 30, 44, 33, 54, 39, 50, 45, 55, 51, 56, 57, 58, 0};
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return table[(v * U64(0x03F79D71, 0xB4CB0A89)) >> 58];
#endif
}

/** Returns the number of leading 0-bits in value (input should not be 0). */
force_inline u32 u64_lz_bits(u64 v) {
    assert(v);
#if GCC_HAS_CLZLL
    return (u32)__builtin_clzll(v);
#elif MSC_HAS_BIT_SCAN_64
    unsigned long r;
    _BitScanReverse64(&r, v);
    return (u32)63 - (u32)r;
#elif MSC_HAS_BIT_SCAN
    unsigned long hi, lo;
    bool hi_set = _BitScanReverse(&hi, (u32)(v >> 32)) != 0;
    _BitScanReverse(&lo, (u32)v);
    hi |= 32;
    return (u32)63 - (u32)(hi_set ? hi : lo);
#else
    /*
     branchless, use de Bruijn sequences
     see: https://www.chessprogramming.org/BitScan
     */
    const u8 table[64] = {
            63, 16, 62, 7, 15, 36, 61, 3, 6, 14, 22, 26, 35, 47, 60, 2,
            9, 5, 28, 11, 13, 21, 42, 19, 25, 31, 34, 40, 46, 52, 59, 1,
            17, 8, 37, 4, 23, 27, 48, 10, 29, 12, 43, 20, 32, 41, 53, 18,
            38, 24, 49, 30, 44, 33, 54, 39, 50, 45, 55, 51, 56, 57, 58, 0};
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    return table[(v * U64(0x03F79D71, 0xB4CB0A89)) >> 58];
#endif
}

/** Returns the number of trailing 0-bits in value (input should not be 0). */
force_inline u32 u32_tz_bits(u32 v) {
    assert(v);
#if GCC_HAS_CTZ
    return (u32)__builtin_ctz(v);
#elif MSC_HAS_BIT_SCAN
    unsigned long r;
    _BitScanForward(&r, v);
    return (u32)r;
#else
    /*
     branchless, use de Bruijn sequences
     see: https://www.chessprogramming.org/BitScan
     */
    const u8 table[64] = {
            0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28,
            62, 5, 39, 46, 44, 42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11,
            63, 52, 6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
            51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12};
    return table[(((u64)v & (~(u64)v + 1)) * U64(0x022FDD63, 0xCC95386D)) >> 58];
#endif
}

/** Returns the number of trailing 0-bits in value (input should not be 0). */
force_inline u32 u64_tz_bits(u64 v) {
    // assert(v);
#if GCC_HAS_CTZLL
    return (u32)__builtin_ctzll(v);
#elif MSC_HAS_BIT_SCAN_64
    unsigned long r;
    _BitScanForward64(&r, v);
    return (u32)r;
#elif MSC_HAS_BIT_SCAN
    unsigned long lo, hi;
    bool lo_set = _BitScanForward(&lo, (u32)(v)) != 0;
    _BitScanForward(&hi, (u32)(v >> 32));
    hi += 32;
    return lo_set ? lo : hi;
#else
    /*
     branchless, use de Bruijn sequences
     see: https://www.chessprogramming.org/BitScan
     */
    const u8 table[64] = {
            0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28,
            62, 5, 39, 46, 44, 42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11,
            63, 52, 6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
            51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12};
    return table[((v & (~v + 1)) * U64(0x022FDD63, 0xCC95386D)) >> 58];
#endif
}

/*==============================================================================
 * Utils
 *============================================================================*/

/** Returns whether the size is power of 2 (size should not be 0). */
force_inline bool size_is_pow2(usize size) {
    return (size & (size - 1)) == 0;
}

/** Align size upwards (may overflow). */
force_inline usize size_align_up(usize size, usize align) {
    if (size_is_pow2(align)) {
        return (size + (align - 1)) & ~(align - 1);
    } else {
        return size + align - (size + align - 1) % align - 1;
    }
}

/*
 * Split tail length into multi parts.
 */
force_inline void split_tail_len_two_parts(usize tail_len, usize check_count, usize *restrict part1, usize *restrict part2) {
    assert(tail_len > 0 && tail_len < check_count);
    assert(check_count / 2 * 2 == check_count);
    const usize check_half = check_count / 2;
    usize p1, p2;
    p2 = tail_len > check_half ? check_half : tail_len;
    p1 = tail_len - p2;
    assert(p1 >= 0 && p2 >= 0);
    assert(p1 <= check_half && p2 <= check_half);
    assert(p1 + p2 == tail_len);
    *part2 = p2;
    *part1 = p1;
}

/* Get tail length at specific part.*/
force_inline usize get_tail_len_parts_by_index(usize tail_len, usize batch_count, usize parts, usize index) {
    usize small_batch = batch_count / parts;
    assert(batch_count == small_batch * parts);
    usize ret = SSRJSON_MIN(tail_len, (index + 1) * small_batch);
    ret = SSRJSON_MAX(ret, index * small_batch);
    ret -= index * small_batch;
    return ret;
}

/* typedefs */
typedef struct {
    PyObject *key;
    u64 key_hint;
} decode_cache_t;


#endif // SSRJSON_H

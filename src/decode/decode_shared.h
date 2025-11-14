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

#ifndef SSRJSON_DECODE_H
#define SSRJSON_DECODE_H
#include "pyutils.h"
#include "simd/memcmp.h"
#include "simd/simd_impl.h"
#include "ssrjson.h"
#include "xxhash.h"


typedef pyobj_ptr_t *decode_obj_stack_ptr_t;

typedef struct DecodeObjStackInfo {
    decode_obj_stack_ptr_t decode_obj_writer;
    decode_obj_stack_ptr_t decode_obj_stack;
    decode_obj_stack_ptr_t decode_obj_stack_end;
} DecodeObjStackInfo;

typedef struct DecodeCtnWithSize {
    Py_ssize_t raw;
} DecodeCtnWithSize;


#if !defined(Py_GIL_DISABLED)
extern DecodeCtnWithSize _DecodeCtnBuffer[SSRJSON_DECODE_MAX_RECURSION];
extern u8 _DecodeTempBuffer[SSRJSON_STRING_BUFFER_SIZE];

force_inline DecodeCtnWithSize *get_decode_ctn_stack_buffer(void) {
    return _DecodeCtnBuffer;
}

extern PyObject *_DecodeObjBuffer[SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE];

force_inline decode_obj_stack_ptr_t get_decode_obj_stack_buffer(void) {
    return _DecodeObjBuffer;
}
#endif

// typedef struct DecodeCtnStackInfo {
//     DecodeCtnWithSize *ctn;
//     DecodeCtnWithSize *ctn_start;
//     DecodeCtnWithSize *ctn_end;
// } DecodeCtnStackInfo;

typedef union {
    struct {
        u32 escape_val;
        u32 escape_size;
    };

    u64 union_value;
} EscapeInfo;

#define _DECODE_UNICODE_ERR SSRJSON_CAST(u32, -1)

#define DECODE_LOOPSTATE_CONTINUE 0
#define DECODE_LOOPSTATE_END 1
#define DECODE_LOOPSTATE_ESCAPE 2
#define DECODE_LOOPSTATE_INVALID 3

extern PyObject *JSONDecodeError;

typedef enum ReadStrScanFlag {
    StrContinue,
    StrInvalid,
    StrEnd,
} ReadStrScanFlag;

typedef struct ReadStrState {
    ReadStrScanFlag scan_flag;
    int max_char_type;
    bool need_copy;
    bool dont_check_max_char;
    bool state_dirty;
} ReadStrState;

typedef struct SpecialCharReadResult {
    u32 value;
    ReadStrScanFlag flag;
} SpecialCharReadResult;

/*==============================================================================
 * Integer Constants
 *============================================================================*/

// /* Used to write u64 literal for C89 which doesn't support "ULL" suffix. */
// #undef U64
// #define U64(hi, lo) ((((u64)hi##UL) << 32U) + lo##UL)

/* U64 constant values */
#undef U64_MAX
#define U64_MAX U64(0xFFFFFFFF, 0xFFFFFFFF)
#undef I64_MAX
#define I64_MAX U64(0x7FFFFFFF, 0xFFFFFFFF)
#undef USIZE_MAX
#define USIZE_MAX ((usize)(~(usize)0))

/* Maximum number of digits for reading u32/u64/usize safety (not overflow). */
#undef U32_SAFE_DIG
#define U32_SAFE_DIG 9 /* u32 max is 4294967295, 10 digits */
#undef U64_SAFE_DIG
#define U64_SAFE_DIG 19 /* u64 max is 18446744073709551615, 20 digits */
#undef USIZE_SAFE_DIG
#define USIZE_SAFE_DIG (sizeof(usize) == 8 ? U64_SAFE_DIG : U32_SAFE_DIG)


/*==============================================================================
 * IEEE-754 Double Number Constants
 *============================================================================*/

/* Inf raw value (positive) */
#define F64_RAW_INF U64(0x7FF00000, 0x00000000)

/* NaN raw value (quiet NaN, no payload, no sign) */
#if defined(__hppa__) || (defined(__mips__) && !defined(__mips_nan2008))
#    define F64_RAW_NAN U64(0x7FF7FFFF, 0xFFFFFFFF)
#else
#    define F64_RAW_NAN U64(0x7FF80000, 0x00000000)
#endif

/* double number bits */
#define F64_BITS 64

/* double number exponent part bits */
#define F64_EXP_BITS 11

/* double number significand part bits */
#define F64_SIG_BITS 52

/* double number significand part bits (with 1 hidden bit) */
#define F64_SIG_FULL_BITS 53

/* double number significand bit mask */
#define F64_SIG_MASK U64(0x000FFFFF, 0xFFFFFFFF)

/* double number exponent bit mask */
#define F64_EXP_MASK U64(0x7FF00000, 0x00000000)

/* double number exponent bias */
#define F64_EXP_BIAS 1023

/* double number significant digits count in decimal */
// #define F64_DEC_DIG 17

/* max significant digits count in decimal when reading double number */
#define F64_MAX_DEC_DIG 768

/* maximum decimal power of double number (1.7976931348623157e308) */
#define F64_MAX_DEC_EXP 308

/* minimum decimal power of double number (4.9406564584124654e-324) */
#define F64_MIN_DEC_EXP (-324)

/* maximum binary power of double number */
#define F64_MAX_BIN_EXP 1024

/* minimum binary power of double number */
#define F64_MIN_BIN_EXP (-1021)

/*==============================================================================
 * Hex Character Reader
 * This function is used by JSON reader to read escaped characters.
 *============================================================================*/


/**
 Scans an escaped character sequence as a UTF-16 code unit (branchless).
 e.g. "\\u005C" should pass "005C" as `cur`.
 
 This requires the string has 4-byte zero padding.
 */
// force_inline bool read_8_to_hex_u16(const u8 *cur, u16 *val);

force_inline bool byte_match_2(const void *buf, const void *pat) {
    u16 u1, u2;
    memcpy(&u1, buf, 2);
    memcpy(&u2, pat, 2);
    return u1 == u2;
}

// force_inline bool byte_match_4(const void *buf, const void *pat) {
//     u32 u1, u2;
//     memcpy(&u1, buf, 4);
//     memcpy(&u2, pat, 4);
//     return u1 == u2;
// }

force_inline void byte_move_2(void *dst, const void *src) {
    u16 tmp;
    memcpy(&tmp, src, 2);
    memcpy(dst, &tmp, 2);
}

force_inline void byte_move_4(void *dst, const void *src) {
    u32 tmp;
    memcpy(&tmp, src, 4);
    memcpy(dst, &tmp, 4);
}

force_inline void byte_move_8(void *dst, const void *src) {
    u64 tmp;
    memcpy(&tmp, src, 8);
    memcpy(dst, &tmp, 8);
}

// force_inline void byte_move_16(void *dst, const void *src) {
// char *pdst = (char *)dst;
// const char *psrc = (const char *)src;
// u64 tmp1, tmp2;
// memcpy(&tmp1, psrc, 8);
// memcpy(&tmp2, psrc + 8, 8);
// memcpy(pdst, &tmp1, 8);
// memcpy(pdst + 8, &tmp2, 8);
// }

force_inline u32 byte_load_4(const void *src) {
    u32 u;
    memcpy(&u, src, 4);
    return u;
}

/*==============================================================================
 * JSON Reader Utils
 * These functions are used by JSON reader to read literals and comments.
 *============================================================================*/

force_inline bool decode_nan(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                             decode_obj_stack_ptr_t *decode_obj_stack_addr,
                             decode_obj_stack_ptr_t *decode_obj_stack_end_addr, bool is_signed);

extern const u8 char_table[256];

/** Match a character with specified type. */
force_inline bool char_is_type(u8 c, u8 type) {
    return (char_table[c] & type) != 0;
}

/** Match a whitespace: ' ', '\\t', '\\n', '\\r'. */
force_inline bool char_is_space(u8 c) {
    return char_is_type(c, (u8)CHAR_TYPE_SPACE);
}

/** Match a whitespace or comment: ' ', '\\t', '\\n', '\\r', '/'. */
force_inline bool char_is_space_or_comment(u8 c) {
    return char_is_type(c, (u8)(CHAR_TYPE_SPACE | CHAR_TYPE_COMMENT));
}

/** Match a JSON number: '-', [0-9]. */
force_inline bool char_is_number(u8 c) {
    return char_is_type(c, (u8)CHAR_TYPE_NUMBER);
}

/** Match a JSON container: '{', '['. */
force_inline bool char_is_container(u8 c) {
    return char_is_type(c, (u8)CHAR_TYPE_CONTAINER);
}

/** Match a stop character in ASCII string: '"', '\', [0x00-0x1F,0x80-0xFF]. */
force_inline bool char_is_ascii_stop(u8 c) {
    return char_is_type(c, (u8)(CHAR_TYPE_ESC_ASCII |
                                CHAR_TYPE_NON_ASCII));
}

force_inline u16 read_b2_unicode(u32 uni) {
#if PY_BIG_ENDIAN
    return ((uni & 0x1f000000) >> 18) | ((uni & 0x3f0000) >> 16);
#else
    return ((uni & 0x1f) << 6) | ((uni & 0x3f00) >> 8);
#endif
}

force_inline u16 read_b3_unicode(u32 uni) {
#if PY_BIG_ENDIAN
    return ((uni & 0x0f000000) >> 12) | ((uni & 0x3f0000) >> 10) | ((uni & 0x3f00) >> 8);
#else
    return ((uni & 0x0f) << 12) | ((uni & 0x3f00) >> 2) | ((uni & 0x3f0000) >> 16);
#endif
}

force_inline u32 read_b4_unicode(u32 uni) {
#if PY_BIG_ENDIAN
    return ((uni & 0x07000000) >> 6) | ((uni & 0x3f0000) >> 4) | ((uni & 0x3f00) >> 2) | ((uni & 0x3f));
#else
    return ((uni & 0x07) << 18) | ((uni & 0x3f00) << 4) | ((uni & 0x3f0000) >> 10) | ((uni & 0x3f000000) >> 24);
#endif
}

force_inline bool init_decode_obj_stack_info(
        // DecodeObjStackInfo *restrict decode_obj_stack_info
        decode_obj_stack_ptr_t *decode_obj_writer_addr,
        decode_obj_stack_ptr_t *decode_obj_stack_addr,
        decode_obj_stack_ptr_t *decode_obj_stack_end_addr) {
    pyobj_ptr_t *new_buffer = get_decode_obj_stack_buffer();
    if (unlikely(!new_buffer)) {
        PyErr_NoMemory();
        return false;
    }
    *decode_obj_stack_addr = new_buffer;
    *decode_obj_writer_addr = new_buffer;
    *decode_obj_stack_end_addr = new_buffer + SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE;
    return true;
}

force_inline bool init_decode_ctn_stack_info(DecodeCtnWithSize **ctn_start_addr, DecodeCtnWithSize **ctn_addr, DecodeCtnWithSize **ctn_end_addr) {
    DecodeCtnWithSize *new_buffer = get_decode_ctn_stack_buffer();
    if (unlikely(!new_buffer)) {
        PyErr_NoMemory();
        return false;
    }
    *ctn_start_addr = new_buffer;
    *ctn_addr = new_buffer;
    *ctn_end_addr = new_buffer + SSRJSON_DECODE_MAX_RECURSION;
    return true;
}

force_inline bool decode_ctn_is_arr(DecodeCtnWithSize *ctn) {
    return ctn->raw < 0;
}

force_inline Py_ssize_t get_decode_ctn_len(DecodeCtnWithSize *ctn) {
    return ctn->raw & PY_SSIZE_T_MAX;
}

force_inline void set_decode_ctn(DecodeCtnWithSize *ctn, Py_ssize_t len, bool is_arr) {
    assert(len >= 0);
    ctn->raw = len | (is_arr ? PY_SSIZE_T_MIN : 0);
}

force_inline void incr_decode_ctn_size(DecodeCtnWithSize *ctn) {
    assert(ctn->raw != PY_SSIZE_T_MAX);
    ctn->raw++;
}

force_inline bool ctn_grow_check(DecodeCtnWithSize **ctn_addr, DecodeCtnWithSize *ctn_end) {
    return ++(*ctn_addr) < ctn_end;
}

force_inline PyObject *make_string(const u8 *unicode_str, Py_ssize_t len, int type_flag, bool is_key);

force_inline bool push_obj(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                           decode_obj_stack_ptr_t *decode_obj_stack_addr,
                           decode_obj_stack_ptr_t *decode_obj_stack_end_addr, pyobj_ptr_t obj);

force_inline PyObject *read_bytes(const u8 **ptr, u8 *write_buffer, bool is_key);

force_inline bool decode_true(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                              decode_obj_stack_ptr_t *decode_obj_stack_addr,
                              decode_obj_stack_ptr_t *decode_obj_stack_end_addr);

force_inline bool decode_false(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                               decode_obj_stack_ptr_t *decode_obj_stack_addr,
                               decode_obj_stack_ptr_t *decode_obj_stack_end_addr);

force_inline bool decode_null(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                              decode_obj_stack_ptr_t *decode_obj_stack_addr,
                              decode_obj_stack_ptr_t *decode_obj_stack_end_addr);

force_inline bool decode_arr(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                             decode_obj_stack_ptr_t *decode_obj_stack_addr,
                             decode_obj_stack_ptr_t *decode_obj_stack_end_addr, usize arr_len);

force_inline bool decode_obj(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                             decode_obj_stack_ptr_t *decode_obj_stack_addr,
                             decode_obj_stack_ptr_t *decode_obj_stack_end_addr, usize dict_len);


#if PY_MINOR_VERSION >= 12
#    define SSRJSON_PY_DECREF_DEBUG() (_Py_DECREF_STAT_INC())
#    define SSRJSON_PY_INCREF_DEBUG() (_Py_INCREF_STAT_INC())
#else
#    ifdef Py_REF_DEBUG
#        define SSRJSON_PY_DECREF_DEBUG() (_Py_RefTotal--)
#        define SSRJSON_PY_INCREF_DEBUG() (_Py_RefTotal++)
#    else
#        define SSRJSON_PY_DECREF_DEBUG()
#        define SSRJSON_PY_INCREF_DEBUG()
#    endif
#endif


force_inline void Py_DecRef_NoCheck(PyObject *op) {
    // Non-limited C API and limited C API for Python 3.9 and older access
    // directly PyObject.ob_refcnt.
#if PY_MINOR_VERSION >= 12
    if (_Py_IsImmortal(op)) {
        return;
    }
#endif
    SSRJSON_PY_DECREF_DEBUG();
    assert(op->ob_refcnt > 1);
    --op->ob_refcnt;
}

force_inline void Py_Immortal_IncRef(PyObject *op) {
    // Non-limited C API and limited C API for Python 3.9 and older access
    // directly PyObject.ob_refcnt.
#if PY_MINOR_VERSION >= 12
    // Portable saturated add, branching on the carry flag and set low bits
#    if !defined(NDEBUG) && PY_MINOR_VERSION < 14
    assert(0 > (int32_t)op->ob_refcnt_split[PY_BIG_ENDIAN]);
#    endif // NDEBUG
#else      // PY_MINOR_VERSION >= 12
    op->ob_refcnt++;
#endif     // PY_MINOR_VERSION >= 12
    SSRJSON_PY_INCREF_DEBUG();
}

/*==============================================================================
 * Power10 Lookup Table
 * These data are used by the floating-point number reader and writer.
 *============================================================================*/

/** Normalized significant 128 bits of pow10, no rounded up (size: 10.4KB).
    This lookup table is used by both the double number reader and writer.
    (generate with misc/make_tables.c) */
extern const u64 pow10_sig_table[];

/**
 Convert normalized u64 (highest bit is 1) to f64.
 */
force_inline f64 normalized_u64_to_f64(u64 val) {
    return (f64)val;
}

/*==============================================================================
 * Read state utilities
 *============================================================================*/

// force_inline void update_max_char_type(ReadStrState *read_state, int max_char_type) {
//     assert(read_state->max_char_type < max_char_type);
//     read_state->max_char_type = max_char_type;
//     read_state->state_dirty = true;
// }

// force_inline void init_read_state(ReadStrState *state) {
//     // all initialized as 0 or false
//     memset(state, 0, sizeof(ReadStrState));
// }

/*==============================================================================
 * xxhash and key cache utilities
 *============================================================================*/


#define REHASHER(_x) (((size_t)(_x)) % (SSRJSON_KEY_CACHE_SIZE))
typedef XXH64_hash_t decode_keyhash_t;
extern decode_cache_t DecodeKeyCache[SSRJSON_KEY_CACHE_SIZE];

force_inline u64 make_key_hint(usize real_len, int kind) {
    u64 ret = SSRJSON_CAST(u64, real_len) | (SSRJSON_CAST(u64, kind) << 32);
    return ret;
}

force_inline void add_key_cache(decode_keyhash_t hash, PyObject *obj, usize real_len, int kind) {
    assert(PyUnicode_GET_LENGTH(obj) * PyUnicode_KIND(obj) <= 64);
    size_t index = REHASHER(hash);
    // SSRJSON_TRACE_HASH(index);
    decode_cache_t old = DecodeKeyCache[index];
    if (old.key) {
        // SSRJSON_TRACE_HASH_CONFLICT(hash);
        Py_DECREF(old.key);
    }
    Py_INCREF(obj);
    DecodeKeyCache[index].key = obj;
    DecodeKeyCache[index].key_hint = make_key_hint(real_len, kind);
}

force_inline PyObject *get_key_cache(const void *unicode_str, decode_keyhash_t hash, usize real_len, int kind) {
    assert(real_len <= 64);
    decode_cache_t cache = DecodeKeyCache[REHASHER(hash)];
    if (!cache.key) return NULL;
    u64 key_hint = make_key_hint(real_len, kind);
    Py_ssize_t cache_offset = kind ? sizeof(PyCompactUnicodeObject) : sizeof(PyASCIIObject);
    if (likely(key_hint == cache.key_hint && (ssrjson_memcmp_neq_le64(SSRJSON_CAST(u8 *, unicode_str), SSRJSON_CAST(u8 *, cache.key) + cache_offset, real_len) == 0))) {
        // SSRJSON_TRACE_CACHE_HIT();
        Py_INCREF(cache.key);
        return cache.key;
    }
    return NULL;
}

#endif // SSRJSON_DECODE_H

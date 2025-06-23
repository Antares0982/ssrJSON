#ifndef SSRJSON_ENCODE_SHARED_H
#define SSRJSON_ENCODE_SHARED_H

#include "simd/simd_detect.h"
#include "ssrjson.h"
#include "tls.h"
#include "utils/unicode.h"

#define CONTROL_SEQ_ESCAPE_PREFIX _Slash, 'u', '0', '0'
#define CONTROL_SEQ_ESCAPE_SUFFIX '\0', '\0'
#define CONTROL_SEQ_ESCAPE_MIDDLE CONTROL_SEQ_ESCAPE_SUFFIX, CONTROL_SEQ_ESCAPE_SUFFIX
#define CONTROL_SEQ_ESCAPE_FULL_ZERO CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_MIDDLE
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT2 CONTROL_SEQ_ESCAPE_FULL_ZERO, CONTROL_SEQ_ESCAPE_FULL_ZERO
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT4 CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT2, CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT2
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT8 CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT4, CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT4
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT16 CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT8, CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT8
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT32 CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT16, CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT16

// /** Minimum decimal exponent in pow10_sig_table. */
// #define POW10_SIG_TABLE_MIN_EXP -343

// /** Maximum decimal exponent in pow10_sig_table. */
// #define POW10_SIG_TABLE_MAX_EXP 324

// /** Minimum exact decimal exponent in pow10_sig_table */
// #define POW10_SIG_TABLE_MIN_EXACT_EXP 0

// /** Maximum exact decimal exponent in pow10_sig_table */
// #define POW10_SIG_TABLE_MAX_EXACT_EXP 55

/* double number exponent bias */
#define F64_EXP_BIAS 1023

/* double number significand part bits */
#define F64_SIG_BITS 52

/* double number bits */
#define F64_BITS 64

/* double number exponent part bits */
#define F64_EXP_BITS 11

/* double number significand bit mask */
#define F64_SIG_MASK U64(0x000FFFFF, 0xFFFFFFFF)

/* double number exponent bit mask */
#define F64_EXP_MASK U64(0x7FF00000, 0x00000000)


/*==============================================================================
 * Buffer
 *============================================================================*/

static_assert((SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE % 64) == 0, "(SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE % 64) == 0");

typedef struct EncodeCtnWithIndex {
    PyObject *ctn;
    Py_ssize_t index;
} EncodeCtnWithIndex;

#if !defined(Py_GIL_DISABLED)

extern EncodeCtnWithIndex _EncodeCtnBuffer[SSRJSON_ENCODE_MAX_RECURSION];

force_inline EncodeCtnWithIndex *get_encode_obj_stack_buffer(void) {
    return _EncodeCtnBuffer;
}
#endif

/*==============================================================================
 * Utils
 *============================================================================*/

force_inline Py_ssize_t get_indent_char_count(Py_ssize_t cur_nested_depth, Py_ssize_t indent_level) {
    return indent_level ? (indent_level * cur_nested_depth + 1) : 0;
}

/*==============================================================================
 * Python Utils
 *============================================================================*/

#define PYBYTES_START_OFFSET (offsetof(PyBytesObject, ob_sval))

extern PyObject *JSONEncodeError;

force_inline void *get_unicode_data(PyObject *unicode) {
    if (((PyASCIIObject *)unicode)->state.ascii) {
        return (void *)(((PyASCIIObject *)unicode) + 1);
    }
    return (void *)(((PyCompactUnicodeObject *)unicode) + 1);
}

force_inline bool pylong_is_unsigned(PyObject *obj) {
#if PY_MINOR_VERSION >= 12
    return !(bool)(((PyLongObject *)obj)->long_value.lv_tag & 2);
#else
    return ((PyVarObject *)obj)->ob_size > 0;
#endif
}

force_inline bool pylong_is_zero(PyObject *obj) {
#if PY_MINOR_VERSION >= 12
    return (bool)(((PyLongObject *)obj)->long_value.lv_tag & 1);
#else
    return ((PyVarObject *)obj)->ob_size == 0;
#endif
}

// PyErr may occur.
force_inline bool pylong_value_unsigned(PyObject *obj, u64 *value) {
#if PY_MINOR_VERSION >= 12
    if (likely(((PyLongObject *)obj)->long_value.lv_tag < (2 << _PyLong_NON_SIZE_BITS))) {
        *value = (u64) * ((PyLongObject *)obj)->long_value.ob_digit;
        return true;
    }
#endif
    unsigned long long v = PyLong_AsUnsignedLongLong(obj);
    if (unlikely(v == (unsigned long long)-1 && PyErr_Occurred())) {
        return false;
    }
    *value = (u64)v;
    static_assert(sizeof(unsigned long long) <= sizeof(u64), "sizeof(unsigned long long) <= sizeof(u64)");
    return true;
}

force_inline i64 pylong_value_signed(PyObject *obj, i64 *value) {
#if PY_MINOR_VERSION >= 12
    if (likely(((PyLongObject *)obj)->long_value.lv_tag < (2 << _PyLong_NON_SIZE_BITS))) {
        i64 sign = 1 - (i64)((((PyLongObject *)obj)->long_value.lv_tag & 3));
        *value = sign * (i64) * ((PyLongObject *)obj)->long_value.ob_digit;
        return true;
    }
#endif
    long long v = PyLong_AsLongLong(obj);
    if (unlikely(v == -1 && PyErr_Occurred())) {
        return false;
    }
    *value = (i64)v;
    static_assert(sizeof(long long) <= sizeof(i64), "sizeof(long long) <= sizeof(i64)");
    return true;
}

force_inline int pydict_next(PyObject *op, Py_ssize_t *ppos, PyObject **pkey,
                             PyObject **pvalue) {
#if PY_MINOR_VERSION >= 13
    return PyDict_Next(op, ppos, pkey, pvalue);
#else
    return _PyDict_Next(op, ppos, pkey, pvalue, NULL);
#endif
}

typedef enum PyFastTypes {
    T_Unicode,
    T_Long,
    T_Bool,
    // T_False,
    // T_True,
    T_None,
    T_Float,
    T_List,
    T_Dict,
    T_Tuple,
    Unknown,
} PyFastTypes;
#if PY_MINOR_VERSION >= 13
// _PyNone_Type is hidden in Python 3.13
extern PyTypeObject *PyNone_Type;
#else
#    define PyNone_Type &_PyNone_Type
#endif
#if PY_MINOR_VERSION >= 13
extern PyTypeObject *PyNone_Type;
#endif

/* Get the value type as fast as possible. */
force_inline PyFastTypes fast_type_check(PyObject *val) {
    PyTypeObject *type = Py_TYPE(val);
    assert(type);
    // #if SSRJSON_X86 && __AVX2__
    // #    if PY_MINOR_VERSION >= 13
    //     static const ssrjson_align(64) PyTypeObject *vector_py_types[8] = {
    //             &PyUnicode_Type,
    //             &PyLong_Type,
    //             &PyBool_Type,
    //             0,
    //             &PyFloat_Type,
    //             &PyList_Type,
    //             &PyDict_Type,
    //             &PyTuple_Type,
    //     };
    // #    else
    //     static const ssrjson_align(64) PyTypeObject *vector_py_types[8] = {
    //             &PyUnicode_Type,
    //             &PyLong_Type,
    //             &PyBool_Type,
    //             PyNone_Type,
    //             &PyFloat_Type,
    //             &PyList_Type,
    //             &PyDict_Type,
    //             &PyTuple_Type,
    //     };
    // #    endif
    // #    if __AVX512F__
    //     __m512i vec = _mm512_set1_epi64((i64)(uintptr_t)type);
    //     u8 mask = (u8)_mm512_cmpeq_epi64_mask(vec, *(__m512i *)vector_py_types);
    //     if (unlikely(!mask)) {
    //         if (PY_MINOR_VERSION >= 13 && type == PyNone_Type) {
    //             return T_None;
    //         }
    //         return Unknown;
    //     }
    //     usize index = u32_tz_bits(mask);
    //     assert(index < 8);
    //     return (PyFastTypes)index;
    // #    else
    //     __m256i vec = _mm256_set1_epi64x((i64)(uintptr_t)type);
    //     __m256i m1 = _mm256_cmpeq_epi64(vec, *(SSRJSON_CAST(__m256i *, vector_py_types) + 0));
    //     if (likely(!_mm256_testz_si256(m1, m1))) {
    //         u32 mask = (u32)_mm256_movemask_epi8(m1);
    //         return SSRJSON_CAST(PyFastTypes, u32_tz_bits(mask) / 8);
    //     }
    //     if (PY_MINOR_VERSION >= 13 && type == PyNone_Type) {
    //         return T_None;
    //     }
    //     m1 = _mm256_cmpeq_epi64(vec, *(SSRJSON_CAST(__m256i *, vector_py_types) + 1));
    //     if (likely(!_mm256_testz_si256(m1, m1))) {
    //         u32 mask = (u32)_mm256_movemask_epi8(m1);
    //         return SSRJSON_CAST(PyFastTypes, u32_tz_bits(mask) / 8 + 4);
    //     }
    //     return Unknown;
    // #    endif
    // #else
    if (type == &PyUnicode_Type) {
        return T_Unicode;
    } else if (type == &PyLong_Type) {
        return T_Long;
    } else if (type == &PyBool_Type) {
        return T_Bool;
    } else if (type == PyNone_Type) {
        return T_None;
    } else if (type == &PyFloat_Type) {
        return T_Float;
    } else if (type == &PyList_Type) {
        return T_List;
    } else if (type == &PyDict_Type) {
        return T_Dict;
    } else if (type == &PyTuple_Type) {
        return T_Tuple;
    } else {
        return Unknown;
    }
    // #endif
}

typedef struct EncodeUnicodeInfo {
    Py_ssize_t ascii_size;
    Py_ssize_t u8_size;
    Py_ssize_t u16_size;
    Py_ssize_t u32_size;
    int cur_ucs_type;
} EncodeUnicodeInfo;

typedef struct EncodeUnicodeBufferInfo {
    void *head;
    void *end;
} EncodeUnicodeBufferInfo;

typedef union {
    u8 *writer_u8;
    u16 *writer_u16;
    u32 *writer_u32;
    void *writer_void;
} EncodeUnicodeWriter;

#define U8_WRITER(_writer_addr_) ((_writer_addr_)->writer_u8)
#define U16_WRITER(_writer_addr_) ((_writer_addr_)->writer_u16)
#define U32_WRITER(_writer_addr_) ((_writer_addr_)->writer_u32)

#define GET_VEC_ASCII_START(_unicode_buffer_info_) (SSRJSON_CAST(PyASCIIObject *, (_unicode_buffer_info_)->head) + 1)
#define GET_VEC_COMPACT_START(_unicode_buffer_info_) (SSRJSON_CAST(PyCompactUnicodeObject *, (_unicode_buffer_info_)->head) + 1)


bool _unicode_buffer_reserve(EncodeUnicodeBufferInfo *unicode_buffer_info, usize target_size);

force_inline bool check_unicode_writer_valid(void *writer, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return SSRJSON_CAST(u8 *, writer) <= (u8 *)unicode_buffer_info->end && SSRJSON_CAST(u8 *, writer) >= (u8 *)unicode_buffer_info->head;
}

/* Resize the buffer described by `unicode_buffer_info`.
 * If resize succeed, the buffer will be updated to the new address and return true.
 * Otherwise, buffer left unchanged and returns false.
 * Args:
 *     unicode_buffer_info: The buffer.
 *     len: Count of valid unicode points in the buffer.
 *     ucs_type: The unicode type of the buffer (0 stands for ascii).
 */
bool resize_to_fit_pyunicode(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t len, int ucs_type);


/*==============================================================================
 * Writer
 *============================================================================*/

/* These codes are modified from yyjson. */


/** Digit table from 00 to 99. */
extern ssrjson_align(8) const u8 DIGIT_TABLE[200];

/** Normalized significant 128 bits of pow10, no rounded up (size: 10.4KB).
    This lookup table is used by both the double number reader and writer.
    (generate with misc/make_tables.c) */
extern const u64 pow10_sig_table[];

force_inline void byte_copy_2(void *dst, const void *src) {
    memcpy(dst, src, 2);
}

force_inline void byte_copy_4(void *dst, const void *src) {
    memcpy(dst, src, 4);
}

force_inline void byte_copy_8(void *dst, const void *src) {
    memcpy(dst, src, 8);
}

/*==============================================================================
 * Number Utils
 * These functions are used to detect and convert NaN and Inf numbers.
 *============================================================================*/

/** Convert raw binary to double. */
force_inline f64 f64_from_raw(u64 u) {
    /* use memcpy to avoid violating the strict aliasing rule */
    f64 f;
    memcpy(&f, &u, 8);
    return f;
}

// /**
//  Get the cached pow10 value from pow10_sig_table.
//  @param exp10 The exponent of pow(10, e). This value must in range
//               POW10_SIG_TABLE_MIN_EXP to POW10_SIG_TABLE_MAX_EXP.
//  @param hi    The highest 64 bits of pow(10, e).
//  @param lo    The lower 64 bits after `hi`.
//  */
// force_inline void pow10_table_get_sig(i32 exp10, u64 *hi, u64 *lo) {
//     i32 idx = exp10 - (POW10_SIG_TABLE_MIN_EXP);
//     *hi = pow10_sig_table[idx * 2];
//     *lo = pow10_sig_table[idx * 2 + 1];
// }

/*==============================================================================
 * Integer Writer
 *
 * The maximum value of uint32_t is 4294967295 (10 digits),
 * these digits are named as 'aabbccddee' here.
 *
 * Although most compilers may convert the "division by constant value" into
 * "multiply and shift", manual conversion can still help some compilers
 * generate fewer and better instructions.
 *
 * Reference:
 * Division by Invariant Integers using Multiplication, 1994.
 * https://gmplib.org/~tege/divcnst-pldi94.pdf
 * Improved division by invariant integers, 2011.
 * https://gmplib.org/~tege/division-paper.pdf
 *============================================================================*/

force_inline u8 *write_u32_len_8(u32 val, u8 *buf) {
    u32 aa, bb, cc, dd, aabb, ccdd;             /* 8 digits: aabbccdd */
    aabb = (u32)(((u64)val * 109951163) >> 40); /* (val / 10000) */
    ccdd = val - aabb * 10000;                  /* (val % 10000) */
    aa = (aabb * 5243) >> 19;                   /* (aabb / 100) */
    cc = (ccdd * 5243) >> 19;                   /* (ccdd / 100) */
    bb = aabb - aa * 100;                       /* (aabb % 100) */
    dd = ccdd - cc * 100;                       /* (ccdd % 100) */
    byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2);
    byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
    byte_copy_2(buf + 4, DIGIT_TABLE + cc * 2);
    byte_copy_2(buf + 6, DIGIT_TABLE + dd * 2);
    return buf + 8;
}

force_inline u8 *write_u32_len_4(u32 val, u8 *buf) {
    u32 aa, bb;              /* 4 digits: aabb */
    aa = (val * 5243) >> 19; /* (val / 100) */
    bb = val - aa * 100;     /* (val % 100) */
    byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2);
    byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
    return buf + 4;
}

force_inline u8 *write_u32_len_1_8(u32 val, u8 *buf) {
    u32 aa, bb, cc, dd, aabb, bbcc, ccdd, lz;

    if (val < 100) {   /* 1-2 digits: aa */
        lz = val < 10; /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, DIGIT_TABLE + val * 2 + lz);
        buf -= lz;
        return buf + 2;

    } else if (val < 10000) {    /* 3-4 digits: aabb */
        aa = (val * 5243) >> 19; /* (val / 100) */
        bb = val - aa * 100;     /* (val % 100) */
        lz = aa < 10;            /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
        return buf + 4;

    } else if (val < 1000000) {                /* 5-6 digits: aabbcc */
        aa = (u32)(((u64)val * 429497) >> 32); /* (val / 10000) */
        bbcc = val - aa * 10000;               /* (val % 10000) */
        bb = (bbcc * 5243) >> 19;              /* (bbcc / 100) */
        cc = bbcc - bb * 100;                  /* (bbcc % 100) */
        lz = aa < 10;                          /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
        byte_copy_2(buf + 4, DIGIT_TABLE + cc * 2);
        return buf + 6;

    } else {                                        /* 7-8 digits: aabbccdd */
        aabb = (u32)(((u64)val * 109951163) >> 40); /* (val / 10000) */
        ccdd = val - aabb * 10000;                  /* (val % 10000) */
        aa = (aabb * 5243) >> 19;                   /* (aabb / 100) */
        cc = (ccdd * 5243) >> 19;                   /* (ccdd / 100) */
        bb = aabb - aa * 100;                       /* (aabb % 100) */
        dd = ccdd - cc * 100;                       /* (ccdd % 100) */
        lz = aa < 10;                               /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
        byte_copy_2(buf + 4, DIGIT_TABLE + cc * 2);
        byte_copy_2(buf + 6, DIGIT_TABLE + dd * 2);
        return buf + 8;
    }
}

force_inline u8 *write_u64_len_5_8(u32 val, u8 *buf) {
    u32 aa, bb, cc, dd, aabb, bbcc, ccdd, lz;

    if (val < 1000000) {                       /* 5-6 digits: aabbcc */
        aa = (u32)(((u64)val * 429497) >> 32); /* (val / 10000) */
        bbcc = val - aa * 10000;               /* (val % 10000) */
        bb = (bbcc * 5243) >> 19;              /* (bbcc / 100) */
        cc = bbcc - bb * 100;                  /* (bbcc % 100) */
        lz = aa < 10;                          /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
        byte_copy_2(buf + 4, DIGIT_TABLE + cc * 2);
        return buf + 6;

    } else {                                        /* 7-8 digits: aabbccdd */
        aabb = (u32)(((u64)val * 109951163) >> 40); /* (val / 10000) */
        ccdd = val - aabb * 10000;                  /* (val % 10000) */
        aa = (aabb * 5243) >> 19;                   /* (aabb / 100) */
        cc = (ccdd * 5243) >> 19;                   /* (ccdd / 100) */
        bb = aabb - aa * 100;                       /* (aabb % 100) */
        dd = ccdd - cc * 100;                       /* (ccdd % 100) */
        lz = aa < 10;                               /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
        byte_copy_2(buf + 4, DIGIT_TABLE + cc * 2);
        byte_copy_2(buf + 6, DIGIT_TABLE + dd * 2);
        return buf + 8;
    }
}

force_inline u8 *write_u64(u64 val, u8 *buf) {
    u64 tmp, hgh;
    u32 mid, low;

    if (val < 100000000) { /* 1-8 digits */
        buf = write_u32_len_1_8((u32)val, buf);
        return buf;

    } else if (val < (u64)100000000 * 100000000) { /* 9-16 digits */
        hgh = val / 100000000;                     /* (val / 100000000) */
        low = (u32)(val - hgh * 100000000);        /* (val % 100000000) */
        buf = write_u32_len_1_8((u32)hgh, buf);
        buf = write_u32_len_8(low, buf);
        return buf;

    } else {                                /* 17-20 digits */
        tmp = val / 100000000;              /* (val / 100000000) */
        low = (u32)(val - tmp * 100000000); /* (val % 100000000) */
        hgh = (u32)(tmp / 10000);           /* (tmp / 10000) */
        mid = (u32)(tmp - hgh * 10000);     /* (tmp % 10000) */
        buf = write_u64_len_5_8((u32)hgh, buf);
        buf = write_u32_len_4(mid, buf);
        buf = write_u32_len_8(low, buf);
        return buf;
    }
}

force_inline Py_ssize_t get_unicode_buffer_final_len_ascii(EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info) {
#if COMPILE_UCS_LEVEL == 0
    return writer.writer_u8 - (u8 *)GET_VEC_ASCII_START(unicode_buffer_info);
#elif COMPILE_UCS_LEVEL == 1
#elif COMPILE_UCS_LEVEL == 2
    return unicode_buffer_info->writer.writer_u16 - (u16 *)GET_VEC_COMPACT_START(unicode_buffer_info);
#elif COMPILE_UCS_LEVEL == 4
    return unicode_buffer_info->writer.writer_u32 - (u32 *)GET_VEC_COMPACT_START(unicode_buffer_info);
#endif
}

force_inline Py_ssize_t get_unicode_buffer_final_len_ucs1(EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return writer.writer_u8 - (u8 *)GET_VEC_COMPACT_START(unicode_buffer_info);
}

force_inline Py_ssize_t get_unicode_buffer_final_len_ucs2(EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return writer.writer_u16 - (u16 *)GET_VEC_COMPACT_START(unicode_buffer_info);
}

force_inline Py_ssize_t get_unicode_buffer_final_len_ucs4(EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return writer.writer_u32 - (u32 *)GET_VEC_COMPACT_START(unicode_buffer_info);
}

typedef enum EncodeValJumpFlag {
    JumpFlag_Default,
    JumpFlag_ArrValBegin,
    JumpFlag_DictPairBegin,
    JumpFlag_TupleValBegin,
    JumpFlag_Elevate1_ArrVal,
    JumpFlag_Elevate1_ObjVal,
    JumpFlag_Elevate1_Key,
    JumpFlag_Elevate2_ArrVal,
    JumpFlag_Elevate2_ObjVal,
    JumpFlag_Elevate2_Key,
    JumpFlag_Elevate4_ArrVal,
    JumpFlag_Elevate4_ObjVal,
    JumpFlag_Elevate4_Key,
    JumpFlag_Fail,
} EncodeValJumpFlag;

typedef enum EncodeCallFlag {
    CallFlag_ObjVal,
    CallFlag_ArrVal,
    CallFlag_Key,
} EncodeCallFlag;

force_inline bool init_encode_ctn_stack(EncodeCtnWithIndex **ctn_stack_addr) {
    EncodeCtnWithIndex *ctn_stack = get_encode_obj_stack_buffer();
    *ctn_stack_addr = ctn_stack;
    if (unlikely(!ctn_stack)) {
        PyErr_NoMemory();
        return false;
    }
    return true;
}

force_inline bool _init_encode_buffer(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, usize u8_start_offset) {
    unicode_buffer_info->head = PyObject_Malloc(SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    if (likely(unicode_buffer_info->head)) {
#ifndef NDEBUG
        memset(unicode_buffer_info->head, 0, SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE);
#endif
        writer_addr->writer_void = SSRJSON_CAST(u8 *, unicode_buffer_info->head) + u8_start_offset;
        unicode_buffer_info->end = SSRJSON_CAST(u8 *, unicode_buffer_info->head) + SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE;
    } else {
        PyErr_NoMemory();
        return false;
    }
    return true;
}

force_inline bool init_unicode_buffer(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return _init_encode_buffer(writer_addr, unicode_buffer_info, sizeof(PyASCIIObject));
}

force_inline bool init_bytes_buffer(EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return _init_encode_buffer(writer_addr, unicode_buffer_info, PYBYTES_START_OFFSET);
}

force_inline usize get_bytes_buffer_final_len(u8 *writer, void *head) {
    assert(writer >= SSRJSON_CAST(u8 *, head) + PYBYTES_START_OFFSET);
    usize ret = writer - SSRJSON_CAST(u8 *, head) - PYBYTES_START_OFFSET;
    return ret;
}

force_inline bool resize_to_fit_pybytes(EncodeUnicodeBufferInfo *unicode_buffer_info, usize len) {
    usize buffer_total_size = PYBYTES_START_OFFSET + len + 1;
    void *new_ptr = PyObject_Realloc(unicode_buffer_info->head, buffer_total_size);
    if (unlikely(!new_ptr)) {
        return false;
    }
    unicode_buffer_info->head = new_ptr;
    return true;
}

force_inline void init_pybytes(PyObject *in_new_bytes, usize final_len) {
    PyBytesObject *new_bytes = SSRJSON_CAST(PyBytesObject *, in_new_bytes);
    PyObject_Init(in_new_bytes, &PyBytes_Type);
    new_bytes->ob_base.ob_size = (Py_ssize_t)final_len;
#if PY_MINOR_VERSION < 11
    new_bytes->ob_shash = -1;
#endif
    new_bytes->ob_sval[final_len] = 0;
}

#endif // SSRJSON_ENCODE_SHARED_H

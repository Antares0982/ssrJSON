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

#ifndef SSRJSON_ENCODE_SHARED_H
#define SSRJSON_ENCODE_SHARED_H

#include "encode_typedefs.h"
#include "simd/simd_detect.h"
#include "tls.h"
#include "utils/unicode.h"
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
#    include "khash.h"
#endif

/*==============================================================================
 * Macros
 *============================================================================*/

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

#define WRITER_AS_U8(_writer_) (*SSRJSON_CAST(u8 **, &(_writer_)))
#define WRITER_AS_U16(_writer_) (*SSRJSON_CAST(u16 **, &(_writer_)))
#define WRITER_AS_U32(_writer_) (*SSRJSON_CAST(u32 **, &(_writer_)))

#define GET_VEC_ASCII_START(_unicode_buffer_info_) (SSRJSON_CAST(PyASCIIObject *, (_unicode_buffer_info_)->head) + 1)
#define GET_VEC_COMPACT_START(_unicode_buffer_info_) (SSRJSON_CAST(PyCompactUnicodeObject *, (_unicode_buffer_info_)->head) + 1)

/*==============================================================================
 * Global Vars
 *============================================================================*/
#if SSRJSON_GIL_ENABLED
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

// `is_dict` is known at compile time, but `is_tuple` is not
force_inline EncodeContainerType get_encode_ctn_type(ssrjson_compiletime bool is_dict, bool is_tuple) {
    if (ssrjson_consteval(is_dict)) {
        return EncodeContainerType_Dict;
    }
    return EncodeContainerType_List << unlikely(is_tuple);
}

force_inline void extract_index_and_type(EncodeCtnWithIndex *ctn_with_index, Py_ssize_t *index, EncodeContainerType *type) {
    *index = SSRJSON_CAST(Py_ssize_t, ctn_with_index->index_and_type >> 2);
    *type = ctn_with_index->index_and_type & 0x3;
}

force_inline void set_index_and_type(EncodeCtnWithIndex *ctn_with_index, Py_ssize_t index, EncodeContainerType type) {
    usize _index = SSRJSON_CAST(usize, index);
    // not expect index so large that it will overflow after left shift
    assert((_index & ~(SSRJSON_CAST(usize, -1) >> 2)) == 0);
    ctn_with_index->index_and_type = (_index << 2) | type;
}

/*==============================================================================
 * Python Utils
 *============================================================================*/

#define PYBYTES_START_OFFSET (offsetof(PyBytesObject, ob_sval))

extern PyObject *JSONEncodeError;

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

EncodePyTypes slow_type_check(PyTypeObject *type);

/* Get the value type as fast as possible. */
force_inline EncodePyTypes ssrjson_type_check(PyObject *val) {
    u64 type = (u64)Py_TYPE(val);
    assert(type);
    u64 *py_fast_type = _PyFastType;
    if (type == _PyFastType[0]) {
        return T_Unicode;
    } else if (type == _PyFastType[1]) {
        return T_Long;
    } else if (type == _PyFastType[2]) {
        return T_Bool;
    } else if (type == _PyFastType[3]) {
        return T_None;
    } else if (type == _PyFastType[4]) {
        return T_Float;
    } else if (type == _PyFastType[5]) {
        return T_List;
    } else if (type == _PyFastType[6]) {
        return T_Dict;
    } else if (type == _PyFastType[7]) {
        return T_Tuple;
    } else {
        return slow_type_check((PyTypeObject *)type);
    }
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

/*==============================================================================
 * Writer
 *============================================================================*/


EncodeUnicodeBufferInfo _unicode_buffer_reserve(EncodeUnicodeBufferInfo unicode_buffer_info, usize target_size);

#ifndef NDEBUG
force_inline bool check_unicode_writer_valid(void *writer, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return SSRJSON_CAST(u8 *, writer) <= (u8 *)unicode_buffer_info->end && SSRJSON_CAST(u8 *, writer) >= (u8 *)unicode_buffer_info->head;
}
#endif

/* Resize the buffer described by `unicode_buffer_info`.
 * If resize succeed, the buffer will be updated to the new address and return true.
 * Otherwise, buffer left unchanged and returns false.
 * Args:
 *     unicode_buffer_info: The buffer.
 *     len: Count of valid unicode points in the buffer.
 *     ucs_type: The unicode type of the buffer (0 stands for ascii).
 */
bool resize_to_fit_pyunicode(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t len, int ucs_type);

/** Digit table from 00 to 99. */
extern ssrjson_align(8) const u8 EncodeDigitTable[200];

force_inline void byte_copy_2(void *dst, const void *src) {
    memcpy(dst, src, 2);
}

force_inline Py_ssize_t get_unicode_buffer_final_len_ascii(EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return WRITER_AS_U8(writer) - (u8 *)GET_VEC_ASCII_START(unicode_buffer_info);
}

force_inline Py_ssize_t get_unicode_buffer_final_len_ucs1(EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return WRITER_AS_U8(writer) - (u8 *)GET_VEC_COMPACT_START(unicode_buffer_info);
}

force_inline Py_ssize_t get_unicode_buffer_final_len_ucs2(EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return WRITER_AS_U16(writer) - (u16 *)GET_VEC_COMPACT_START(unicode_buffer_info);
}

force_inline Py_ssize_t get_unicode_buffer_final_len_ucs4(EncodeUnicodeWriter writer, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return WRITER_AS_U32(writer) - (u32 *)GET_VEC_COMPACT_START(unicode_buffer_info);
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
    byte_copy_2(buf + 0, EncodeDigitTable + aa * 2);
    byte_copy_2(buf + 2, EncodeDigitTable + bb * 2);
    byte_copy_2(buf + 4, EncodeDigitTable + cc * 2);
    byte_copy_2(buf + 6, EncodeDigitTable + dd * 2);
    return buf + 8;
}

force_inline u8 *write_u32_len_4(u32 val, u8 *buf) {
    u32 aa, bb;              /* 4 digits: aabb */
    aa = (val * 5243) >> 19; /* (val / 100) */
    bb = val - aa * 100;     /* (val % 100) */
    byte_copy_2(buf + 0, EncodeDigitTable + aa * 2);
    byte_copy_2(buf + 2, EncodeDigitTable + bb * 2);
    return buf + 4;
}

force_inline u8 *write_u32_len_1_8(u32 val, u8 *buf) {
    u32 aa, bb, cc, dd, aabb, bbcc, ccdd, lz;

    if (val < 100) {   /* 1-2 digits: aa */
        lz = val < 10; /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, EncodeDigitTable + val * 2 + lz);
        buf -= lz;
        return buf + 2;

    } else if (val < 10000) {    /* 3-4 digits: aabb */
        aa = (val * 5243) >> 19; /* (val / 100) */
        bb = val - aa * 100;     /* (val % 100) */
        lz = aa < 10;            /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, EncodeDigitTable + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, EncodeDigitTable + bb * 2);
        return buf + 4;

    } else if (val < 1000000) {                /* 5-6 digits: aabbcc */
        aa = (u32)(((u64)val * 429497) >> 32); /* (val / 10000) */
        bbcc = val - aa * 10000;               /* (val % 10000) */
        bb = (bbcc * 5243) >> 19;              /* (bbcc / 100) */
        cc = bbcc - bb * 100;                  /* (bbcc % 100) */
        lz = aa < 10;                          /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, EncodeDigitTable + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, EncodeDigitTable + bb * 2);
        byte_copy_2(buf + 4, EncodeDigitTable + cc * 2);
        return buf + 6;

    } else {                                        /* 7-8 digits: aabbccdd */
        aabb = (u32)(((u64)val * 109951163) >> 40); /* (val / 10000) */
        ccdd = val - aabb * 10000;                  /* (val % 10000) */
        aa = (aabb * 5243) >> 19;                   /* (aabb / 100) */
        cc = (ccdd * 5243) >> 19;                   /* (ccdd / 100) */
        bb = aabb - aa * 100;                       /* (aabb % 100) */
        dd = ccdd - cc * 100;                       /* (ccdd % 100) */
        lz = aa < 10;                               /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, EncodeDigitTable + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, EncodeDigitTable + bb * 2);
        byte_copy_2(buf + 4, EncodeDigitTable + cc * 2);
        byte_copy_2(buf + 6, EncodeDigitTable + dd * 2);
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
        byte_copy_2(buf + 0, EncodeDigitTable + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, EncodeDigitTable + bb * 2);
        byte_copy_2(buf + 4, EncodeDigitTable + cc * 2);
        return buf + 6;

    } else {                                        /* 7-8 digits: aabbccdd */
        aabb = (u32)(((u64)val * 109951163) >> 40); /* (val / 10000) */
        ccdd = val - aabb * 10000;                  /* (val % 10000) */
        aa = (aabb * 5243) >> 19;                   /* (aabb / 100) */
        cc = (ccdd * 5243) >> 19;                   /* (ccdd / 100) */
        bb = aabb - aa * 100;                       /* (aabb % 100) */
        dd = ccdd - cc * 100;                       /* (ccdd % 100) */
        lz = aa < 10;                               /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, EncodeDigitTable + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, EncodeDigitTable + bb * 2);
        byte_copy_2(buf + 4, EncodeDigitTable + cc * 2);
        byte_copy_2(buf + 6, EncodeDigitTable + dd * 2);
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

/*==============================================================================
 * Buffer Init
 *============================================================================*/

force_inline bool init_encode_ctn_stack(EncodeCtnWithIndex **ctn_stack_addr) {
    EncodeCtnWithIndex *ctn_stack = get_encode_obj_stack_buffer();
    *ctn_stack_addr = ctn_stack;
#if SSRJSON_GIL_ENABLED
    assert(ctn_stack);
#else
    if (unlikely(!ctn_stack)) {
        PyErr_NoMemory();
        return false;
    }
#endif
    return true;
}

force_inline EncodeUnicodeWriter _init_encode_buffer(EncodeUnicodeBufferInfo *unicode_buffer_info, usize u8_start_offset) {
    EncodeUnicodeWriter writer;
    unicode_buffer_info->head = PyObject_Malloc(SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    if (likely(unicode_buffer_info->head)) {
#ifndef NDEBUG
        memset(unicode_buffer_info->head, 0, SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE);
#endif
        WRITER_AS_U8(writer) = SSRJSON_CAST(u8 *, unicode_buffer_info->head) + u8_start_offset;
        unicode_buffer_info->end = SSRJSON_CAST(u8 *, unicode_buffer_info->head) + SSRJSON_ENCODE_DST_BUFFER_INIT_SIZE;
    } else {
        PyErr_NoMemory();
        return NULL;
    }
    return writer;
}

force_inline EncodeUnicodeWriter init_unicode_buffer(EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return _init_encode_buffer(unicode_buffer_info, sizeof(PyASCIIObject));
}

force_inline u8 *init_bytes_buffer(EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return _init_encode_buffer(unicode_buffer_info, PYBYTES_START_OFFSET);
}

/*==============================================================================
 * Hash set for free-threading circular reference detection
 *============================================================================*/
#if !SSRJSON_GIL_ENABLED && !SSRJSON_FREE_THREADING_LOCKFREE
KHASH_SET_INIT_INT64(ptr_set)
#endif

#endif // SSRJSON_ENCODE_SHARED_H

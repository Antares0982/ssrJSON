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

#ifdef SSRJSON_CLANGD_DUMMY
#    ifndef COMPILE_READ_UCS_LEVEL
#        include "decode/decode_shared.h"
#        include "simd/simd_impl.h"
#        define COMPILE_READ_UCS_LEVEL 2
#    endif
#endif

#include "compile_context/r_in.inl.h"

force_inline bool verify_escape_hex(const _src_t *src, const _src_t *src_end, int offset) {
    if (unlikely(src + 4 + offset > src_end)) {
        PyErr_SetString(JSONDecodeError, "Unexpected ending when reading escaped sequence in string");
        return false;
    }
    // need to verify the next 4 unicode for u16 and u32, since the size of hex conv table is 256
    // u8: no need to check
#if COMPILE_READ_UCS_LEVEL == 2
    typedef union {
        u64 u64_value;
        vector_a_u16_64 vec;
    } Verifier;

    Verifier srcvec, mask;
    const vector_a_u16_64 _template = {0xff00, 0xff00, 0xff00, 0xff00};

    srcvec.vec = *(vector_u_u16_64 *)(src + offset);
    mask.vec = _template;

    // const vector_a_u16_64 verify_mask = {0xff00, 0xff00, 0xff00, 0xff00};
    if (unlikely((srcvec.u64_value & mask.u64_value) != 0)) {
        PyErr_SetString(JSONDecodeError, "Invalid escape sequence in string");
        return false;
    }
#elif COMPILE_READ_UCS_LEVEL == 4
    vector_a_u32_128 to_verify = *(vector_u_u32_128 *)(src + offset); //load_128((void *)(decode_src_info->src + offset));
    const vector_a_u32_128 verify_mask = broadcast_u32_128(0xffffff00);
    if (unlikely(!testz2_128(to_verify, verify_mask))) {
        PyErr_SetString(JSONDecodeError, "Invalid escape sequence in string");
        return false;
    }
#endif
    return true;
}

extern const u8 hex_conv_table[256];

/**
 Scans an escaped character sequence as a UTF-16 code unit (branchless).
 e.g. "\\u005C" should pass "005C" as `cur`.
 
 This requires the string has 4-byte zero padding.
 */
force_inline bool read_to_hex(const _src_t *cur, u16 *val) {
    u16 c0, c1, c2, c3, t0, t1;
    assert(cur[0] <= U8MAX);
    assert(cur[1] <= U8MAX);
    assert(cur[2] <= U8MAX);
    assert(cur[3] <= U8MAX);
    c0 = hex_conv_table[cur[0]];
    c1 = hex_conv_table[cur[1]];
    c2 = hex_conv_table[cur[2]];
    c3 = hex_conv_table[cur[3]];
    t0 = (u16)((c0 << 8) | c2);
    t1 = (u16)((c1 << 8) | c3);
    *val = (u16)((t0 << 4) | t1);
    return ((t0 | t1) & (u16)0xF0F0) == 0;
}

/** Read 'true' literal, '*cur' should be 't'. */
force_inline bool _read_true(const _src_t **restrict ptr, const _src_t *restrict end) {
    _src_t *cur = (_src_t *)*ptr;
    ssrjson_align(sizeof(_src_t) * 4) static const _src_t t[4] = {'t', 'r', 'u', 'e'};
    if (likely(end >= cur + 4 && memcmp(cur, t, 4 * sizeof(_src_t)) == 0)) {
        *ptr = cur + 4;
        return true;
    }
    return false;
}

/** Read 'false' literal, '*cur' should be 'f'. */
force_inline bool _read_false(const _src_t **restrict ptr, const _src_t *restrict end) {
    // the first 'f' is already checked
    _src_t *cur = (_src_t *)*ptr;
    ssrjson_align(sizeof(_src_t) * 4) static const _src_t t[4] = {'a', 'l', 's', 'e'};
    if (likely(end >= cur + 4 && memcmp(cur + 1, t, 4 * sizeof(_src_t)) == 0)) {
        *ptr = cur + 5;
        return true;
    }
    return false;
}

/** Read 'null' literal, '*cur' should be 'n'. */
force_inline bool _read_null(const _src_t **restrict ptr, const _src_t *restrict end) {
    _src_t *cur = (_src_t *)*ptr;
    ssrjson_align(sizeof(_src_t) * 4) static const _src_t t[4] = {'n', 'u', 'l', 'l'};
    if (likely(end >= cur + 4 && memcmp(cur, t, 4 * sizeof(_src_t)) == 0)) {
        *ptr = cur + 4;
        return true;
    }
    return false;
}

/** Read 'Infinity' literal (ignoring case). */
force_inline bool _read_inf(const _src_t **ptr, const _src_t *end) {
#define read_inf_vector SSRJSON_CONCAT4(vector, a, _src_t, READ_BIT_SIZEx8)
#define read_inf_vector_u SSRJSON_CONCAT4(vector, u, _src_t, READ_BIT_SIZEx8)
    if (unlikely(end < *ptr + 8)) {
        return false;
    }
    read_inf_vector _mask = {~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20,
                             ~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20};
    read_inf_vector _template = {'I', 'N', 'F', 'I', 'N', 'I', 'T', 'Y'};
    read_inf_vector data;
    data = *(read_inf_vector_u *)(*ptr);
    data = data & _mask;
    if (likely(0 == memcmp(&data, &_template, sizeof(data)))) {
        *ptr += 8;
        return true;
    }
    return false;
#undef read_inf_vector_u
#undef read_inf_vector
}

/** Read 'NaN' literal (ignoring case). */
force_inline bool _read_nan(const _src_t **restrict ptr, const _src_t *restrict end) {
#define read_nan_vector SSRJSON_CONCAT4(vector, a, _src_t, READ_BIT_SIZEx4)
#define read_nan_vector_u SSRJSON_CONCAT4(vector, u, _src_t, READ_BIT_SIZEx4)
    if (unlikely(end < *ptr + 3)) {
        return false;
    }
    // it is safe to load *end, so here we load `4 * sizeof(_src_t)` bytes
    read_nan_vector _mask = {~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20, 0};
    read_nan_vector _template = {'N', 'A', 'N', 0};
    read_nan_vector data = *(read_nan_vector_u *)(*ptr);
    data = data & _mask;
    if (likely(0 == memcmp(&data, &_template, sizeof(data)))) {
        *ptr += 3;
        return true;
    }
    return false;
#undef read_nan_vector_u
#undef read_nan_vector
}

/** Read 'Infinity' or 'NaN' literal (ignoring case). */
force_inline PyObject *read_inf_or_nan(bool sign, const _src_t **ptr, const _src_t *end) {
    if (_read_inf(ptr, end)) {
        return PyFloat_FromDouble(sign ? -fabs(Py_HUGE_VAL) : fabs(Py_HUGE_VAL));
    }
    if (_read_nan(ptr, end)) {
        return PyFloat_FromDouble(sign ? -fabs(Py_NAN) : fabs(Py_NAN));
    }
    return NULL;
}

#include "compile_context/r_out.inl.h"

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

#include "encode_fuzz_gen.h"
#include "pyutils.h"

// ---------------------------------------------------------------------------
// Python-side references, resolved once in encode_fuzz_gen_init().
// ---------------------------------------------------------------------------

static PyObject *g_dict_subclass = NULL;
static PyObject *g_list_subclass = NULL;
static PyObject *g_tuple_subclass = NULL;
static PyObject *g_int_subclass = NULL;
static PyObject *g_float_subclass = NULL;
static PyObject *g_str_subclass = NULL;

// numpy
static bool g_has_numpy = false;
static PyObject *g_np_frombuffer = NULL;

typedef struct {
    PyObject *dtype; // numpy scalar type object, e.g. np.float64
    int itemsize;
} NumpyDType;

// Keep in sync with the Python NUMPY_DTYPES order (only used for naming here).
static NumpyDType g_np_dtypes[12];
static int g_np_dtype_count = 0;

// ---------------------------------------------------------------------------
// Byte consumer + generation state.
// ---------------------------------------------------------------------------

typedef struct {
    const u8 *data;
    usize size;
    usize pos;
    bool has_surrogate;
} GenState;

#define MAX_DEPTH 100

// value type ids
#define TYPE_STR 0
#define TYPE_INT 1
#define TYPE_BOOL 2
#define TYPE_NONE 3
#define TYPE_FLOAT 4
#define TYPE_NDARRAY 5
#define TYPE_NUMPY_VAL 6

// container type ids
#define TYPE_DICT 0
#define TYPE_LIST 1
#define TYPE_TUPLE 2

static u8 bc_byte(GenState *st) {
    if (st->pos >= st->size) return 0;
    return st->data[st->pos++];
}

static u32 bc_choose(GenState *st, u32 n) { return ((u32)bc_byte(st)) % n; }

static bool bc_has(GenState *st) { return st->pos < st->size; }

static u64 bc_u64(GenState *st) {
    u64 v = 0;
    for (int i = 0; i < 8; i++) { v |= ((u64)bc_byte(st)) << (i * 8); }
    return v;
}

// ---------------------------------------------------------------------------
// String template fill.
//
// The bulk of a string is coverage-irrelevant filler; only the string's kind
// and the position of the escape/surrogate characters gate encode branches.
// We fill the filler by reading a fixed deterministic template at index
// (offset + position): incrementing the byte-chosen `offset` by 1 shifts every
// character to the adjacent slot, which keeps mutation local while letting long
// strings cost ~constant input bytes.
// ---------------------------------------------------------------------------

static u32 tpl_value(usize idx) {
    // Fixed pseudo template (Knuth multiplicative hash). Deterministic.
    return (u32)(((u64)idx * 2654435761u) >> 8);
}

// Map a raw template value into the canonical "safe" filler range for `kind`,
// excluding JSON escape chars and surrogates. Filler stays at/above the kind's
// minimum codepoint so the resulting string is canonical for that kind.
static u32 safe_filler(int kind, u32 v) {
    switch (kind) {
        case 0: { // ASCII [0x20, 0x7F], excluding '"' and '\\'
            u32 c = 0x20 + (v % (0x80 - 0x20));
            if (c == 0x22 || c == 0x5C) c = 0x20;
            return c;
        }
        case 1: { // Latin-1, use [0xA0, 0xFF] (all >= 0x80, no escapes here)
            return 0xA0 + (v % (0x100 - 0xA0));
        }
        case 2: { // UCS2 [0x100, 0xFFFF] excluding surrogates
            u32 r = v % 0xF700u;
            return (r < 0xD700u) ? (0x100u + r) : (0xE000u + (r - 0xD700u));
        }
        default: { // kind == 4, UCS4 [0x10000, 0x10FFFF]
            return 0x10000u + (v % 0x100000u);
        }
    }
}

static u32 pick_escape_char(GenState *st) {
    // 34 options: 0x00..0x1F, plus '"' (0x22) and '\\' (0x5C).
    u32 idx = bc_choose(st, 34);
    if (idx < 0x20) return idx;
    return (idx == 0x20) ? 0x22 : 0x5C;
}

// Write codepoint `c` at index `i` of a unicode buffer of the given kind.
static void put_char(void *data, int kind, usize i, u32 c) {
    if (kind <= 1) {
        ((u8 *)data)[i] = (u8)c;
    } else if (kind == 2) {
        ((u16 *)data)[i] = (u16)c;
    } else {
        ((u32 *)data)[i] = c;
    }
}

// A lone surrogate in [0xD800, 0xDFFF] from two input bytes.
static u32 pick_surrogate(GenState *st) {
    u32 lo = bc_byte(st);
    u32 hi = bc_byte(st);
    return 0xD800u + (((lo | (hi << 8))) % 0x800u);
}

// True if the (kind >= 2) buffer contains any lone surrogate. Used to set the
// has_surrogate flag accurately after fills that may overwrite each other.
static bool has_lone_surrogate(const void *data, int kind, usize length) {
    if (kind < 2) return false;
    for (usize i = 0; i < length; i++) {
        u32 c = (kind == 2) ? ((const u16 *)data)[i] : ((const u32 *)data)[i];
        if (c >= 0xD800u && c <= 0xDFFFu) return true;
    }
    return false;
}

static PyObject *gen_str(GenState *st, bool allow_surrogate) {
    static const int kinds[4] = {0, 1, 2, 4};
    int kind = kinds[bc_choose(st, 4)];

    // length bucket (in characters)
    static const u32 len_lo[4] = {0, 16, 32, 64};
    static const u32 len_hi[4] = {16, 32, 64, 1024};
    u32 lb = bc_choose(st, 4);
    u32 span = len_hi[lb] - len_lo[lb];
    u32 len_lo_byte = bc_byte(st);
    u32 len_hi_byte = bc_byte(st);
    usize length = len_lo[lb] + ((len_lo_byte | (len_hi_byte << 8)) % span);

    // Fill strategy. Normal: mostly ordinary chars with sparse escapes and
    // surrogates. Inverse: mostly escapes/surrogates with a few ordinary chars,
    // which saturates the SIMD encoder's escape/surrogate paths over long runs
    // -- a density the normal mode's bounded counts cannot reach.
    bool inverse = bc_choose(st, 4) == 0;

    u32 escape_count = 0;
    u32 surrogate_count = 0;
    if (!inverse) {
        // Byte-chosen counts (not booleans): a string may carry several escapes
        // and several surrogates, so the SIMD encoder's multi-escape-per-vector
        // and consecutive-escape paths get exercised.
        escape_count = bc_choose(st, 9); // 0..8
        if (allow_surrogate && kind >= 2) {
            surrogate_count = bc_choose(st, 5); // 0..4
        }
        // Slot 0 is reserved as a pure filler anchor (never overwritten), which
        // keeps the string canonical for its kind regardless of how many
        // low/surrogate codepoints we inject.
        usize min_len = 1 + (usize)escape_count + (usize)surrogate_count;
        if (length < min_len) length = min_len;
    } else if (length < 2) {
        length = 2; // slot 0 anchor + at least one escape/surrogate slot
    }

    PyObject *str = create_empty_unicode(length, kind);
    if (unlikely(!str)) return NULL;

    void *data;
    if (kind <= 1) {
        data = PyUnicode_1BYTE_DATA(str);
    } else if (kind == 2) {
        data = PyUnicode_2BYTE_DATA(str);
    } else {
        data = PyUnicode_4BYTE_DATA(str);
    }

    usize offset = bc_byte(st);

    if (!inverse) {
        for (usize i = 0; i < length; i++) { put_char(data, kind, i, safe_filler(kind, tpl_value(offset + i))); }

        // Inject escapes then surrogates at byte-chosen positions in [1, length).
        // Positions may collide; that just lowers the effective count and is
        // fine. Slot 0 stays untouched as the canonical anchor.
        for (u32 k = 0; k < escape_count; k++) {
            usize pos = 1 + (bc_byte(st) % (length - 1));
            put_char(data, kind, pos, pick_escape_char(st));
        }
        for (u32 k = 0; k < surrogate_count; k++) {
            usize pos = 1 + (bc_byte(st) % (length - 1));
            put_char(data, kind, pos, pick_surrogate(st));
        }
        if (surrogate_count > 0) st->has_surrogate = true;
    } else {
        // The bulk is escapes and (for UCS2/UCS4) surrogates, drawn
        // deterministically from the template so long strings stay compact in
        // input bytes. An ordinary anchor char is only forced where a
        // pure-escape/surrogate fill would be non-canonical:
        //   kind 0 (ASCII):  none -- all escapes are < 0x80.
        //   kind 2 + surr:   a lone surrogate (>= 0xD800) anchors the kind, so
        //                    force one surrogate and skip the ordinary anchor.
        //   kind 1:          needs a char in [0x80, 0xFF].
        //   kind 4:          needs a char >= 0x10000.
        //   kind 2, no surr: needs a char >= 0x100 (dict keys).
        // This lets ASCII and surrogate-bearing UCS2 strings be 100% special.
        bool use_surr = allow_surrogate && kind >= 2;
        usize fill_start = 0;
        if (kind == 1 || kind == 4 || (kind == 2 && !use_surr)) {
            put_char(data, kind, 0, safe_filler(kind, tpl_value(offset)));
            fill_start = 1;
        } else if (use_surr) {
            put_char(data, kind, 0, 0xD800u + (tpl_value(offset) % 0x800u));
            fill_start = 1;
        }
        for (usize i = fill_start; i < length; i++) {
            u32 t = tpl_value(offset + i);
            u32 c;
            if (use_surr && (t & 1u)) {
                c = 0xD800u + (t % 0x800u);
            } else {
                u32 idx = t % 34u; // 0x00..0x1F, '"', '\\'
                c = (idx < 0x20u) ? idx : (idx == 0x20u ? 0x22u : 0x5Cu);
            }
            put_char(data, kind, i, c);
        }
        // Sprinkle a few ordinary chars (may be zero) at byte-chosen positions.
        // safe_filler stays >= the kind's minimum, so overwriting an anchor slot
        // with another ordinary char keeps the string canonical regardless.
        u32 normal_count = bc_choose(st, 5); // 0..4
        for (u32 k = 0; k < normal_count; k++) {
            usize pos = bc_byte(st) % length;
            put_char(data, kind, pos, safe_filler(kind, tpl_value(offset + length + k)));
        }
        // Ordinary-char injection may have clobbered every surrogate; set the
        // flag from the buffer's actual contents.
        if (has_lone_surrogate(data, kind, length)) st->has_surrogate = true;
    }

    return str;
}

// ---------------------------------------------------------------------------
// Scalars.
// ---------------------------------------------------------------------------

static PyObject *maybe_wrap(GenState *st, PyObject *val, PyObject *subtype) {
    if (!val) return NULL;
    if (subtype && bc_choose(st, 8) == 0) {
        PyObject *wrapped = PyObject_CallOneArg(subtype, val);
        Py_DECREF(val);
        return wrapped;
    }
    return val;
}

static PyObject *gen_int(GenState *st) {
    u64 v = bc_u64(st);
    PyObject *val;
    if (bc_choose(st, 2) == 0) {
        val = PyLong_FromLongLong((long long)(i64)v);
    } else {
        val = PyLong_FromUnsignedLongLong((unsigned long long)v);
    }
    return maybe_wrap(st, val, g_int_subclass);
}

static PyObject *gen_float(GenState *st) {
    u64 bits = bc_u64(st);
    double d;
    memcpy(&d, &bits, sizeof(d));
    PyObject *val = PyFloat_FromDouble(d);
    return maybe_wrap(st, val, g_float_subclass);
}

// Fill `buf` of `n` bytes from the template window (compact, deterministic).
static void fill_bytes_from_template(u8 *buf, usize n, usize offset) {
    for (usize i = 0; i < n; i++) { buf[i] = (u8)tpl_value(offset + i); }
}

static PyObject *make_frombuffer_array(const NumpyDType *dt, usize count, usize offset) {
    usize nbytes = count * (usize)dt->itemsize;
    PyObject *payload = PyBytes_FromStringAndSize(NULL, (Py_ssize_t)nbytes);
    if (!payload) return NULL;
    fill_bytes_from_template((u8 *)PyBytes_AS_STRING(payload), nbytes, offset);
    PyObject *arr = PyObject_CallFunctionObjArgs(g_np_frombuffer, payload, dt->dtype, NULL);
    Py_DECREF(payload);
    return arr; // 1-D array (or NULL)
}

static PyObject *gen_numpy_scalar(GenState *st) {
    const NumpyDType *dt = &g_np_dtypes[bc_choose(st, g_np_dtype_count)];
    usize offset = bc_byte(st);
    PyObject *arr = make_frombuffer_array(dt, 1, offset);
    if (!arr) return NULL;
    PyObject *scalar = PySequence_GetItem(arr, 0);
    Py_DECREF(arr);
    return scalar;
}

static PyObject *gen_ndarray(GenState *st) {
    const NumpyDType *dt = &g_np_dtypes[bc_choose(st, g_np_dtype_count)];
    int ndim = (int)bc_choose(st, 4) + 1; // 1..4

    usize shape[4];
    usize total = 1;
    usize remaining = 4096;
    for (int i = 0; i < ndim; i++) {
        usize cap = (i == ndim - 1) ? 64 : 8;
        if (cap > remaining) cap = remaining;
        if (cap < 1) cap = 1;
        usize dim = 1 + ((usize)bc_byte(st)) % cap;
        shape[i] = dim;
        total *= dim;
        remaining = remaining / dim;
        if (remaining < 1) remaining = 1;
    }

    usize offset = bc_byte(st);
    PyObject *arr = make_frombuffer_array(dt, total, offset);
    if (!arr) return NULL;

    PyObject *shape_tuple = PyTuple_New(ndim);
    if (!shape_tuple) {
        Py_DECREF(arr);
        return NULL;
    }
    for (int i = 0; i < ndim; i++) {
        PyObject *d = PyLong_FromSize_t(shape[i]);
        if (!d) {
            Py_DECREF(shape_tuple);
            Py_DECREF(arr);
            return NULL;
        }
        PyTuple_SET_ITEM(shape_tuple, i, d);
    }
    PyObject *reshaped = PyObject_CallMethod(arr, "reshape", "O", shape_tuple);
    Py_DECREF(shape_tuple);
    Py_DECREF(arr);
    return reshaped;
}

// ---------------------------------------------------------------------------
// Recursive object generation.
// ---------------------------------------------------------------------------

static PyObject *gen_object(GenState *st, int depth);

static PyObject *gen_value(GenState *st) {
    u32 num_types = g_has_numpy ? 7 : 5;
    u32 type_id = bc_choose(st, num_types);
    switch (type_id) {
        case TYPE_STR:
            return maybe_wrap(st, gen_str(st, true), g_str_subclass);
        case TYPE_INT:
            return gen_int(st);
        case TYPE_BOOL:
            return Py_NewRef(bc_choose(st, 2) == 0 ? Py_True : Py_False);
        case TYPE_NONE:
            return Py_NewRef(Py_None);
        case TYPE_FLOAT:
            return gen_float(st);
        case TYPE_NDARRAY:
            return gen_ndarray(st);
        case TYPE_NUMPY_VAL:
            return gen_numpy_scalar(st);
        default:
            return Py_NewRef(Py_None);
    }
}

static PyObject *gen_dict(GenState *st, int depth) {
    bool is_subclass = bc_choose(st, 8) == 0;
    PyObject *d = is_subclass ? PyObject_CallNoArgs(g_dict_subclass) : PyDict_New();
    if (!d) return NULL;

    while (bc_has(st)) {
        if (bc_choose(st, 4) == 0) break; // exit container
        PyObject *val = gen_object(st, depth + 1);
        if (!val) goto fail;
        PyObject *key = gen_str(st, false); // keys: no surrogate
        if (!key) {
            Py_DECREF(val);
            goto fail;
        }
        int rc = PyDict_SetItem(d, key, val);
        Py_DECREF(key);
        Py_DECREF(val);
        if (rc != 0) goto fail;
    }
    return d;
fail:
    Py_DECREF(d);
    return NULL;
}

static PyObject *gen_list(GenState *st, int depth) {
    bool is_subclass = bc_choose(st, 8) == 0;
    PyObject *lst = is_subclass ? PyObject_CallNoArgs(g_list_subclass) : PyList_New(0);
    if (!lst) return NULL;

    while (bc_has(st)) {
        if (bc_choose(st, 4) == 0) break;
        PyObject *val = gen_object(st, depth + 1);
        if (!val) goto fail;
        int rc = PyList_Append(lst, val);
        Py_DECREF(val);
        if (rc != 0) goto fail;
    }
    return lst;
fail:
    Py_DECREF(lst);
    return NULL;
}

static PyObject *gen_tuple(GenState *st, int depth) {
    bool is_subclass = bc_choose(st, 8) == 0;
    // Collect items in a list first, then convert.
    PyObject *items = PyList_New(0);
    if (!items) return NULL;

    while (bc_has(st)) {
        if (bc_choose(st, 4) == 0) break;
        PyObject *val = gen_object(st, depth + 1);
        if (!val) {
            Py_DECREF(items);
            return NULL;
        }
        int rc = PyList_Append(items, val);
        Py_DECREF(val);
        if (rc != 0) {
            Py_DECREF(items);
            return NULL;
        }
    }

    PyObject *result;
    if (is_subclass) {
        result = PyObject_CallOneArg(g_tuple_subclass, items);
    } else {
        result = PyList_AsTuple(items);
    }
    Py_DECREF(items);
    return result;
}

static PyObject *gen_container(GenState *st, int depth) {
    switch (bc_choose(st, 3)) {
        case TYPE_DICT:
            return gen_dict(st, depth);
        case TYPE_LIST:
            return gen_list(st, depth);
        default:
            return gen_tuple(st, depth);
    }
}

static PyObject *gen_object(GenState *st, int depth) {
    if (depth >= MAX_DEPTH || !bc_has(st)) { return gen_value(st); }
    if (bc_choose(st, 2) == 0) { return gen_container(st, depth); }
    return gen_value(st);
}

// ---------------------------------------------------------------------------
// Public API.
// ---------------------------------------------------------------------------

static PyObject *fetch_attr(PyObject *module, const char *name) {
    PyObject *attr = PyObject_GetAttrString(module, name);
    return attr; // borrowed-check done by caller; new reference or NULL
}

bool encode_fuzz_gen_init(PyObject *encode_fuzz_module) {
    g_dict_subclass = fetch_attr(encode_fuzz_module, "DictSubclass");
    g_list_subclass = fetch_attr(encode_fuzz_module, "ListSubclass");
    g_tuple_subclass = fetch_attr(encode_fuzz_module, "TupleSubclass");
    g_int_subclass = fetch_attr(encode_fuzz_module, "IntSubclass");
    g_float_subclass = fetch_attr(encode_fuzz_module, "FloatSubclass");
    g_str_subclass = fetch_attr(encode_fuzz_module, "StrSubclass");
    if (!g_dict_subclass || !g_list_subclass || !g_tuple_subclass || !g_int_subclass || !g_float_subclass ||
        !g_str_subclass) {
        return false;
    }

    // numpy is optional.
    PyObject *np = PyImport_ImportModule("numpy");
    if (!np) {
        PyErr_Clear();
        g_has_numpy = false;
        return true;
    }
    g_np_frombuffer = PyObject_GetAttrString(np, "frombuffer");
    if (!g_np_frombuffer) {
        Py_DECREF(np);
        return false;
    }

    static const struct {
        const char *name;
        int itemsize;
    } dtype_table[] = {{"float16", 2}, {"float32", 4}, {"float64", 8}, {"uint8", 1}, {"uint16", 2}, {"uint32", 4},
                       {"uint64", 8},  {"int8", 1},    {"int16", 2},   {"int32", 4}, {"int64", 8},  {"bool_", 1}};

    int n = (int)(sizeof(dtype_table) / sizeof(dtype_table[0]));
    for (int i = 0; i < n; i++) {
        PyObject *dt = PyObject_GetAttrString(np, dtype_table[i].name);
        if (!dt) {
            Py_DECREF(np);
            return false;
        }
        g_np_dtypes[g_np_dtype_count].dtype = dt;
        g_np_dtypes[g_np_dtype_count].itemsize = dtype_table[i].itemsize;
        g_np_dtype_count++;
    }
    Py_DECREF(np);
    g_has_numpy = true;
    return true;
}

PyObject *encode_fuzz_gen_build(const u8 *data, usize size, bool *out_has_surrogate) {
    GenState st = {.data = data, .size = size, .pos = 0, .has_surrogate = false};
    PyObject *obj = gen_object(&st, 0);
    *out_has_surrogate = st.has_surrogate;
    return obj;
}

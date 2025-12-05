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

#define COMPILE_CONTEXT_DECODE

#define XXH_INLINE_ALL
#include "decode_bytes.h"
#include "decode_bytes_root_wrap.h"
#include "decode_shared.h"
#include "decode_str_root_wrap.h"
#include "simd/cvt.h"
#include "simd/mask_table.h"
#include "simd/memcpy.h"
#include "simd/simd_impl.h"
#include "ssrjson.h"
#include "str/ascii.h"
#include "str/ucs.h"
#include "tls.h"

static_assert((SSRJSON_STRING_BUFFER_SIZE % 64) == 0, "(SSRJSON_STRING_BUFFER_SIZE % 64) == 0");


#if SSRJSON_ENABLE_TRACE
Py_ssize_t max_str_len = 0;
int __count_trace[SSRJSON_OP_BITCOUNT_MAX] = {0};
int __hash_trace[SSRJSON_KEY_CACHE_SIZE] = {0};
size_t __hash_hit_counter = 0;
size_t __hash_add_key_call_count = 0;

#    define SSRJSON_TRACE_STR_LEN(_len) max_str_len = max_str_len > _len ? max_str_len : _len
#    define SSRJSON_TRACE_HASH(_hash) \
        __hash_add_key_call_count++;  \
        __hash_trace[_hash & (SSRJSON_KEY_CACHE_SIZE - 1)]++
#    define SSRJSON_TRACE_CACHE_HIT() __hash_hit_counter++
#    define SSRJSON_TRACE_HASH_CONFLICT(_hash) printf("hash conflict: %lld, index=%lld\n", (long long int)_hash, (long long int)(_hash & (SSRJSON_KEY_CACHE_SIZE - 1)))
#else // SSRJSON_ENABLE_TRACE
#    define SSRJSON_TRACE_STR_LEN(_len) (void)(0)
#    define SSRJSON_TRACE_HASH(_hash) (void)(0)
#    define SSRJSON_TRACE_CACHE_HIT() (void)(0)
#    define SSRJSON_TRACE_HASH_CONFLICT(_hash) (void)(0)
#endif // SSRJSON_ENABLE_TRACE

force_inline PyObject *make_string(const u8 *unicode_str, Py_ssize_t len, int kind, bool is_key) {
    SSRJSON_TRACE_STR_LEN(len);
    PyObject *obj;
    decode_keyhash_t hash;
    size_t real_len;
    Py_ssize_t offset;
    int pyunicode_kind;
    bool ascii;

    switch (kind) {
        case SSRJSON_STRING_TYPE_ASCII: {
            ascii = true;
            pyunicode_kind = 1;
            real_len = len;
            offset = sizeof(PyASCIIObject);
            break;
        }
        case SSRJSON_STRING_TYPE_LATIN1: {
            ascii = false;
            pyunicode_kind = 1;
            real_len = len;
            offset = sizeof(PyCompactUnicodeObject);
            break;
        }
        case SSRJSON_STRING_TYPE_UCS2: {
            ascii = false;
            pyunicode_kind = 2;
            real_len = len * 2;
            offset = sizeof(PyCompactUnicodeObject);
            break;
        }
        case SSRJSON_STRING_TYPE_UCS4: {
            ascii = false;
            pyunicode_kind = 4;
            real_len = len * 4;
            offset = sizeof(PyCompactUnicodeObject);
            break;
        }
        default:
            SSRJSON_UNREACHABLE();
    }

    bool should_cache = (is_key && real_len && likely(real_len <= 64));

    if (should_cache) {
        hash = XXH3_64bits(unicode_str, real_len);
        obj = get_key_cache(unicode_str, hash, real_len, kind);
        if (obj) {
            return obj;
        }
    }

    obj = create_empty_unicode(len, kind);
    if (obj == NULL) return NULL;
    ssrjson_memcpy(SSRJSON_CAST(u8 *, obj) + offset, unicode_str, real_len);
    if (should_cache) {
        add_key_cache(hash, obj, real_len, kind);
    }
success:
    if (is_key) {
        PyASCIIObject *ascii_obj = SSRJSON_CAST(PyASCIIObject *, obj);
        if (len) {
            assert(ascii_obj->hash == -1);
            make_hash(ascii_obj, unicode_str, real_len);
        } else {
            // empty unicode has zero hash
            assert(ascii_obj->hash != -1);
        }
    }
    return obj;
}

#if SSRJSON_ENABLE_TRACE
#    define SSRJSON_TRACE_OP(x)                                 \
        do {                                                    \
            for (int i = 0; i < SSRJSON_OP_BITCOUNT_MAX; i++) { \
                if (x & (1 << i)) {                             \
                    __count_trace[i]++;                         \
                    break;                                      \
                }                                               \
            }                                                   \
            __op_counter++;                                     \
        } while (0)
#else
#    define SSRJSON_TRACE_OP(x) (void)0
#endif


bool _decode_obj_stack_resize(
        decode_obj_stack_ptr_t *decode_obj_writer_addr,
        decode_obj_stack_ptr_t *decode_obj_stack_addr,
        decode_obj_stack_ptr_t *decode_obj_stack_end_addr);

force_inline bool push_obj(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                           decode_obj_stack_ptr_t *decode_obj_stack_addr,
                           decode_obj_stack_ptr_t *decode_obj_stack_end_addr, pyobj_ptr_t obj) {
    static_assert(((Py_ssize_t)SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1) > 0, "(SSRJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1) > 0");
    if (unlikely((*decode_obj_writer_addr) >= (*decode_obj_stack_end_addr))) {
        bool c = _decode_obj_stack_resize(decode_obj_writer_addr, decode_obj_stack_addr, decode_obj_stack_end_addr);
        RETURN_ON_UNLIKELY_ERR(!c);
    }
    *(*decode_obj_writer_addr)++ = obj;
    return true;
}

force_inline bool decode_arr(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                             decode_obj_stack_ptr_t *decode_obj_stack_addr,
                             decode_obj_stack_ptr_t *decode_obj_stack_end_addr, usize arr_len) {
    assert(arr_len >= 0);
    PyObject *list = PyList_New(arr_len);
    RETURN_ON_UNLIKELY_ERR(!list);
    decode_obj_stack_ptr_t list_val_start = (*decode_obj_writer_addr) - arr_len;
    assert(list_val_start >= (*decode_obj_stack_addr));
    for (usize j = 0; j < arr_len; j++) {
        PyObject *val = list_val_start[j];
        assert(val);
        PyList_SET_ITEM(list, j, val); // this never fails
    }
    (*decode_obj_writer_addr) -= arr_len;
    return push_obj(decode_obj_writer_addr, decode_obj_stack_addr, decode_obj_stack_end_addr, list);
}

force_inline bool decode_obj(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                             decode_obj_stack_ptr_t *decode_obj_stack_addr,
                             decode_obj_stack_ptr_t *decode_obj_stack_end_addr, usize dict_len) {
    PyObject *dict = _PyDict_NewPresized((Py_ssize_t)dict_len);
    RETURN_ON_UNLIKELY_ERR(!dict);
    decode_obj_stack_ptr_t dict_val_start = (*decode_obj_writer_addr) - dict_len * 2;
    decode_obj_stack_ptr_t dict_val_view = dict_val_start;
    for (usize j = 0; j < dict_len; j++) {
        PyObject *key = *dict_val_view++;
        assert(PyUnicode_Check(key));
        PyObject *val = *dict_val_view++;
        assert(((PyASCIIObject *)key)->hash != -1);
        Py_ssize_t key_refcnt = key->ob_refcnt;
        int retcode = _PyDict_SetItem_KnownHash(dict, key, val, ((PyASCIIObject *)key)->hash); // this may fail
        if (likely(0 == retcode)) {
            assert(key->ob_refcnt >= key_refcnt);
            key->ob_refcnt = key_refcnt; // restore refcnt, as the key may be already in the dict
            Py_DecRef_NoCheck(val);
        } else {
            // we already decrefed some objects, have to manually handle all refcnt here
            Py_DECREF(dict);
            // also need to clean up the rest k-v pairs
            for (usize k = j * 2; k < dict_len * 2; k++) {
                Py_DECREF(dict_val_start[k]);
            }
            // move decode_obj_writer to the first key addr, avoid double decref
            (*decode_obj_writer_addr) = dict_val_start;
            return false;
        }
    }
    (*decode_obj_writer_addr) -= dict_len * 2;
    return push_obj(decode_obj_writer_addr, decode_obj_stack_addr, decode_obj_stack_end_addr, dict);
}

force_inline bool decode_null(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                              decode_obj_stack_ptr_t *decode_obj_stack_addr,
                              decode_obj_stack_ptr_t *decode_obj_stack_end_addr) {
    SSRJSON_TRACE_OP(SSRJSON_OP_CONSTANTS);
    Py_Immortal_IncRef(Py_None);
    return push_obj(decode_obj_writer_addr, decode_obj_stack_addr, decode_obj_stack_end_addr, Py_None);
}

force_inline bool decode_false(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                               decode_obj_stack_ptr_t *decode_obj_stack_addr,
                               decode_obj_stack_ptr_t *decode_obj_stack_end_addr) {
    SSRJSON_TRACE_OP(SSRJSON_OP_CONSTANTS);
    Py_Immortal_IncRef(Py_False);
    return push_obj(decode_obj_writer_addr, decode_obj_stack_addr, decode_obj_stack_end_addr, Py_False);
}

force_inline bool decode_true(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                              decode_obj_stack_ptr_t *decode_obj_stack_addr,
                              decode_obj_stack_ptr_t *decode_obj_stack_end_addr) {
    SSRJSON_TRACE_OP(SSRJSON_OP_CONSTANTS);
    Py_Immortal_IncRef(Py_True);
    return push_obj(decode_obj_writer_addr, decode_obj_stack_addr, decode_obj_stack_end_addr, Py_True);
}

force_inline bool decode_nan(decode_obj_stack_ptr_t *decode_obj_writer_addr,
                             decode_obj_stack_ptr_t *decode_obj_stack_addr,
                             decode_obj_stack_ptr_t *decode_obj_stack_end_addr, bool is_signed) {
    SSRJSON_TRACE_OP(SSRJSON_OP_NAN_INF);
    PyObject *o = PyFloat_FromDouble(is_signed ? -fabs(Py_NAN) : fabs(Py_NAN));
    RETURN_ON_UNLIKELY_ERR(!o);
    return push_obj(decode_obj_writer_addr, decode_obj_stack_addr, decode_obj_stack_end_addr, o);
}

extern int ssrjson_invalid_arg_checked;
extern int ssrjson_nonstrict_argparse;

force_inline bool decode_argparse_with_kw(PyObject *const *args, usize npargs, PyObject *kwnames, PyObject **s_out) {
    assert(kwnames);
    PyObject *s;
    //
    const bool nonstrict_argparse = ssrjson_nonstrict_argparse;
    bool invalid_arg_checked = ssrjson_invalid_arg_checked;
    //
    usize nkwargs = PyTuple_GET_SIZE(kwnames);
    usize nargs = npargs + nkwargs;
    assert(nkwargs <= nargs);
    //
    s = npargs ? args[0] : NULL;
    //
    const char *func_name = "loads";
    const char *_s_str = "s";
    const usize _s_str_len = strlen(_s_str);
    //
    if (unlikely(npargs > 1)) {
        PyErr_Format(PyExc_TypeError, "loads() takes 1 positional argument but %d were given", (int)npargs);
        return false;
    }
    for (usize i = 0; i < nkwargs; i++) {
        PyObject *kwname = PyTuple_GET_ITEM(kwnames, i);
        assert(PyUnicode_Check(kwname));
        bool is_ascii;
        const u8 *char_data;
        usize char_count;
        parse_ascii(kwname, &is_ascii, &char_data, &char_count);
        if (likely(is_ascii)) {
            if (char_count == _s_str_len && memcmp(char_data, _s_str, _s_str_len) == 0) {
                if (unlikely(s)) {
                    // repeated arg
                    PyErr_Format(PyExc_TypeError, "%s() got multiple values for argument '%s'", func_name, _s_str);
                    return false;
                }
                s = args[npargs + i];
                continue;
            }
        }
        // unknown argument
        if (!nonstrict_argparse) {
            handle_unexpected_kw(func_name, kwname);
            return false;
        }
        if (!invalid_arg_checked) {
            fprintf(stderr, "Warning: some options are not supported in this version of ssrjson\n");
            ssrjson_invalid_arg_checked = 1;
            invalid_arg_checked = true;
        }
    }
    //
    if (unlikely(!s)) {
        PyErr_Format(PyExc_TypeError, "%s() missing 1 required positional argument: '%s'", func_name, _s_str);
        return false;
    }
    *s_out = s;
    return true;
}

PyObject *SIMD_NAME_MODIFIER(ssrjson_Decode)(PyObject *self, PyObject *const *args, Py_ssize_t nargsf, PyObject *kwnames) {
    PyObject *ret;
    PyObject *s;
    //
    usize npargs = PyVectorcall_NARGS(nargsf);
    //
    if (!kwnames) {
        // positional-only args except `s' are not allowed even in nonstrict mode
        if (unlikely(npargs != 1)) {
            if (npargs > 1) {
                PyErr_Format(PyExc_TypeError, "loads() takes 1 positional argument but %d were given", (int)npargs);
            } else {
                PyErr_SetString(PyExc_TypeError, "loads() missing 1 required positional argument: 's'");
            }
            return NULL;
        }
        s = args[0];
    } else if (!decode_argparse_with_kw(args, npargs, kwnames, &s)) {
        return NULL;
    }
    //
    if (PyUnicode_Check(s)) {
        PyASCIIObject *ascii_head = SSRJSON_CAST(PyASCIIObject *, s);
        PyUnicodeObject *in_unicode = SSRJSON_CAST(PyUnicodeObject *, s);
        int pyunicode_kind = ascii_head->state.ascii ? 0 : ascii_head->state.kind;
        switch (pyunicode_kind) {
            case SSRJSON_STRING_TYPE_ASCII: {
                ret = decode_ascii(in_unicode);
                break;
            }
            case SSRJSON_STRING_TYPE_LATIN1: {
                ret = decode_ucs1(in_unicode);
                break;
            }
            case SSRJSON_STRING_TYPE_UCS2: {
                ret = decode_ucs2(in_unicode);
                break;
            }
            case SSRJSON_STRING_TYPE_UCS4: {
                ret = decode_ucs4(in_unicode);
                break;
            }
            default: {
                ret = NULL;
                SSRJSON_UNREACHABLE();
            }
        }
        goto done;
    }
    //
    if (PyBytes_Check(s)) {
        char *buffer;
        Py_ssize_t length;
        if (unlikely(0 != PyBytes_AsStringAndSize(s, &buffer, &length))) {
            ret = NULL;
            goto done;
        }
        ret = ssrjson_decode_bytes(buffer, length);
        goto done;
    }
    //
    if (PyByteArray_Check(s)) {
        char *buffer = PyByteArray_AS_STRING(s);
        Py_ssize_t length = PyByteArray_GET_SIZE(s);
        ret = ssrjson_decode_bytes(buffer, length);
        goto done;
    }

fail:;
    ret = NULL;
    PyErr_SetString(PyExc_TypeError, "Invalid argument");

done:;
    if (unlikely(!ret && !PyErr_Occurred())) {
        PyErr_SetString(JSONDecodeError, "Failed to decode JSON: unknown error");
    }
    return ret;
}

static_assert(SSRJSON_EXPORTS, "SSRJSON_EXPORTS=1");

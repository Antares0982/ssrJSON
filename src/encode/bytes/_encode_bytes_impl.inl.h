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
#    include "encode/encode_impl_wrap.h"
#    include "encode/encode_shared.h"
#    include "encode_utf8.h"
#    include "ssrjson.h"
#    include "tls.h"
#    include "utils/unicode.h"
//
#    ifndef COMPILE_INDENT_LEVEL
#        define COMPILE_INDENT_LEVEL 2
#    endif
#endif

// use writer: ascii
#define COMPILE_UCS_LEVEL 0
#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
//
#include "compile_context/sirw_in.inl.h"

force_inline bool bytes_buffer_append_key(PyObject *key, EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    int kind, write_kind;
    int read_kind_val = PyUnicode_KIND(key);
    bool is_ascii = PyUnicode_IS_ASCII(key);
    usize len = PyUnicode_GET_LENGTH(key);
    //
    RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(&writer_addr->writer_u8, unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 5 + 6 * len + TAIL_PADDING));
    write_unicode_indent(&writer_addr->writer_u8, cur_nested_depth);
    *writer_addr->writer_u8++ = '"';
    if (is_ascii) {
        bytes_write_ascii(&writer_addr->writer_u8, SSRJSON_CAST(const u8 *, SSRJSON_CAST(PyASCIIObject *, key) + 1), len, true);
    } else {
        switch (read_kind_val) {
            case 1: {
                bytes_write_ucs1(&writer_addr->writer_u8, SSRJSON_CAST(const u8 *, SSRJSON_CAST(PyCompactUnicodeObject *, key) + 1), len);
                break;
            }
            case 2: {
                if (unlikely(!bytes_write_ucs2(&writer_addr->writer_u8, SSRJSON_CAST(const u16 *, SSRJSON_CAST(PyCompactUnicodeObject *, key) + 1), len))) return false;
                break;
            }
            case 4: {
                if (unlikely(!bytes_write_ucs4(&writer_addr->writer_u8, SSRJSON_CAST(const u32 *, SSRJSON_CAST(PyCompactUnicodeObject *, key) + 1), len))) return false;
                break;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
    }
    *writer_addr->writer_u8++ = '"';
    *writer_addr->writer_u8++ = ':';
#if COMPILE_INDENT_LEVEL > 0
    *writer_addr->writer_u8++ = ' ';
    *writer_addr->writer_u8 = 0;
#endif // COMPILE_INDENT_LEVEL > 0
    return true;
}

force_inline bool bytes_buffer_append_str(PyObject *str, EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    int kind, write_kind;
    int read_kind_val = PyUnicode_KIND(str);
    bool is_ascii = PyUnicode_IS_ASCII(str);
    usize len = PyUnicode_GET_LENGTH(str);
    //
    if (is_in_obj) {
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(&writer_addr->writer_u8, unicode_buffer_info, 3 + 6 * len + TAIL_PADDING));
    } else {
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(&writer_addr->writer_u8, unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 3 + 6 * len + TAIL_PADDING));
        write_unicode_indent(&writer_addr->writer_u8, cur_nested_depth);
    }
    *writer_addr->writer_u8++ = '"';
    if (is_ascii) {
        bytes_write_ascii_not_key(&writer_addr->writer_u8, SSRJSON_CAST(const u8 *, SSRJSON_CAST(PyASCIIObject *, str) + 1), len);
    } else {
        switch (read_kind_val) {
            case 1: {
                bytes_write_ucs1(&writer_addr->writer_u8, SSRJSON_CAST(const u8 *, SSRJSON_CAST(PyCompactUnicodeObject *, str) + 1), len);
                break;
            }
            case 2: {
                if (unlikely(!bytes_write_ucs2(&writer_addr->writer_u8, SSRJSON_CAST(const u16 *, SSRJSON_CAST(PyCompactUnicodeObject *, str) + 1), len))) return false;
                break;
            }
            case 4: {
                if (unlikely(!bytes_write_ucs4(&writer_addr->writer_u8, SSRJSON_CAST(const u32 *, SSRJSON_CAST(PyCompactUnicodeObject *, str) + 1), len))) return false;
                break;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
    }
    *writer_addr->writer_u8++ = '"';
    *writer_addr->writer_u8++ = ',';
    return true;
}

force_inline EncodeValJumpFlag encode_bytes_process_val(
        EncodeUnicodeWriter *writer_addr,
        EncodeUnicodeBufferInfo *unicode_buffer_info, PyObject *val,
        PyObject **cur_obj_addr,
        Py_ssize_t *cur_pos_addr,
        Py_ssize_t *cur_nested_depth_addr,
        Py_ssize_t *cur_list_size_addr,
        EncodeCtnWithIndex *ctn_stack,
        // EncodeUnicodeInfo *unicode_info_addr,
        bool is_in_obj) {
#define CTN_SIZE_GROW()                                                         \
    do {                                                                        \
        if (unlikely(*cur_nested_depth_addr == SSRJSON_ENCODE_MAX_RECURSION)) { \
            PyErr_SetString(JSONEncodeError, "Too many nested structures");     \
            return JumpFlag_Fail;                                               \
        }                                                                       \
    } while (0)
#define RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(_cond_) \
    do {                                         \
        if (unlikely(_cond_)) {                  \
            return JumpFlag_Fail;                \
        }                                        \
    } while (0)

    ssrjson_py_types obj_type = ssrjson_type_check(val);

    switch (obj_type) {
        case T_Unicode: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!bytes_buffer_append_str(val, writer_addr, unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            break;
        }
        case T_Long: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_long(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, val, is_in_obj));
            break;
        }
        case T_Bool: {
            if (val == Py_False) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_false(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            } else {
                assert(val == Py_True);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_true(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            }
            break;
        }
        case T_None: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_null(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            break;
        }
        case T_Float: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_float(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, val, is_in_obj));
            break;
        }
        case T_List: {
            Py_ssize_t this_list_size = PyList_GET_SIZE(val);
            if (unlikely(this_list_size == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_empty_arr(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_arr_begin(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = ctn_stack + ((*cur_nested_depth_addr)++);
                cur_write_ctn->ctn = *cur_obj_addr;
                cur_write_ctn->index = *cur_pos_addr;
                *cur_obj_addr = val;
                *cur_pos_addr = 0;
                *cur_list_size_addr = this_list_size;
                return JumpFlag_ArrValBegin;
            }
            break;
        }
        case T_Dict: {
            if (unlikely(PyDict_GET_SIZE(val) == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_empty_obj(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_obj_begin(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = ctn_stack + ((*cur_nested_depth_addr)++);
                cur_write_ctn->ctn = *cur_obj_addr;
                cur_write_ctn->index = *cur_pos_addr;
                *cur_obj_addr = val;
                *cur_pos_addr = 0;
                return JumpFlag_DictPairBegin;
            }
            break;
        }
        case T_Tuple: {
            Py_ssize_t this_list_size = PyTuple_Size(val);
            if (unlikely(this_list_size == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_empty_arr(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_arr_begin(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = ctn_stack + ((*cur_nested_depth_addr)++);
                cur_write_ctn->ctn = *cur_obj_addr;
                cur_write_ctn->index = *cur_pos_addr;
                *cur_obj_addr = val;
                *cur_pos_addr = 0;
                *cur_list_size_addr = this_list_size;
                return JumpFlag_TupleValBegin;
            }
            break;
        }
        default: {
            PyErr_SetString(JSONEncodeError, "Unsupported type to encode");
            return JumpFlag_Fail;
        }
    }

    return JumpFlag_Default;
#undef RETURN_JUMP_FAIL_ON_UNLIKELY_ERR
#undef CTN_SIZE_GROW
}

static force_noinline PyObject *
ssrjson_dumps_to_bytes_obj(PyObject *in_obj) {
#define GOTO_FAIL_ON_UNLIKELY_ERR(_condition) \
    do {                                      \
        if (unlikely(_condition)) {           \
            goto fail;                        \
        }                                     \
    } while (0)

    EncodeUnicodeWriter writer;
    EncodeUnicodeBufferInfo _unicode_buffer_info;
    PyObject *key, *val;
    PyObject *cur_obj = in_obj;
    Py_ssize_t cur_pos = 0;
    Py_ssize_t cur_nested_depth = 0;
    Py_ssize_t cur_list_size;
    // alias thread local buffer
    EncodeCtnWithIndex *ctn_stack;
    // EncodeUnicodeInfo unicode_info;
    bool cur_is_tuple;
    // memset(&unicode_info, 0, sizeof(unicode_info));
    //
    GOTO_FAIL_ON_UNLIKELY_ERR(!init_bytes_buffer(&writer, &_unicode_buffer_info) || !init_encode_ctn_stack(&ctn_stack));

    // this is the starting, we don't need an indent before container.
    // so is_in_obj always pass true
    if (PyDict_Check(cur_obj)) {
        if (unlikely(PyDict_GET_SIZE(cur_obj) == 0)) {
            bool _c = unicode_buffer_append_empty_obj(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = unicode_buffer_append_obj_begin(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
        }
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        goto dict_pair_begin;
    } else if (PyList_Check(cur_obj)) {
        cur_list_size = PyList_GET_SIZE(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            bool _c = unicode_buffer_append_empty_arr(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = unicode_buffer_append_arr_begin(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
        }
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        cur_is_tuple = false;
        goto arr_val_begin;
    } else {
        if (unlikely(!PyTuple_Check(cur_obj))) {
            goto fail_ctntype;
        }
        cur_list_size = PyTuple_GET_SIZE(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            bool _c = unicode_buffer_append_empty_arr(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = unicode_buffer_append_arr_begin(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
        }
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        cur_is_tuple = true;
        goto arr_val_begin;
    }

    SSRJSON_UNREACHABLE();


dict_pair_begin:;
    assert(PyDict_GET_SIZE(cur_obj) != 0);
    if (pydict_next(cur_obj, &cur_pos, &key, &val)) {
        if (unlikely(!PyUnicode_CheckExact(key))) {
            goto fail_keytype;
        }
        GOTO_FAIL_ON_UNLIKELY_ERR(!bytes_buffer_append_key(key, &writer, &_unicode_buffer_info, cur_nested_depth));
    dict_key_done:;
        //
        EncodeValJumpFlag jump_flag = encode_bytes_process_val(&writer, &_unicode_buffer_info, val, &cur_obj, &cur_pos, &cur_nested_depth, &cur_list_size, ctn_stack, true);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                break;
            }
            case JumpFlag_ArrValBegin: {
                cur_is_tuple = false;
                goto arr_val_begin;
            }
            case JumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case JumpFlag_TupleValBegin: {
                cur_is_tuple = true;
                goto arr_val_begin;
            }
            case JumpFlag_Fail: {
                goto fail;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
        goto dict_pair_begin;
    } else {
        // dict end
        assert(cur_nested_depth);
        EncodeCtnWithIndex *last_pos = ctn_stack + (--cur_nested_depth);

        GOTO_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_obj_end(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth));
        if (unlikely(cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        cur_obj = last_pos->ctn;
        cur_pos = last_pos->index;

        if (PyDict_CheckExact(cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(cur_obj)) {
            cur_list_size = PyList_GET_SIZE(cur_obj);
            cur_is_tuple = false;
            goto arr_val_begin;
        } else {
            assert(PyTuple_CheckExact(cur_obj));
            cur_list_size = PyTuple_GET_SIZE(cur_obj);
            cur_is_tuple = true;
            goto arr_val_begin;
        }
    }

    SSRJSON_UNREACHABLE();

arr_val_begin:;
    assert(cur_list_size != 0);

    if (cur_pos < cur_list_size) {
        if (likely(!cur_is_tuple)) {
            val = PyList_GET_ITEM(cur_obj, cur_pos);
        } else {
            val = PyTuple_GET_ITEM(cur_obj, cur_pos);
        }
        cur_pos++;
        //
        EncodeValJumpFlag jump_flag = encode_bytes_process_val(&writer, &_unicode_buffer_info, val, &cur_obj, &cur_pos, &cur_nested_depth, &cur_list_size, ctn_stack, false);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                break;
            }
            case JumpFlag_ArrValBegin: {
                cur_is_tuple = false;
                goto arr_val_begin;
            }
            case JumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case JumpFlag_TupleValBegin: {
                cur_is_tuple = true;
                goto arr_val_begin;
            }
            case JumpFlag_Fail: {
                goto fail;
            }
            default: {
                SSRJSON_UNREACHABLE();
            }
        }
        //
        goto arr_val_begin;
    } else {
        // list end
        assert(cur_nested_depth);
        EncodeCtnWithIndex *last_pos = ctn_stack + (--cur_nested_depth);

        GOTO_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_arr_end(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth));
        if (unlikely(cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        cur_obj = last_pos->ctn;
        cur_pos = last_pos->index;

        if (PyDict_CheckExact(cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(cur_obj)) {
            cur_list_size = PyList_GET_SIZE(cur_obj);
            cur_is_tuple = false;
            goto arr_val_begin;
        } else {
            assert(PyTuple_CheckExact(cur_obj));
            cur_list_size = PyTuple_GET_SIZE(cur_obj);
            cur_is_tuple = true;
            goto arr_val_begin;
        }
    }
    SSRJSON_UNREACHABLE();

success:;
    assert(cur_nested_depth == 0);
    // remove trailing comma
    (_WRITER(&writer))--;
    usize final_len = get_bytes_buffer_final_len(writer.writer_u8, _unicode_buffer_info.head);
    GOTO_FAIL_ON_UNLIKELY_ERR(!resize_to_fit_pybytes(&_unicode_buffer_info, final_len));
    init_pybytes(_unicode_buffer_info.head, final_len);
    return (PyObject *)_unicode_buffer_info.head;
fail:;
    if (_unicode_buffer_info.head) {
        PyObject_Free(_unicode_buffer_info.head);
    }
    return NULL;
fail_ctntype:;
    PyErr_SetString(JSONEncodeError, "Unsupported type to encode");
    goto fail;
fail_keytype:;
    PyErr_SetString(JSONEncodeError, "Expected `str` as key");
    goto fail;
#undef GOTO_FAIL_ON_UNLIKELY_ERR
}

#include "compile_context/sirw_out.inl.h"
#undef COMPILE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_WRITE_UCS_LEVEL

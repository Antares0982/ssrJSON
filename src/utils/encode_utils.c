#include "encode/encode_shared.h"


#define RESERVE_MAX ((~(usize)PY_SSIZE_T_MAX) >> 1)
static_assert((SSRJSON_CAST(usize, RESERVE_MAX) & (SSRJSON_CAST(usize, RESERVE_MAX) - 1)) == 0, "");

bool _unicode_buffer_reserve(EncodeUnicodeBufferInfo *unicode_buffer_info, usize target_size) {
    usize u8len = SSRJSON_CAST(uintptr_t, unicode_buffer_info->end) - SSRJSON_CAST(uintptr_t, unicode_buffer_info->head);
    assert((u8len & (u8len - 1)) == 0);
    while (target_size > u8len) {
        if (u8len & RESERVE_MAX) {
            PyErr_NoMemory();
            return false;
        }
        u8len = (u8len << 1);
    }
    void *new_ptr = PyObject_Realloc(unicode_buffer_info->head, u8len);
    if (unlikely(!new_ptr)) {
        assert(PyErr_Occurred());
        return false;
    }
    unicode_buffer_info->head = new_ptr;
    unicode_buffer_info->end = SSRJSON_CAST(u8 *, unicode_buffer_info->head) + u8len;
    return true;
}

bool resize_to_fit_pyunicode(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t len, int ucs_type) {
    Py_ssize_t char_size = ucs_type ? ucs_type : 1;
    Py_ssize_t struct_size = ucs_type ? sizeof(PyCompactUnicodeObject) : sizeof(PyASCIIObject);
    assert(len <= ((PY_SSIZE_T_MAX - struct_size) / char_size - 1));
    // Resizes to a smaller size. It *should* always be successful
    void *new_ptr = PyObject_Realloc(unicode_buffer_info->head, struct_size + (len + 1) * char_size);
    if (unlikely(!new_ptr)) {
        return false;
    }
    unicode_buffer_info->head = new_ptr;
    return true;
}

#if !defined(Py_GIL_DISABLED)
EncodeCtnWithIndex _EncodeCtnBuffer[SSRJSON_ENCODE_MAX_RECURSION];
#endif

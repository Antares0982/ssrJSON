#ifdef SSRJSON_CLANGD_DUMMY
#    include "encode/encode_shared.h"
#    include "utils/unicode.h"
#endif

#include "compile_context/iw_in.inl.h"

force_inline void write_unicode_indent(_dst_t **writer_addr, Py_ssize_t _cur_nested_depth) {
#if COMPILE_INDENT_LEVEL > 0
    _dst_t *writer = *writer_addr;
    *writer++ = '\n';
    usize cur_nested_depth = (usize)_cur_nested_depth;
    for (usize i = 0; i < cur_nested_depth; i++) {
        *writer++ = ' ';
        *writer++ = ' ';
#    if COMPILE_INDENT_LEVEL == 4
        *writer++ = ' ';
        *writer++ = ' ';
#    endif // COMPILE_INDENT_LEVEL == 4
    }
    *writer_addr = writer;
#endif // COMPILE_INDENT_LEVEL > 0
}

// forward declaration
force_inline bool unicode_buffer_reserve(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, usize size);

force_inline bool unicode_indent_writer(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj, Py_ssize_t additional_reserve_count) {
    if (!is_in_obj && COMPILE_INDENT_LEVEL != 0) {
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + additional_reserve_count));
        write_unicode_indent(writer_addr, cur_nested_depth);
    } else {
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, additional_reserve_count));
    }
    return true;
}

#include "compile_context/iw_out.inl.h"

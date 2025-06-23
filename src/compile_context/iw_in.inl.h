#ifndef SSRJSON_COMPILE_CONTEXT_IW
#define SSRJSON_COMPILE_CONTEXT_IW

#include "w_in.inl.h"

// fake include and definition to deceive clangd
#ifdef SSRJSON_CLANGD_DUMMY
#    include "ssrjson.h"
#    ifndef COMPILE_INDENT_LEVEL
#        define COMPILE_INDENT_LEVEL 2
#    endif
#endif

/*
 * Basic definitions.
 */
#if COMPILE_INDENT_LEVEL == 4
#elif COMPILE_INDENT_LEVEL == 2
#elif COMPILE_INDENT_LEVEL == 0
#else
#    error "COMPILE_INDENT_LEVEL must be 0, 2 or 4"
#endif

#define __INDENT_NAME SSRJSON_SIMPLE_CONCAT2(indent, COMPILE_INDENT_LEVEL)

#define MAKE_I_NAME(_x_) SSRJSON_CONCAT2(_x_, __INDENT_NAME)
#define MAKE_IW_NAME(_x_) SSRJSON_CONCAT3(_x_, __INDENT_NAME, _dst_t)

/*
 * Write indents to unicode buffer. Need to reserve space before calling this function.
 */
#define write_unicode_indent MAKE_IW_NAME(write_unicode_indent)

/*
 * Write indents to unicode buffer. Will reserve space if needed.
 */
#define unicode_indent_writer MAKE_IW_NAME(unicode_indent_writer)

#define bytes_buffer_append_key MAKE_I_NAME(bytes_buffer_append_key)
#define bytes_buffer_append_str MAKE_I_NAME(bytes_buffer_append_str)
#define encode_bytes_process_val MAKE_I_NAME(encode_bytes_process_val)
#define ssrjson_dumps_to_bytes_obj MAKE_I_NAME(ssrjson_dumps_to_bytes_obj)

#endif // SSRJSON_COMPILE_CONTEXT_IW

#ifndef SSRJSON_COMPILE_CONTEXT_W
#define SSRJSON_COMPILE_CONTEXT_W

// fake include and definition to deceive clangd
#ifdef SSRJSON_CLANGD_DUMMY
#    include "ssrjson.h"
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif
#endif

/*
 * Basic definitions.
 */
#if COMPILE_WRITE_UCS_LEVEL == 4
#    define WRITE_BIT_SIZE 32
#    define _WRITER U32_WRITER
#elif COMPILE_WRITE_UCS_LEVEL == 2
#    define WRITE_BIT_SIZE 16
#    define _WRITER U16_WRITER
#elif COMPILE_WRITE_UCS_LEVEL == 1
#    define WRITE_BIT_SIZE 8
#    define _WRITER U8_WRITER
#else
#    error "COMPILE_WRITE_UCS_LEVEL must be 1, 2 or 4"
#endif

// The destination type.
#define _dst_t SSRJSON_SIMPLE_CONCAT2(u, WRITE_BIT_SIZE)

// Name creation macro.
#define MAKE_W_NAME(_x_) SSRJSON_CONCAT2(_x_, _dst_t)

/*
 * Names using W context.
 */
#define unicode_buffer_reserve MAKE_W_NAME(unicode_buffer_reserve)
#define u64_to_unicode MAKE_W_NAME(u64_to_unicode)
#define f64_to_unicode MAKE_W_NAME(f64_to_unicode)
#define ControlEscapeTable MAKE_W_NAME(ControlEscapeTable)

#endif // SSRJSON_COMPILE_CONTEXT_W

#ifndef SSRJSON_COMPILE_CONTEXT_RW
#define SSRJSON_COMPILE_CONTEXT_RW

// Include sub contexts.
#include "r_in.inl.h"
#include "w_in.inl.h"


// Name creation macro.
#define MAKE_RW_NAME(_x_) SSRJSON_CONCAT3(_x_, _src_t, _dst_t)

#ifdef COMPILE_UCS_LEVEL
#    define MAKE_UCS_W_NAME(_x_) MAKE_W_NAME(MAKE_UCS_NAME(_x_))
// some decoder impls
#    define decode_str_copy_loop4 MAKE_UCS_W_NAME(decode_str_copy_loop4)
#    define decode_str_copy_loop MAKE_UCS_W_NAME(decode_str_copy_loop)
#    define decode_str_copy_trailing MAKE_UCS_W_NAME(decode_str_copy_trailing)
#    define process_escape MAKE_UCS_W_NAME(process_escape)
#endif

#endif // SSRJSON_COMPILE_CONTEXT_RW

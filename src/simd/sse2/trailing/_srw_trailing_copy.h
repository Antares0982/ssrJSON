#ifdef SSRJSON_CLANGD_DUMMY
#    include "simd/sse2/checker.h"
#    include "simd/sse2/common.h"
#    include "simd/sse2/cvt.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif
#endif
//
#define COMPILE_SIMD_BITS 128

#include "compile_context/srw_in.inl.h"
extern const Py_ssize_t _ControlJump[_Slash + 1];
extern const _dst_t ControlEscapeTable[(_Slash + 1) * 8];

force_inline void trailing_copy_with_cvt(_dst_t **dst_addr, const _src_t *src, usize copy_len) {
    _dst_t *dst = *dst_addr;
    assert(copy_len * sizeof(_src_t) < 16);
    const _src_t *const load_start = src + copy_len - 16 / sizeof(_src_t);
    const vector_a vec = *(vector_u *)load_start;
    vector_a vec_shifted = runtime_byte_rshift_128(vec, 16 - copy_len * sizeof(_src_t));
    cvt_to_dst(dst, vec_shifted);
    dst += copy_len;
    *dst_addr = dst;
}

#undef COMPILE_SIMD_BITS
#include "compile_context/srw_out.inl.h"

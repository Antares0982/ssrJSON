#ifdef SSRJSON_CLANGD_DUMMY
#    include "simd/avx512f_cd/common.h"
#    include "simd/avx512vl_dq_bw/checker.h"
#    include "simd/avx512vl_dq_bw/common.h"
#    include "simd/avx512vl_dq_bw/cvt.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif
#endif
//
#define COMPILE_SIMD_BITS 512

#include "compile_context/srw_in.inl.h"

extern const _dst_t ControlEscapeTable[(_Slash + 1) * 8];
extern const Py_ssize_t _ControlJump[_Slash + 1];

force_inline void trailing_copy_with_cvt(_dst_t **dst_addr, const _src_t *src, usize len) {
    _dst_t *dst = *dst_addr;
    vector_a vec;
    usize maskz = len_to_maskz(len);
    vec = maskz_loadu(maskz, src);
    cvt_to_dst(dst, vec);
    dst += len;
    *dst_addr = dst;
}

#undef COMPILE_SIMD_BITS
#include "compile_context/srw_out.inl.h"

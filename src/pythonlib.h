#ifndef SSRJSON_PYTHONLIB_H
#define SSRJSON_PYTHONLIB_H

#include "ssrjson.h"

#if BUILD_MULTI_LIB
#    if SSRJSON_X86
#        define DECLARE_MULTILIB_PYFUNCTION(_func_name_)                                                               \
            PyObject *SSRJSON_CONCAT2(_func_name_, avx512)(PyObject * self, PyObject * args, PyObject * kwargs);       \
            PyObject *SSRJSON_CONCAT2(_func_name_, avx2)(PyObject * self, PyObject * args, PyObject * kwargs);         \
            PyObject *SSRJSON_CONCAT2(_func_name_, sse4_2)(PyObject * self, PyObject * args, PyObject * kwargs);       \
            typedef PyObject *(*SSRJSON_CONCAT2(_func_name_, t))(PyObject * self, PyObject * args, PyObject * kwargs); \
            extern SSRJSON_CONCAT2(_func_name_, t) SSRJSON_CONCAT2(_func_name_, interface);


#        define DECLARE_MULTILIB_ANYFUNCTION(_func_name_, _ret_type_, ...)      \
            _ret_type_ SSRJSON_CONCAT2(_func_name_, avx512)(__VA_ARGS__);       \
            _ret_type_ SSRJSON_CONCAT2(_func_name_, avx2)(__VA_ARGS__);         \
            _ret_type_ SSRJSON_CONCAT2(_func_name_, sse4_2)(__VA_ARGS__);       \
            typedef _ret_type_ (*SSRJSON_CONCAT2(_func_name_, t))(__VA_ARGS__); \
            extern SSRJSON_CONCAT2(_func_name_, t) SSRJSON_CONCAT2(_func_name_, interface);

#        define IMPL_MULTILIB_FUNCTION_INTERFACE(_func_name_) SSRJSON_CONCAT2(_func_name_, t) SSRJSON_CONCAT2(_func_name_, interface);

#        define MAKE_FORWARD_PYFUNCTION_IMPL(_func_name_)                             \
            PyObject *_func_name_(PyObject *self, PyObject *args, PyObject *kwargs) { \
                _update_simd_features();                                              \
                assert(SSRJSON_CONCAT2(_func_name_, interface));                      \
                return SSRJSON_CONCAT2(_func_name_, interface)(self, args, kwargs);   \
            }

#        define SET_INTERFACE(_func_name_, _feature_name_) SSRJSON_CONCAT2(_func_name_, interface) = SSRJSON_CONCAT2(_func_name_, _feature_name_)

DECLARE_MULTILIB_PYFUNCTION(ssrjson_Encode)
DECLARE_MULTILIB_PYFUNCTION(ssrjson_Decode)
DECLARE_MULTILIB_PYFUNCTION(ssrjson_EncodeToBytes)
DECLARE_MULTILIB_ANYFUNCTION(long_cvt_noinline_u16_u32, void, u32 *restrict write_start, const u16 *restrict read_start, usize _len)
DECLARE_MULTILIB_ANYFUNCTION(long_cvt_noinline_u8_u32, void, u32 *restrict write_start, const u8 *restrict read_start, usize _len)
DECLARE_MULTILIB_ANYFUNCTION(long_cvt_noinline_u8_u16, void, u16 *restrict write_start, const u8 *restrict read_start, usize _len)
DECLARE_MULTILIB_ANYFUNCTION(long_cvt_noinline_u32_u16, void, u16 *restrict write_start, const u32 *restrict read_start, usize _len)
DECLARE_MULTILIB_ANYFUNCTION(long_cvt_noinline_u32_u8, void, u8 *restrict write_start, const u32 *restrict read_start, usize _len)
DECLARE_MULTILIB_ANYFUNCTION(long_cvt_noinline_u16_u8, void, u8 *restrict write_start, const u16 *restrict read_start, usize _len)

#    endif // SSRJSON_X86
#endif     // BUILD_MULTI_LIB

#endif // SSRJSON_PYTHONLIB_H

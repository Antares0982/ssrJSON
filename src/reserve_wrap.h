#ifndef RESERVE_IMPL_H
#define RESERVE_IMPL_H


#define COMPILE_WRITE_UCS_LEVEL 1
#include "_reserve.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 2
#include "_reserve.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 4
#include "_reserve.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL


#endif // RESERVE_IMPL_H

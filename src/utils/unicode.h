#ifndef SSRJSON_UNICODE_UNICODE_H
#define SSRJSON_UNICODE_UNICODE_H

#include "ssrjson.h"

#define PYUNICODE_ASCII_START(_obj_) SSRJSON_CAST(u8 *, SSRJSON_CAST(PyASCIIObject *, (_obj_)) + 1)
#define PYUNICODE_UCS1_START(_obj_) SSRJSON_CAST(u8 *, SSRJSON_CAST(PyCompactUnicodeObject *, (_obj_)) + 1)
#define PYUNICODE_UCS2_START(_obj_) SSRJSON_CAST(u16 *, SSRJSON_CAST(PyCompactUnicodeObject *, (_obj_)) + 1)
#define PYUNICODE_UCS4_START(_obj_) SSRJSON_CAST(u32 *, SSRJSON_CAST(PyCompactUnicodeObject *, (_obj_)) + 1)

force_noinline void init_pyunicode_noinline(void *, Py_ssize_t size, int kind);

#endif // SSRJSON_UNICODE_UNICODE_H

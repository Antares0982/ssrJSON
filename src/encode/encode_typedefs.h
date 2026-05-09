#ifndef SSRJSON_ENCODE_TYPEDEFS_H
#define SSRJSON_ENCODE_TYPEDEFS_H
#include "ssrjson.h"

/*==============================================================================
 * Enums
 *============================================================================*/
typedef enum EncodeContainerType {
    EncodeContainerType_Dict = 0,
    EncodeContainerType_List = 1,
    EncodeContainerType_Tuple = 2,
} EncodeContainerType;

typedef enum EncodePyTypes {
    T_Unicode,
    T_Long,
    T_Bool,
    T_None,
    T_Float,
    T_List,
    T_Dict,
    T_Tuple,
    T_UnicodeNonCompact,
    T_NumpyArray,
    T_NumpyFloat32,
    T_NumpyInt64,
    T_NumpyInt32,
    T_NumpyUint64,
    T_NumpyUint32,
    T_NumpyBool,
    T_NumpyFloat16,
    T_NumpyInt16,
    T_NumpyInt8,
    T_NumpyUint16,
    T_NumpyUint8,
    T_Unknown,
} EncodePyTypes;

typedef enum EncodeValJumpFlag {
    JumpFlag_Default,
    JumpFlag_ArrValBegin,
    JumpFlag_DictPairBegin,
    JumpFlag_TupleValBegin,
    JumpFlag_Elevate1_ArrVal,
    JumpFlag_Elevate1_ObjVal,
    JumpFlag_Elevate1_Key,
    JumpFlag_Elevate2_ArrVal,
    JumpFlag_Elevate2_ObjVal,
    JumpFlag_Elevate2_Key,
    JumpFlag_Elevate4_ArrVal,
    JumpFlag_Elevate4_ObjVal,
    JumpFlag_Elevate4_Key,
    JumpFlag_Fail,
} EncodeValJumpFlag;

typedef enum EncodeCallFlag {
    CallFlag_ObjVal,
    CallFlag_ArrVal,
    CallFlag_Key,
} EncodeCallFlag;

/*==============================================================================
 * Typedefs
 *============================================================================*/

typedef struct EncodeCtnWithIndex {
    PyObject *ctn;
    usize index_and_type;
} EncodeCtnWithIndex;

typedef struct EncodeUnicodeInfo {
    Py_ssize_t ascii_size;
    Py_ssize_t u8_size;
    Py_ssize_t u16_size;
    Py_ssize_t u32_size;
    int cur_ucs_type;
} EncodeUnicodeInfo;

typedef struct EncodeUBufInfo {
    void *head;
    void *end;
} EncodeUBufInfo;

typedef void *EncodeUnicodeWriter;

#endif // SSRJSON_ENCODE_TYPEDEFS_H

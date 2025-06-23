#ifndef SSRJSON_UNION_VECTOR_H
#define SSRJSON_UNION_VECTOR_H
#include "vector_types.h"

typedef union {
    vector_a_u8_128 x[2];
    vector_a_u8_256 y;
} unionvector_a_u8_128_x2;

typedef union {
    vector_a_u16_128 x[2];
    vector_a_u16_256 y;
} unionvector_a_u16_128_x2;

typedef union {
    vector_a_u32_128 x[2];
    vector_a_u32_256 y;
} unionvector_a_u32_128_x2;

typedef union {
    vector_a_u8_128 x[4];
    vector_a_u8_256 y[2];
    vector_a_u8_512 z;
} unionvector_a_u8_128_x4;

typedef union {
    vector_a_u16_128 x[4];
    vector_a_u16_256 y[2];
    vector_a_u16_512 z;
} unionvector_a_u16_128_x4;

typedef union {
    vector_a_u32_128 x[4];
    vector_a_u32_256 y[2];
    vector_a_u32_512 z;
} unionvector_a_u32_128_x4;

typedef union {
    vector_a_u8_256 x[2];
    vector_a_u8_512 y;
} unionvector_a_u8_256_x2;

typedef union {
    vector_a_u16_256 x[2];
    vector_a_u16_512 y;
} unionvector_a_u16_256_x2;

typedef union {
    vector_a_u32_256 x[2];
    vector_a_u32_512 y;
} unionvector_a_u32_256_x2;

typedef union {
    vector_a_u8_256 x[4];
    vector_a_u8_512 y[2];
    vector_a_u8_1024 z;
} unionvector_a_u8_256_x4;

typedef union {
    vector_a_u16_256 x[4];
    vector_a_u16_512 y[2];
    vector_a_u16_1024 z;
} unionvector_a_u16_256_x4;

typedef union {
    vector_a_u32_256 x[4];
    vector_a_u32_512 y[2];
    vector_a_u32_1024 z;
} unionvector_a_u32_256_x4;

typedef union {
    vector_a_u8_512 x[2];
    vector_a_u8_1024 y;
} unionvector_a_u8_512_x2;

typedef union {
    vector_a_u16_512 x[2];
    vector_a_u16_1024 y;
} unionvector_a_u16_512_x2;

typedef union {
    vector_a_u32_512 x[2];
    vector_a_u32_1024 y;
} unionvector_a_u32_512_x2;

typedef union {
    vector_a_u8_512 x[4];
    vector_a_u8_1024 y[2];
    vector_a_u8_2048 z;
} unionvector_a_u8_512_x4;

typedef union {
    vector_a_u16_512 x[4];
    vector_a_u16_1024 y[2];
    vector_a_u16_2048 z;
} unionvector_a_u16_512_x4;

typedef union {
    vector_a_u32_512 x[4];
    vector_a_u32_1024 y[2];
    vector_a_u32_2048 z;
} unionvector_a_u32_512_x4;

#endif // SSRJSON_UNION_VECTOR_H

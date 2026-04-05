/*==============================================================================
 Copyright (c) 2025 Antares <antares0982@gmail.com>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *============================================================================*/

#ifndef SSRJSON_NDARRAY_H
#define SSRJSON_NDARRAY_H

#include "encode/writer_wrap.h"
#include "reserve_wrap.h"

#define MAX_NDARRAY_DIMENSION 32

/*==============================================================================
 * PyArrayInterface (from numpy array interface protocol)
 *============================================================================*/

#define NPY_ARRAY_C_CONTIGUOUS 0x0001
#define NPY_ARRAY_NOTSWAPPED 0x0200

typedef enum {
    NDA_f64,
    NDA_f32,
    NDA_f16,
    NDA_i64,
    NDA_i32,
    NDA_i16,
    NDA_i8,
    NDA_u64,
    NDA_u32,
    NDA_u16,
    NDA_u8,
    NDA_bool,
    NDA_err,
} NDATypes;

typedef struct {
    int two;             /* contains the integer 2 -- simple sanity check */
    int nd;              /* number of dimensions */
    char typekind;       /* kind in array --- character code of typestr */
    int itemsize;        /* size of each element */
    int flags;           /* flags indicating how the data should be interpreted */
                         /*   must set ARR_HAS_DESCR bit to validate descr */
    Py_ssize_t *shape;   /* A length-nd array of shape information */
    Py_ssize_t *strides; /* A length-nd array of stride information */
    void *data;          /* A pointer to the first element of the array */
    PyObject *descr;     /* NULL or data-description (same as descr key
                                of __array_interface__) -- must set ARR_HAS_DESCR
                                flag or this will be ignored. */
} PyArrayInterface;

extern u8 *xjb64(double value, u8 *buffer);

/*==============================================================================
 * Element writers (ssrjson_nofail, buffer already reserved)
 *
 * Each writes the value followed by a trailing comma.
 *============================================================================*/

force_inline ssrjson_nofail u8 *ndarray_write_i8_elem(u8 *writer, i8 v) {
    if (v == 0) {
        *writer++ = '0';
        *writer++ = ',';
    } else {
        int sign = v < 0;
        *writer = '-';
        writer = write_u8(sign ? (u8)(-(i8)v) : (u8)v, writer + sign);
        *writer++ = ',';
    }
    return writer;
}

force_inline ssrjson_nofail u8 *ndarray_write_i16_elem(u8 *writer, i16 v) {
    if (v == 0) {
        *writer++ = '0';
        *writer++ = ',';
    } else {
        int sign = v < 0;
        *writer = '-';
        writer = write_u16(sign ? (u16)(-(i16)v) : (u16)v, writer + sign);
        *writer++ = ',';
    }
    return writer;
}

force_inline ssrjson_nofail u8 *ndarray_write_i32_elem(u8 *writer, i32 v) {
    if (v == 0) {
        *writer++ = '0';
        *writer++ = ',';
    } else {
        int sign = v < 0;
        *writer = '-';
        writer = write_u32(sign ? (u32)(-(i32)v) : (u32)v, writer + sign);
        *writer++ = ',';
    }
    return writer;
}

force_inline ssrjson_nofail u8 *ndarray_write_i64_elem(u8 *writer, i64 v) {
    if (v == 0) {
        *writer++ = '0';
        *writer++ = ',';
    } else {
        int sign = v < 0;
        *writer = '-';
        writer = write_u64(sign ? (u64)(-(i64)v) : (u64)v, writer + sign);
        *writer++ = ',';
    }
    return writer;
}

force_inline ssrjson_nofail u8 *ndarray_write_u8_elem(u8 *writer, u8 v) {
    if (v == 0) {
        *writer++ = '0';
        *writer++ = ',';
    } else {
        writer = write_u8(v, writer);
        *writer++ = ',';
    }
    return writer;
}

force_inline ssrjson_nofail u8 *ndarray_write_u16_elem(u8 *writer, u16 v) {
    if (v == 0) {
        *writer++ = '0';
        *writer++ = ',';
    } else {
        writer = write_u16(v, writer);
        *writer++ = ',';
    }
    return writer;
}

force_inline ssrjson_nofail u8 *ndarray_write_u32_elem(u8 *writer, u32 v) {
    if (v == 0) {
        *writer++ = '0';
        *writer++ = ',';
    } else {
        writer = write_u32(v, writer);
        *writer++ = ',';
    }
    return writer;
}

force_inline ssrjson_nofail u8 *ndarray_write_u64_elem(u8 *writer, u64 v) {
    if (v == 0) {
        *writer++ = '0';
        *writer++ = ',';
    } else {
        writer = write_u64(v, writer);
        *writer++ = ',';
    }
    return writer;
}

force_inline ssrjson_nofail u8 *ndarray_write_f16_elem(u8 *writer, u16 raw) {
    double v = f16_to_f64(raw);
    writer = xjb64(v, writer);
    *writer++ = ',';
    return writer;
}

force_inline ssrjson_nofail u8 *ndarray_write_f32_elem(u8 *writer, float v) {
    writer = xjb64((double)v, writer);
    *writer++ = ',';
    return writer;
}

force_inline ssrjson_nofail u8 *ndarray_write_f64_elem(u8 *writer, double v) {
    writer = xjb64(v, writer);
    *writer++ = ',';
    return writer;
}

/*==============================================================================
 * Reserve count estimation (stub)
 *
 * Returns the upper bound of bytes needed to encode the entire ndarray,
 * including all brackets, commas, and indent whitespace.
 *============================================================================*/

extern const usize _NdaElemWriteSizeTable[];

force_inline usize get_elem_write_size(NDATypes ndatype) {
    return _NdaElemWriteSizeTable[ndatype];
}

/*==============================================================================
 * Stack frame for iterative traversal
 *============================================================================*/

typedef struct {
    const char *data_ptr;
    Py_intptr_t remaining;
    Py_intptr_t stride;
} NdarrayFrame;

force_inline NDATypes to_ndatype(char typekind, int itemsize) {
    switch (typekind) {
        case 'f':
            switch (itemsize) {
                case 8:
                    return NDA_f64;
                case 4:
                    return NDA_f32;
                case 2:
                    return NDA_f16;
                default:
                    return NDA_err;
            }
        case 'i':
            switch (itemsize) {
                case 8:
                    return NDA_i64;
                case 4:
                    return NDA_i32;
                case 2:
                    return NDA_i16;
                case 1:
                    return NDA_i8;
                default:
                    return NDA_err;
            }
        case 'u':
            switch (itemsize) {
                case 8:
                    return NDA_u64;
                case 4:
                    return NDA_u32;
                case 2:
                    return NDA_u16;
                case 1:
                    return NDA_u8;
                default:
                    return NDA_err;
            }
        case 'b':
            return NDA_bool;
        default:
            return NDA_err;
    }
}


#endif // SSRJSON_NDARRAY_H

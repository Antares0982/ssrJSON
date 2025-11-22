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

#ifndef SSRJSON_ENCODE_ENCODE_UTF8_SHARED_H
#define SSRJSON_ENCODE_ENCODE_UTF8_SHARED_H

#include "ssrjson.h"

extern const u8 ControlEscapeTable_u8[(_Slash + 1) * 8];
extern const Py_ssize_t _ControlJump[_Slash + 1];
extern PyObject *JSONEncodeError;

/* UCS1 src. */
force_inline void encode_one_special_ucs1(u8 **writer_addr, u8 unicode) {
    // reserve: 8

    u8 *writer = *writer_addr;

    if (unicode >= 128) {
        *writer++ = (unicode >> 6) | 0xc0;
        *writer++ = (unicode & 0x3f) | 0x80;
    } else {
        assert(unicode < ControlMax || unicode == _Quote || unicode == _Slash);
        memcpy(writer, &ControlEscapeTable_u8[unicode * 8], 8);
        writer += _ControlJump[unicode];
    }

    *writer_addr = writer;
}

force_inline void encode_one_ucs1(u8 **writer_addr, u8 unicode) {
    if (unicode < 128 && unicode >= ControlMax && unicode != _Quote && unicode != _Slash) {
        *(*writer_addr)++ = unicode;
        return;
    }
    encode_one_special_ucs1(writer_addr, unicode);
}

force_inline void encode_one_ucs1_noescape(u8 **writer_addr, u8 unicode) {
    if (unicode < 128) {
        *(*writer_addr)++ = unicode;
    } else {
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 6) | 0xc0;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    }
}

/* UCS2 src. */
force_inline bool encode_one_ucs2(u8 **writer_addr, u16 unicode) {
    if (unicode < 128) {
        if (unicode >= ControlMax && unicode != _Slash && unicode != _Quote) {
            *(*writer_addr)++ = unicode;
        } else {
            u8 *writer = *writer_addr;
            memcpy(writer, &ControlEscapeTable_u8[unicode * 8], 8);
            writer += _ControlJump[unicode];
            *writer_addr = writer;
        }
    } else if (unicode < 0x800) {
        // 2 bytes
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 6) | 0xc0;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    } else {
        // 3 bytes
        if (unlikely(unicode >= 0xd800 && unicode <= 0xdfff)) {
            PyErr_SetString(JSONEncodeError, "Cannot encode unicode character in range [0xd800, 0xdfff] to UTF-8");
            return false;
        }
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 12) | 0xe0;
        *writer++ = ((unicode & 0xfc0) >> 6) | 0x80;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    }
    return true;
}

force_inline bool encode_one_ucs2_noescape(u8 **writer_addr, u16 unicode) {
    if (unicode < 128) {
        *(*writer_addr)++ = unicode;
    } else if (unicode < 0x800) {
        // 2 bytes
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 6) | 0xc0;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    } else {
        // 3 bytes
        if (unlikely(unicode >= 0xd800 && unicode <= 0xdfff)) {
            PyErr_SetString(JSONEncodeError, "Cannot encode unicode character in range [0xd800, 0xdfff] to UTF-8");
            return false;
        }
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 12) | 0xe0;
        *writer++ = ((unicode & 0xfc0) >> 6) | 0x80;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    }
    return true;
}

force_inline int ucs2_get_type(u16 unicode, bool *is_escaped) {
    if (unicode < 128) {
        *is_escaped = !(unicode >= ControlMax && unicode != _Slash && unicode != _Quote);
        return 1;
    } else if (unicode < 0x800) {
        return 2;
    }
    return 3;
}

/* UCS4 src. */
force_inline bool encode_one_ucs4(u8 **writer_addr, u32 unicode) {
    if (unicode < 128) {
        if (unicode >= ControlMax && unicode != _Slash && unicode != _Quote) {
            *(*writer_addr)++ = unicode;
        } else {
            u8 *writer = *writer_addr;
            memcpy(writer, &ControlEscapeTable_u8[unicode * 8], 8);
            writer += _ControlJump[unicode];
            *writer_addr = writer;
        }
    } else if (unicode < 0x800) {
        // 2 bytes
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 6) | 0xc0;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    } else if (unicode < 0x10000) {
        // 3 bytes
        if (unlikely(unicode >= 0xd800 && unicode <= 0xdfff)) {
            PyErr_SetString(JSONEncodeError, "Cannot encode unicode character in range [0xd800, 0xdfff] to UTF-8");
            return false;
        }
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 12) | 0xe0;
        *writer++ = ((unicode & 0xfc0) >> 6) | 0x80;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    } else {
        // 4 bytes
        // assert(unicode <= 0x10ffff); // cannot create such unicode object in normal way
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 18) | 0xf0;
        *writer++ = ((unicode >> 12) & 0x3f) | 0x80;
        *writer++ = ((unicode >> 6) & 0x3f) | 0x80;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    }
    return true;
}

force_inline bool encode_one_ucs4_noescape(u8 **writer_addr, u32 unicode) {
    if (unicode < 128) {
        *(*writer_addr)++ = unicode;
    } else if (unicode < 0x800) {
        // 2 bytes
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 6) | 0xc0;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    } else if (unicode < 0x10000) {
        // 3 bytes
        if (unlikely(unicode >= 0xd800 && unicode <= 0xdfff)) {
            PyErr_SetString(JSONEncodeError, "Cannot encode unicode character in range [0xd800, 0xdfff] to UTF-8");
            return false;
        }
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 12) | 0xe0;
        *writer++ = ((unicode & 0xfc0) >> 6) | 0x80;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    } else {
        // 4 bytes
        // assert(unicode <= 0x10ffff); // cannot create such unicode object in normal way
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 18) | 0xf0;
        *writer++ = ((unicode >> 12) & 0x3f) | 0x80;
        *writer++ = ((unicode >> 6) & 0x3f) | 0x80;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    }
    return true;
}

force_inline int ucs4_get_type(u32 unicode, bool *is_escaped) {
    if (unicode < 128) {
        *is_escaped = !(unicode >= ControlMax && unicode != _Slash && unicode != _Quote);
        return 1;
    } else if (unicode < 0x800) {
        return 2;
    } else if (unicode < 0x10000) {
        return 3;
    }
    return 4;
}

#endif // SSRJSON_ENCODE_ENCODE_UTF8_SHARED_H

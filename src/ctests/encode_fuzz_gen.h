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

#ifndef SSRJSON_ENCODE_FUZZ_GEN_H
#define SSRJSON_ENCODE_FUZZ_GEN_H

#include "ssrjson.h"

// Fetch the Python-side helper references (subclass types, numpy) needed by the
// generator. `encode_fuzz_module` is the imported `encode_fuzz` module.
// Returns true on success; on failure the Python error is set.
bool encode_fuzz_gen_init(PyObject *encode_fuzz_module);

// Build a Python object tree from the fuzzer input bytes. On success returns a
// new reference and writes whether the tree contains a lone surrogate into
// `*out_has_surrogate`. On failure returns NULL with the Python error set.
PyObject *encode_fuzz_gen_build(const u8 *data, usize size, bool *out_has_surrogate);

#endif // SSRJSON_ENCODE_FUZZ_GEN_H

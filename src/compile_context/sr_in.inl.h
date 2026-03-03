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

#ifndef SSRJSON_COMPILE_CONTEXT_SR
#define SSRJSON_COMPILE_CONTEXT_SR

// Include sub contexts.
#include "r_in.inl.h"
#include "s_in.inl.h"

/* The count of unicode that can be read in one SIMD register. */
#define READ_BATCH_COUNT (COMPILE_SIMD_BITS / 8 / sizeof(src_t))

/* Generate function names with SIMD level and reader type. */
#define make_sr_name(_x_) ssrjson_concat3(_x_, src_t, COMPILE_SIMD_BITS)
#ifdef COMPILE_UCS_LEVEL
/* Generate function names with SIMD level and unicode type. */
#    define make_s_ucs_name(_x_) ssrjson_concat3(_x_, __UCS_NAME, COMPILE_SIMD_BITS)
#endif

/*
 * The very basic vector type (aligned/unaligned), 
 * that can be presented by a SIMD register.
 */
#define vector_a make_sr_name(vector_a)
#define vector_u make_sr_name(vector_u)


#if !SSRJSON_X86 || COMPILE_READ_UCS_LEVEL == 4
// x86: for bit size < 512, we don't have cmp_epu8,
// the mask is calculated by subs_epu8.
// so we have to cmpeq with zero to get the real bit mask.
// x86 doesn't have the signed saturate minus for 32-bit integers.
#    define CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS 0
#else
#    define CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS 1
#endif

/*
 * Names using SR context.
 */
#define unionvector_a_x4 ssrjson_concat2(make_sr_name(unionvector_a), x4)
#define unionvector_u_x4 ssrjson_concat2(make_sr_name(unionvector_u), x4)
//
#define get_bitmask_from make_sr_name(get_bitmask_from)
#define get_escape_mask make_sr_name(get_escape_mask)
#define escape_mask_to_bitmask make_sr_name(escape_mask_to_bitmask)
#define escape_mask_to_done_count make_sr_name(escape_mask_to_done_count)
#define escape_mask_to_done_count_no_eq0 make_sr_name(escape_mask_to_done_count_no_eq0)
#define escape_mask_to_done_count_track_max make_sr_name(escape_mask_to_done_count_track_max)
#define joined4_escape_mask_to_done_count make_sr_name(joined4_escape_mask_to_done_count)
#define joined4_escape_mask_to_done_count_track_max make_sr_name(joined4_escape_mask_to_done_count_track_max)
#define broadcast make_sr_name(broadcast)
#define unsigned_saturate_minus make_sr_name(unsigned_saturate_minus)
// signed_cmplt availability: SSE2
#define signed_cmplt make_sr_name(signed_cmplt)
// signed_cmpgt availability: SSE2, AVX2
#define signed_cmpgt make_sr_name(signed_cmpgt)
#define cmpeq make_sr_name(cmpeq)
#define get_escape_bitmask make_sr_name(get_escape_bitmask)
#define escape_bitmask_to_done_count make_sr_name(escape_bitmask_to_done_count)
#define escape_bitmask_to_done_count_track_max make_sr_name(escape_bitmask_to_done_count_track_max)
#define joined4_escape_bitmask_to_done_count make_sr_name(joined4_escape_bitmask_to_done_count)
#define joined4_escape_bitmask_to_done_count_track_max make_sr_name(joined4_escape_bitmask_to_done_count_track_max)
#define cmpeq_bitmask make_sr_name(cmpeq_bitmask)
#define cmpneq_bitmask make_sr_name(cmpneq_bitmask)
#define unsigned_cmple_bitmask make_sr_name(unsigned_cmple_bitmask)
#define unsigned_cmplt_bitmask make_sr_name(unsigned_cmplt_bitmask)
#define unsigned_cmpge_bitmask make_sr_name(unsigned_cmpge_bitmask)
#define unsigned_cmpgt_bitmask make_sr_name(unsigned_cmpgt_bitmask)
#define signed_cmpgt_bitmask make_sr_name(signed_cmpgt_bitmask)
#define get_high_mask make_sr_name(get_high_mask)
#define high_mask make_sr_name(high_mask)
#define get_low_mask make_sr_name(get_low_mask)
#define low_mask make_sr_name(low_mask)
#define maskz_loadu make_sr_name(maskz_loadu)
#define fast_skip_spaces make_sr_name(fast_skip_spaces)
#define checkmax make_sr_name(checkmax)
#define unsigned_max make_sr_name(unsigned_max)
#define unsigned_max4 make_sr_name(unsigned_max4)
#define _CheckerMasks make_sr_name(_CheckerMasks)
//
#if SSRJSON_X86 && COMPILE_SIMD_BITS == 512
#    define anymask_t avx512_bitmask_t
#    define get_escape_anymask get_escape_bitmask
#    define testz_escape_mask(_x_) ((_x_) == 0)
#    define escape_anymask_to_done_count escape_bitmask_to_done_count
#    define escape_anymask_to_done_count_no_eq0 escape_bitmask_to_done_count
#    define escape_anymask_to_done_count_track_max escape_bitmask_to_done_count_track_max
#    define joined4_escape_anymask_to_done_count joined4_escape_bitmask_to_done_count
#    define joined4_escape_anymask_to_done_count_track_max joined4_escape_bitmask_to_done_count_track_max
#elif SSRJSON_X86
#    define anymask_t vector_a
#    define get_escape_anymask get_escape_mask
#    define testz_escape_mask testz
#    define escape_anymask_to_done_count escape_mask_to_done_count
#    define escape_anymask_to_done_count_no_eq0 escape_mask_to_done_count_no_eq0
#    define escape_anymask_to_done_count_track_max escape_mask_to_done_count_track_max
#    define joined4_escape_anymask_to_done_count joined4_escape_mask_to_done_count
#    define joined4_escape_anymask_to_done_count_track_max joined4_escape_mask_to_done_count_track_max
#elif SSRJSON_AARCH
#    define anymask_t vector_a
#    define get_escape_anymask get_escape_mask
#    define testz_escape_mask testz
#    define escape_anymask_to_done_count escape_mask_to_done_count
#    define escape_anymask_to_done_count_no_eq0 escape_mask_to_done_count
#    define escape_anymask_to_done_count_track_max escape_mask_to_done_count_track_max
#    define joined4_escape_anymask_to_done_count joined4_escape_mask_to_done_count
#    define joined4_escape_anymask_to_done_count_track_max joined4_escape_mask_to_done_count_track_max
#endif

#ifdef COMPILE_UCS_LEVEL
#    define __check_vector_max_char_internal make_s_ucs_name(__check_vector_max_char_internal)
#    define check_vector_max_char make_s_ucs_name(check_vector_max_char)
#endif

#endif // SSRJSON_COMPILE_CONTEXT_SR

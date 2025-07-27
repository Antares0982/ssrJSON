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

#undef SSRJSON_COMPILE_CONTEXT_SR
//
#include "r_out.inl.h"
#include "s_out.inl.h"
//
#undef READ_BATCH_COUNT
//
#undef MAKE_SR_NAME
//
#undef MAKE_S_UCS_NAME
//
#undef vector_a
#undef vector_u
//
#undef CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
//
#undef unionvector_a_x4
#undef unionvector_u_x4
//
#undef get_bitmask_from
#undef get_escape_mask
#undef escape_mask_to_bitmask
#undef escape_mask_to_done_count
#undef escape_mask_to_done_count_no_eq0
#undef escape_mask_to_done_count_track_max
#undef joined4_escape_mask_to_done_count
#undef joined4_escape_mask_to_done_count_track_max
#undef broadcast
#undef unsigned_saturate_minus
#undef signed_cmplt
#undef signed_cmpgt
#undef cmpeq
#undef get_escape_bitmask
#undef escape_bitmask_to_done_count
#undef escape_bitmask_to_done_count_track_max
#undef joined4_escape_bitmask_to_done_count
#undef joined4_escape_bitmask_to_done_count_track_max
#undef cmpeq_bitmask
#undef cmpneq_bitmask
#undef unsigned_cmple_bitmask
#undef unsigned_cmplt_bitmask
#undef unsigned_cmpge_bitmask
#undef unsigned_cmpgt_bitmask
#undef signed_cmpgt_bitmask
#undef get_high_mask
#undef high_mask
#undef get_low_mask
#undef low_mask
#undef maskz_loadu
#undef fast_skip_spaces
#undef checkmax
#undef unsigned_max
#undef unsigned_max4
//
#undef anymask_t
#undef get_escape_anymask
#undef testz_escape_mask
#undef escape_anymask_to_done_count
#undef escape_anymask_to_done_count_no_eq0
#undef escape_anymask_to_done_count_track_max
#undef joined4_escape_anymask_to_done_count
#undef joined4_escape_anymask_to_done_count_track_max
//
#undef __check_vector_max_char_internal
#undef check_vector_max_char

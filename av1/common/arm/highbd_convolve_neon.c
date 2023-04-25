/*
 * Copyright (c) 2023, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <assert.h>
#include <arm_neon.h>

#include "config/av1_rtcd.h"

#include "aom_dsp/aom_dsp_common.h"
#include "aom_dsp/arm/mem_neon.h"
#include "aom_dsp/arm/transpose_neon.h"
#include "aom_ports/mem.h"
#include "av1/common/convolve.h"
#include "av1/common/filter.h"
#include "av1/common/arm/highbd_convolve_neon.h"

static INLINE void highbd_convolve_y_sr_6tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *y_filter_ptr, const int bd) {
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  const int16x8_t y_filter_0_7 = vld1q_s16(y_filter_ptr);
  const int32x4_t zero_s32 = vdupq_n_s32(0);

  if (w <= 4) {
    uint16x4_t t0, t1, t2, t3, t4, t5, t6, t7, t8;
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, s8;
    uint16x4_t d0, d1, d2, d3;
    uint16x8_t d01, d23;

    const uint16_t *s = src_ptr + src_stride;
    uint16_t *d = dst_ptr;

    load_u16_4x5(s, src_stride, &t0, &t1, &t2, &t3, &t4);
    s0 = vreinterpret_s16_u16(t0);
    s1 = vreinterpret_s16_u16(t1);
    s2 = vreinterpret_s16_u16(t2);
    s3 = vreinterpret_s16_u16(t3);
    s4 = vreinterpret_s16_u16(t4);
    s += 5 * src_stride;

    do {
      load_u16_4x4(s, src_stride, &t5, &t6, &t7, &t8);
      s5 = vreinterpret_s16_u16(t5);
      s6 = vreinterpret_s16_u16(t6);
      s7 = vreinterpret_s16_u16(t7);
      s8 = vreinterpret_s16_u16(t8);

      d0 = highbd_convolve6_4_s32_s16(s0, s1, s2, s3, s4, s5, y_filter_0_7,
                                      zero_s32);
      d1 = highbd_convolve6_4_s32_s16(s1, s2, s3, s4, s5, s6, y_filter_0_7,
                                      zero_s32);
      d2 = highbd_convolve6_4_s32_s16(s2, s3, s4, s5, s6, s7, y_filter_0_7,
                                      zero_s32);
      d3 = highbd_convolve6_4_s32_s16(s3, s4, s5, s6, s7, s8, y_filter_0_7,
                                      zero_s32);

      d01 = vcombine_u16(d0, d1);
      d23 = vcombine_u16(d2, d3);

      d01 = vminq_u16(d01, max);
      d23 = vminq_u16(d23, max);

      if (w == 2) {
        store_u16q_2x1(d + 0 * dst_stride, d01, 0);
        store_u16q_2x1(d + 1 * dst_stride, d01, 2);
        if (h != 2) {
          store_u16q_2x1(d + 2 * dst_stride, d23, 0);
          store_u16q_2x1(d + 3 * dst_stride, d23, 2);
        }
      } else {
        vst1_u16(d + 0 * dst_stride, vget_low_u16(d01));
        vst1_u16(d + 1 * dst_stride, vget_high_u16(d01));
        if (h != 2) {
          vst1_u16(d + 2 * dst_stride, vget_low_u16(d23));
          vst1_u16(d + 3 * dst_stride, vget_high_u16(d23));
        }
      }

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      s += 4 * src_stride;
      d += 4 * dst_stride;
      h -= 4;
    } while (h > 0);
  } else {
    // if width is a multiple of 8 & height is a multiple of 4
    uint16x8_t t0, t1, t2, t3, t4, t5, t6, t7, t8;
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8;
    uint16x8_t d0, d1, d2, d3;

    do {
      int height = h;
      const uint16_t *s = src_ptr + src_stride;
      uint16_t *d = dst_ptr;

      load_u16_8x5(s, src_stride, &t0, &t1, &t2, &t3, &t4);
      s0 = vreinterpretq_s16_u16(t0);
      s1 = vreinterpretq_s16_u16(t1);
      s2 = vreinterpretq_s16_u16(t2);
      s3 = vreinterpretq_s16_u16(t3);
      s4 = vreinterpretq_s16_u16(t4);
      s += 5 * src_stride;

      do {
        load_u16_8x4(s, src_stride, &t5, &t6, &t7, &t8);
        s5 = vreinterpretq_s16_u16(t5);
        s6 = vreinterpretq_s16_u16(t6);
        s7 = vreinterpretq_s16_u16(t7);
        s8 = vreinterpretq_s16_u16(t8);

        d0 = highbd_convolve6_8_s32_s16(s0, s1, s2, s3, s4, s5, y_filter_0_7,
                                        zero_s32);
        d1 = highbd_convolve6_8_s32_s16(s1, s2, s3, s4, s5, s6, y_filter_0_7,
                                        zero_s32);
        d2 = highbd_convolve6_8_s32_s16(s2, s3, s4, s5, s6, s7, y_filter_0_7,
                                        zero_s32);
        d3 = highbd_convolve6_8_s32_s16(s3, s4, s5, s6, s7, s8, y_filter_0_7,
                                        zero_s32);

        d0 = vminq_u16(d0, max);
        d1 = vminq_u16(d1, max);
        d2 = vminq_u16(d2, max);
        d3 = vminq_u16(d3, max);

        if (h == 2) {
          store_u16_8x2(d, dst_stride, d0, d1);
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        height -= 4;
      } while (height > 0);

      src_ptr += 8;
      dst_ptr += 8;
      w -= 8;
    } while (w > 0);
  }
}

static INLINE void highbd_convolve_y_sr_8tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *y_filter_ptr, int bd) {
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  const int16x8_t y_filter = vld1q_s16(y_filter_ptr);
  const int32x4_t zero_s32 = vdupq_n_s32(0);

  if (w <= 4) {
    uint16x4_t t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10;
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;
    uint16x4_t d0, d1, d2, d3;
    uint16x8_t d01, d23;

    const uint16_t *s = src_ptr;
    uint16_t *d = dst_ptr;

    load_u16_4x7(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6);
    s0 = vreinterpret_s16_u16(t0);
    s1 = vreinterpret_s16_u16(t1);
    s2 = vreinterpret_s16_u16(t2);
    s3 = vreinterpret_s16_u16(t3);
    s4 = vreinterpret_s16_u16(t4);
    s5 = vreinterpret_s16_u16(t5);
    s6 = vreinterpret_s16_u16(t6);

    s += 7 * src_stride;

    do {
      load_u16_4x4(s, src_stride, &t7, &t8, &t9, &t10);
      s7 = vreinterpret_s16_u16(t7);
      s8 = vreinterpret_s16_u16(t8);
      s9 = vreinterpret_s16_u16(t9);
      s10 = vreinterpret_s16_u16(t10);

      d0 = highbd_convolve8_4_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7, y_filter,
                                      zero_s32);
      d1 = highbd_convolve8_4_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8, y_filter,
                                      zero_s32);
      d2 = highbd_convolve8_4_s32_s16(s2, s3, s4, s5, s6, s7, s8, s9, y_filter,
                                      zero_s32);
      d3 = highbd_convolve8_4_s32_s16(s3, s4, s5, s6, s7, s8, s9, s10, y_filter,
                                      zero_s32);

      d01 = vcombine_u16(d0, d1);
      d23 = vcombine_u16(d2, d3);

      d01 = vminq_u16(d01, max);
      d23 = vminq_u16(d23, max);

      if (w == 2) {
        store_u16q_2x1(d + 0 * dst_stride, d01, 0);
        store_u16q_2x1(d + 1 * dst_stride, d01, 2);
        if (h != 2) {
          store_u16q_2x1(d + 2 * dst_stride, d23, 0);
          store_u16q_2x1(d + 3 * dst_stride, d23, 2);
        }
      } else {
        vst1_u16(d + 0 * dst_stride, vget_low_u16(d01));
        vst1_u16(d + 1 * dst_stride, vget_high_u16(d01));
        if (h != 2) {
          vst1_u16(d + 2 * dst_stride, vget_low_u16(d23));
          vst1_u16(d + 3 * dst_stride, vget_high_u16(d23));
        }
      }

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      s5 = s9;
      s6 = s10;
      s += 4 * src_stride;
      d += 4 * dst_stride;
      h -= 4;
    } while (h > 0);
  } else {
    int height;
    uint16x8_t t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10;
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;
    uint16x8_t d0, d1, d2, d3;
    do {
      const uint16_t *s = src_ptr;
      uint16_t *d = dst_ptr;

      load_u16_8x7(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6);
      s0 = vreinterpretq_s16_u16(t0);
      s1 = vreinterpretq_s16_u16(t1);
      s2 = vreinterpretq_s16_u16(t2);
      s3 = vreinterpretq_s16_u16(t3);
      s4 = vreinterpretq_s16_u16(t4);
      s5 = vreinterpretq_s16_u16(t5);
      s6 = vreinterpretq_s16_u16(t6);

      s += 7 * src_stride;
      height = h;

      do {
        load_u16_8x4(s, src_stride, &t7, &t8, &t9, &t10);
        s7 = vreinterpretq_s16_u16(t7);
        s8 = vreinterpretq_s16_u16(t8);
        s9 = vreinterpretq_s16_u16(t9);
        s10 = vreinterpretq_s16_u16(t10);

        d0 = highbd_convolve8_8_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7,
                                        y_filter, zero_s32);
        d1 = highbd_convolve8_8_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8,
                                        y_filter, zero_s32);
        d2 = highbd_convolve8_8_s32_s16(s2, s3, s4, s5, s6, s7, s8, s9,
                                        y_filter, zero_s32);
        d3 = highbd_convolve8_8_s32_s16(s3, s4, s5, s6, s7, s8, s9, s10,
                                        y_filter, zero_s32);

        d0 = vminq_u16(d0, max);
        d1 = vminq_u16(d1, max);
        d2 = vminq_u16(d2, max);
        d3 = vminq_u16(d3, max);

        if (h == 2) {
          store_u16_8x2(d, dst_stride, d0, d1);
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s5 = s9;
        s6 = s10;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        height -= 4;
      } while (height > 0);
      src_ptr += 8;
      dst_ptr += 8;
      w -= 8;
    } while (w > 0);
  }
}

static INLINE void highbd_convolve_y_sr_12tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *y_filter_ptr, int bd) {
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  const int16x8_t y_filter_0_7 = vld1q_s16(y_filter_ptr);
  const int16x4_t y_filter_8_11 = vld1_s16(y_filter_ptr + 8);
  const int32x4_t zero_s32 = vdupq_n_s32(0);

  if (w <= 4) {
    uint16x4_t t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14;
    int16x4_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14;
    uint16x4_t d0, d1, d2, d3;
    uint16x8_t d01, d23;

    const uint16_t *s = src_ptr;
    uint16_t *d = dst_ptr;

    load_u16_4x11(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7, &t8,
                  &t9, &t10);
    s0 = vreinterpret_s16_u16(t0);
    s1 = vreinterpret_s16_u16(t1);
    s2 = vreinterpret_s16_u16(t2);
    s3 = vreinterpret_s16_u16(t3);
    s4 = vreinterpret_s16_u16(t4);
    s5 = vreinterpret_s16_u16(t5);
    s6 = vreinterpret_s16_u16(t6);
    s7 = vreinterpret_s16_u16(t7);
    s8 = vreinterpret_s16_u16(t8);
    s9 = vreinterpret_s16_u16(t9);
    s10 = vreinterpret_s16_u16(t10);

    s += 11 * src_stride;

    do {
      load_u16_4x4(s, src_stride, &t11, &t12, &t13, &t14);
      s11 = vreinterpret_s16_u16(t11);
      s12 = vreinterpret_s16_u16(t12);
      s13 = vreinterpret_s16_u16(t13);
      s14 = vreinterpret_s16_u16(t14);

      d0 = highbd_convolve12_y_4_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9,
                                         s10, s11, y_filter_0_7, y_filter_8_11,
                                         zero_s32);
      d1 = highbd_convolve12_y_4_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8, s9,
                                         s10, s11, s12, y_filter_0_7,
                                         y_filter_8_11, zero_s32);
      d2 = highbd_convolve12_y_4_s32_s16(s2, s3, s4, s5, s6, s7, s8, s9, s10,
                                         s11, s12, s13, y_filter_0_7,
                                         y_filter_8_11, zero_s32);
      d3 = highbd_convolve12_y_4_s32_s16(s3, s4, s5, s6, s7, s8, s9, s10, s11,
                                         s12, s13, s14, y_filter_0_7,
                                         y_filter_8_11, zero_s32);

      d01 = vcombine_u16(d0, d1);
      d23 = vcombine_u16(d2, d3);

      d01 = vminq_u16(d01, max);
      d23 = vminq_u16(d23, max);

      if (w == 2) {
        store_u16q_2x1(d + 0 * dst_stride, d01, 0);
        store_u16q_2x1(d + 1 * dst_stride, d01, 2);
        if (h != 2) {
          store_u16q_2x1(d + 2 * dst_stride, d23, 0);
          store_u16q_2x1(d + 3 * dst_stride, d23, 2);
        }
      } else {
        vst1_u16(d + 0 * dst_stride, vget_low_u16(d01));
        vst1_u16(d + 1 * dst_stride, vget_high_u16(d01));
        if (h != 2) {
          vst1_u16(d + 2 * dst_stride, vget_low_u16(d23));
          vst1_u16(d + 3 * dst_stride, vget_high_u16(d23));
        }
      }

      s0 = s4;
      s1 = s5;
      s2 = s6;
      s3 = s7;
      s4 = s8;
      s5 = s9;
      s6 = s10;
      s7 = s11;
      s8 = s12;
      s9 = s13;
      s10 = s14;
      s += 4 * src_stride;
      d += 4 * dst_stride;
      h -= 4;
    } while (h > 0);
  } else {
    uint16x8_t t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14;
    uint16x8_t d0, d1, d2, d3;
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14;

    do {
      const uint16_t *s = src_ptr;
      uint16_t *d = dst_ptr;
      int height = h;

      load_u16_8x11(s, src_stride, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7, &t8,
                    &t9, &t10);
      s0 = vreinterpretq_s16_u16(t0);
      s1 = vreinterpretq_s16_u16(t1);
      s2 = vreinterpretq_s16_u16(t2);
      s3 = vreinterpretq_s16_u16(t3);
      s4 = vreinterpretq_s16_u16(t4);
      s5 = vreinterpretq_s16_u16(t5);
      s6 = vreinterpretq_s16_u16(t6);
      s7 = vreinterpretq_s16_u16(t7);
      s8 = vreinterpretq_s16_u16(t8);
      s9 = vreinterpretq_s16_u16(t9);
      s10 = vreinterpretq_s16_u16(t10);

      s += 11 * src_stride;

      do {
        load_u16_8x4(s, src_stride, &t11, &t12, &t13, &t14);
        s11 = vreinterpretq_s16_u16(t11);
        s12 = vreinterpretq_s16_u16(t12);
        s13 = vreinterpretq_s16_u16(t13);
        s14 = vreinterpretq_s16_u16(t14);

        d0 = highbd_convolve12_y_8_s32_s16(s0, s1, s2, s3, s4, s5, s6, s7, s8,
                                           s9, s10, s11, y_filter_0_7,
                                           y_filter_8_11, zero_s32);
        d1 = highbd_convolve12_y_8_s32_s16(s1, s2, s3, s4, s5, s6, s7, s8, s9,
                                           s10, s11, s12, y_filter_0_7,
                                           y_filter_8_11, zero_s32);
        d2 = highbd_convolve12_y_8_s32_s16(s2, s3, s4, s5, s6, s7, s8, s9, s10,
                                           s11, s12, s13, y_filter_0_7,
                                           y_filter_8_11, zero_s32);
        d3 = highbd_convolve12_y_8_s32_s16(s3, s4, s5, s6, s7, s8, s9, s10, s11,
                                           s12, s13, s14, y_filter_0_7,
                                           y_filter_8_11, zero_s32);

        d0 = vminq_u16(d0, max);
        d1 = vminq_u16(d1, max);
        d2 = vminq_u16(d2, max);
        d3 = vminq_u16(d3, max);

        if (h == 2) {
          store_u16_8x2(d, dst_stride, d0, d1);
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

        s0 = s4;
        s1 = s5;
        s2 = s6;
        s3 = s7;
        s4 = s8;
        s5 = s9;
        s6 = s10;
        s7 = s11;
        s8 = s12;
        s9 = s13;
        s10 = s14;
        s += 4 * src_stride;
        d += 4 * dst_stride;
        height -= 4;
      } while (height > 0);

      src_ptr += 8;
      dst_ptr += 8;
      w -= 8;
    } while (w > 0);
  }
}

void av1_highbd_convolve_y_sr_neon(const uint16_t *src, int src_stride,
                                   uint16_t *dst, int dst_stride, int w, int h,
                                   const InterpFilterParams *filter_params_y,
                                   const int subpel_y_qn, int bd) {
  const int y_filter_taps = get_filter_tap(filter_params_y, subpel_y_qn);
  const int vert_zero_s32 = filter_params_y->taps / 2 - 1;
  const int16_t *y_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_y, subpel_y_qn & SUBPEL_MASK);

  src -= vert_zero_s32 * src_stride;

  if (y_filter_taps > 8) {
    highbd_convolve_y_sr_12tap_neon(src, src_stride, dst, dst_stride, w, h,
                                    y_filter_ptr, bd);
    return;
  }
  if (y_filter_taps < 8) {
    highbd_convolve_y_sr_6tap_neon(src, src_stride, dst, dst_stride, w, h,
                                   y_filter_ptr, bd);
    return;
  }

  highbd_convolve_y_sr_8tap_neon(src, src_stride, dst, dst_stride, w, h,
                                 y_filter_ptr, bd);
}

static INLINE void highbd_convolve_x_sr_8tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *x_filter_ptr, ConvolveParams *conv_params,
    int bd) {
  const int16x8_t x_filter = vld1q_s16(x_filter_ptr);
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  const int32x4_t shift_s32 = vdupq_n_s32(-conv_params->round_0);
  const int bits = FILTER_BITS - conv_params->round_0;
  const int16x8_t bits_s16 = vdupq_n_s16(-bits);
  const int32x4_t zero_s32 = vdupq_n_s32(0);

  if (w <= 4) {
    uint16x8_t t0, t1, t2, t3;
    int16x8_t s0, s1, s2, s3;
    uint16x4_t d0, d1;
    uint16x8_t d01;

    const uint16_t *s = src_ptr;
    uint16_t *d = dst_ptr;

    do {
      load_u16_8x2(s, src_stride, &t0, &t2);
      load_u16_8x2(s + 8, src_stride, &t1, &t3);
      s0 = vreinterpretq_s16_u16(t0);
      s1 = vreinterpretq_s16_u16(t1);
      s2 = vreinterpretq_s16_u16(t2);
      s3 = vreinterpretq_s16_u16(t3);

      d0 = highbd_convolve8_horiz4_s32_s16(s0, s1, x_filter, shift_s32,
                                           zero_s32);
      d1 = highbd_convolve8_horiz4_s32_s16(s2, s3, x_filter, shift_s32,
                                           zero_s32);

      d01 = vcombine_u16(d0, d1);
      d01 = vqrshlq_u16(d01, bits_s16);
      d01 = vminq_u16(d01, max);

      if (w == 2) {
        store_u16q_2x1(d + 0 * dst_stride, d01, 0);
        store_u16q_2x1(d + 1 * dst_stride, d01, 2);
      } else {
        vst1_u16(d + 0 * dst_stride, vget_low_u16(d01));
        vst1_u16(d + 1 * dst_stride, vget_high_u16(d01));
      }

      s += 2 * src_stride;
      d += 2 * dst_stride;
      h -= 2;
    } while (h > 0);
  } else {
    int height = h;
    uint16x8_t t0, t1, t2, t3, t4, t5, t6, t7;
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7;
    uint16x8_t d0, d1, d2, d3;
    do {
      int width = w;
      const uint16_t *s = src_ptr;
      uint16_t *d = dst_ptr;
      load_u16_8x4(s, src_stride, &t0, &t2, &t4, &t6);
      s0 = vreinterpretq_s16_u16(t0);
      s2 = vreinterpretq_s16_u16(t2);
      s4 = vreinterpretq_s16_u16(t4);
      s6 = vreinterpretq_s16_u16(t6);

      s += 8;
      do {
        load_u16_8x4(s, src_stride, &t1, &t3, &t5, &t7);
        s1 = vreinterpretq_s16_u16(t1);
        s3 = vreinterpretq_s16_u16(t3);
        s5 = vreinterpretq_s16_u16(t5);
        s7 = vreinterpretq_s16_u16(t7);

        d0 = highbd_convolve8_horiz8_s32_s16(s0, s1, x_filter, shift_s32,
                                             zero_s32);
        d1 = highbd_convolve8_horiz8_s32_s16(s2, s3, x_filter, shift_s32,
                                             zero_s32);
        d2 = highbd_convolve8_horiz8_s32_s16(s4, s5, x_filter, shift_s32,
                                             zero_s32);
        d3 = highbd_convolve8_horiz8_s32_s16(s6, s7, x_filter, shift_s32,
                                             zero_s32);

        d0 = vqrshlq_u16(d0, bits_s16);
        d1 = vqrshlq_u16(d1, bits_s16);
        d2 = vqrshlq_u16(d2, bits_s16);
        d3 = vqrshlq_u16(d3, bits_s16);

        d0 = vminq_u16(d0, max);
        d1 = vminq_u16(d1, max);
        d2 = vminq_u16(d2, max);
        d3 = vminq_u16(d3, max);

        if (h == 2) {
          store_u16_8x2(d, dst_stride, d0, d1);
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

        s0 = s1;
        s2 = s3;
        s4 = s5;
        s6 = s7;
        s += 8;
        d += 8;
        width -= 8;
      } while (width > 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height > 0);
  }
}

static INLINE void highbd_convolve_x_sr_12tap_neon(
    const uint16_t *src_ptr, int src_stride, uint16_t *dst_ptr, int dst_stride,
    int w, int h, const int16_t *x_filter_ptr, ConvolveParams *conv_params,
    int bd) {
  const uint16x8_t max = vdupq_n_u16((1 << bd) - 1);
  const int32x4_t shift_s32 = vdupq_n_s32(-conv_params->round_0);
  const int bits = FILTER_BITS - conv_params->round_0;
  const int16x8_t bits_s16 = vdupq_n_s16(-bits);
  const int16x8_t x_filter_0_7 = vld1q_s16(x_filter_ptr);
  const int16x4_t x_filter_8_11 = vld1_s16(x_filter_ptr + 8);
  const int32x4_t zero_s32 = vdupq_n_s32(0);

  if (w <= 4) {
    uint16x8_t t0, t1, t2, t3;
    int16x8_t s0, s1, s2, s3;
    uint16x4_t d0, d1;
    uint16x8_t d01;

    const uint16_t *s = src_ptr;
    uint16_t *d = dst_ptr;

    do {
      load_u16_8x2(s, src_stride, &t0, &t2);
      load_u16_8x2(s + 8, src_stride, &t1, &t3);
      s0 = vreinterpretq_s16_u16(t0);
      s1 = vreinterpretq_s16_u16(t1);
      s2 = vreinterpretq_s16_u16(t2);
      s3 = vreinterpretq_s16_u16(t3);

      d0 = highbd_convolve12_horiz4_s32_s16(s0, s1, x_filter_0_7, x_filter_8_11,
                                            shift_s32, zero_s32);
      d1 = highbd_convolve12_horiz4_s32_s16(s2, s3, x_filter_0_7, x_filter_8_11,
                                            shift_s32, zero_s32);

      d01 = vcombine_u16(d0, d1);
      d01 = vqrshlq_u16(d01, bits_s16);
      d01 = vminq_u16(d01, max);

      if (w == 2) {
        store_u16q_2x1(d + 0 * dst_stride, d01, 0);
        store_u16q_2x1(d + 1 * dst_stride, d01, 2);
      } else {
        vst1_u16(d + 0 * dst_stride, vget_low_u16(d01));
        vst1_u16(d + 1 * dst_stride, vget_high_u16(d01));
      }

      s += 2 * src_stride;
      d += 2 * dst_stride;
      h -= 2;
    } while (h > 0);
  } else {
    int height = h;
    uint16x8_t t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11;
    int16x8_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    uint16x8_t d0, d1, d2, d3;
    do {
      int width = w;
      const uint16_t *s = src_ptr;
      uint16_t *d = dst_ptr;
      load_u16_8x4(s, src_stride, &t0, &t3, &t6, &t9);
      s0 = vreinterpretq_s16_u16(t0);
      s3 = vreinterpretq_s16_u16(t3);
      s6 = vreinterpretq_s16_u16(t6);
      s9 = vreinterpretq_s16_u16(t9);

      s += 8;
      do {
        load_u16_8x4(s, src_stride, &t1, &t4, &t7, &t10);
        load_u16_8x4(s + 8, src_stride, &t2, &t5, &t8, &t11);
        s1 = vreinterpretq_s16_u16(t1);
        s2 = vreinterpretq_s16_u16(t2);
        s4 = vreinterpretq_s16_u16(t4);
        s5 = vreinterpretq_s16_u16(t5);
        s7 = vreinterpretq_s16_u16(t7);
        s8 = vreinterpretq_s16_u16(t8);
        s10 = vreinterpretq_s16_u16(t10);
        s11 = vreinterpretq_s16_u16(t11);

        d0 = highbd_convolve12_horiz8_s32_s16(
            s0, s1, s2, x_filter_0_7, x_filter_8_11, shift_s32, zero_s32);
        d1 = highbd_convolve12_horiz8_s32_s16(
            s3, s4, s5, x_filter_0_7, x_filter_8_11, shift_s32, zero_s32);
        d2 = highbd_convolve12_horiz8_s32_s16(
            s6, s7, s8, x_filter_0_7, x_filter_8_11, shift_s32, zero_s32);
        d3 = highbd_convolve12_horiz8_s32_s16(
            s9, s10, s11, x_filter_0_7, x_filter_8_11, shift_s32, zero_s32);

        d0 = vqrshlq_u16(d0, bits_s16);
        d1 = vqrshlq_u16(d1, bits_s16);
        d2 = vqrshlq_u16(d2, bits_s16);
        d3 = vqrshlq_u16(d3, bits_s16);

        d0 = vminq_u16(d0, max);
        d1 = vminq_u16(d1, max);
        d2 = vminq_u16(d2, max);
        d3 = vminq_u16(d3, max);

        if (h == 2) {
          store_u16_8x2(d, dst_stride, d0, d1);
        } else {
          store_u16_8x4(d, dst_stride, d0, d1, d2, d3);
        }

        s0 = s1;
        s1 = s2;
        s3 = s4;
        s4 = s5;
        s6 = s7;
        s7 = s8;
        s9 = s10;
        s10 = s11;
        s += 8;
        d += 8;
        width -= 8;
      } while (width > 0);
      src_ptr += 4 * src_stride;
      dst_ptr += 4 * dst_stride;
      height -= 4;
    } while (height > 0);
  }
}

void av1_highbd_convolve_x_sr_neon(const uint16_t *src, int src_stride,
                                   uint16_t *dst, int dst_stride, int w, int h,
                                   const InterpFilterParams *filter_params_x,
                                   const int subpel_x_qn,
                                   ConvolveParams *conv_params, int bd) {
  const int x_filter_taps = get_filter_tap(filter_params_x, subpel_x_qn);
  const int horiz_zero_s32 = filter_params_x->taps / 2 - 1;
  const int16_t *x_filter_ptr = av1_get_interp_filter_subpel_kernel(
      filter_params_x, subpel_x_qn & SUBPEL_MASK);

  src -= horiz_zero_s32;

  if (x_filter_taps > 8) {
    highbd_convolve_x_sr_12tap_neon(src, src_stride, dst, dst_stride, w, h,
                                    x_filter_ptr, conv_params, bd);
    return;
  }

  highbd_convolve_x_sr_8tap_neon(src, src_stride, dst, dst_stride, w, h,
                                 x_filter_ptr, conv_params, bd);
}

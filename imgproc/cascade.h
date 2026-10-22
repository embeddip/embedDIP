// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CASCADE_H
#define EMBEDDIP_CASCADE_H

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Builds a summed-area (integral) table of a grayscale image.
 *
 * @param[in]  src       Source grayscale image, width w, height h.
 * @param[out] integral  Caller-allocated output, size (w+1)*(h+1), row
 *                        stride (w+1), with a leading zero row/column.
 */
embeddip_status_t computeIntegralImage(const Image *src, uint32_t *integral);

/**
 * @brief 2-stage Haar-lite cascade + sliding-window scan.
 *
 * Each window is classified by two axis-aligned mean-difference features
 * (left/right halves, then top/bottom halves) read from @p integral; a
 * window passes only if both feature magnitudes exceed @p split_threshold.
 * (A full Haar cascade chains many weak classifiers per stage; this is the
 * simplified 2-feature variant.)
 *
 * @param[in]  integral          Integral table from computeIntegralImage().
 * @param[in]  integral_width    Width of the source image (integral table is integral_width+1 wide).
 * @param[in]  integral_height   Height of the source image.
 * @param[in]  window            Square sliding-window size.
 * @param[in]  step              Scan stride in pixels.
 * @param[in]  split_threshold   Minimum |mean difference| to pass each stage.
 * @param[out] detections        Caller-allocated output array (x, y, width, height per hit).
 * @param[in]  max_detections    Capacity of @p detections.
 * @param[out] num_detections    Number of raw detections written.
 */
embeddip_status_t detectHaarCascade(const uint32_t *integral,
                                    uint32_t integral_width,
                                    uint32_t integral_height,
                                    int window,
                                    int step,
                                    int32_t split_threshold,
                                    Rectangle *detections,
                                    int max_detections,
                                    int *num_detections);

/**
 * @brief Collapses detections whose centers lie within @p merge_dist of a
 * detection already kept, in place.
 *
 * @param[in,out] detections   Detections to filter, in place.
 * @param[in]     num_detections  Number of entries in @p detections.
 * @param[in]     merge_dist      Center distance (pixels) below which two
 *                                 detections are treated as duplicates.
 * @param[out]    num_kept        Number of detections kept (front of array).
 */
embeddip_status_t nonMaxSuppressRects(Rectangle *detections,
                                      int num_detections,
                                      int merge_dist,
                                      int *num_kept);

#ifdef __cplusplus
}
#endif

#endif  // EMBEDDIP_CASCADE_H

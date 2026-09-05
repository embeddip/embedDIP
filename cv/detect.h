// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_DETECT_H
#define EMBEDDIP_CV_DETECT_H

#include <stddef.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"
#include "cv/haar.h"
#include "cv/integral.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief One detection: a window rectangle and its cascade score.
 */
typedef struct {
    Rectangle box; /**< Detected window in integral-image pixels. */
    int32_t score; /**< Cascade confidence (see cv_haar_cascade_score). */
} CvDetection;

/**
 * @brief Sliding-window scan configuration.
 */
typedef struct {
    uint16_t window_width;  /**< Scan window width in pixels (> 0). */
    uint16_t window_height; /**< Scan window height in pixels (> 0). */
    uint16_t step_x;        /**< Horizontal step in pixels (> 0). */
    uint16_t step_y;        /**< Vertical step in pixels (> 0). */
} CvScanConfig;

/**
 * @brief Slide a Haar cascade across an integral image, collecting passes.
 *
 * Appends detections to @p out (never exceeding @p out_capacity) starting at
 * @p *out_count, so the same buffer can accumulate detections from several
 * scales across multiple calls. The window's own dimensions are used; the
 * cascade's stored window is ignored so a caller can reuse one cascade across
 * scales by scaling the integral image instead.
 *
 * @param[in] table Integral image to scan.
 * @param[in] cascade Cascade model, read-only.
 * @param[in] scan Window and step configuration.
 * @param[in,out] out Caller-owned detection buffer.
 * @param[in] out_capacity Capacity of @p out.
 * @param[in,out] out_count In: existing count; out: count after appending.
 * @return EMBEDDIP_OK on success (including a full buffer that stops early),
 *         error code otherwise.
 */
embeddip_status_t cv_detect_scan(const CvIntegralU32 *table,
                                 const CvHaarCascade *cascade,
                                 const CvScanConfig *scan, CvDetection *out,
                                 size_t out_capacity, size_t *out_count);

/**
 * @brief Greedy non-maximum suppression over detections.
 *
 * Sorts by descending score (stable, lower index wins ties), then keeps a
 * detection only if its intersection-over-union with every already-kept
 * detection is at most @p iou_threshold. Operates in place: survivors are
 * moved to the front of @p detections and the survivor count is returned.
 *
 * @param[in,out] detections Detections to filter, reordered in place.
 * @param[in] count Number of input detections.
 * @param[in] iou_threshold IoU above which a lower-scored box is suppressed
 *            (0.0..1.0).
 * @param[out] out_kept Number of survivors left at the front.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_detect_nms(CvDetection *detections, size_t count,
                                float iou_threshold, size_t *out_kept);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_DETECT_H */

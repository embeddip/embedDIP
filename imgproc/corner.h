// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_IMGPROC_CORNER_H
#define EMBEDDIP_IMGPROC_CORNER_H

#include <stddef.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Corner response measure.
 */
typedef enum {
    CV_CORNER_HARRIS = 0,     /**< R = det(M) - k*trace(M)^2 */
    CV_CORNER_SHI_TOMASI = 1  /**< R = min eigenvalue of M */
} CvCornerMethod;

/**
 * @brief Corner detector configuration.
 */
typedef struct {
    CvCornerMethod method; /**< Response measure. */
    int block_size;        /**< Odd structure-tensor window (e.g. 3). */
    float k;               /**< Harris free parameter (~0.04); unused for Shi-Tomasi. */
    float threshold;       /**< Minimum response to keep a corner (absolute). */
    int nms_radius;        /**< Non-max-suppression radius in pixels (>= 1). */
} CvCornerConfig;

/**
 * @brief One detected corner.
 */
typedef struct {
    int32_t x;    /**< Column. */
    int32_t y;    /**< Row. */
    float score;  /**< Corner response at (x, y). */
} CvCorner;

/**
 * @brief Detect corners (Harris / Shi-Tomasi) in an 8-bit grayscale image.
 *
 * Computes Sobel gradients, accumulates the structure tensor over a
 * @p block_size window, evaluates the chosen response, then keeps points that
 * exceed @p threshold and are strict local maxima within @p nms_radius. When
 * more corners survive than @p out_capacity, the highest-scoring ones are kept.
 *
 * @param[in] src 8-bit grayscale image view.
 * @param[in] cfg Detector configuration.
 * @param[out] out Caller-owned corner buffer, filled by descending score.
 * @param[in] out_capacity Capacity of @p out.
 * @param[out] out_count Number of corners written (<= out_capacity).
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_corner_detect(const ImageView *src, const CvCornerConfig *cfg,
                                   CvCorner *out, size_t out_capacity, size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_IMGPROC_CORNER_H */

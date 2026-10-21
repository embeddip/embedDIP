// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_TRACK_HIST_H
#define EMBEDDIP_CV_TRACK_HIST_H

#include <stdint.h>

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Number of bins for a grayscale/mask histogram. */
#define CV_HIST_GRAY_BINS 32u
/** @brief Number of bins for an RGB565 histogram (32 per channel). */
#define CV_HIST_COLOR_BINS 96u
/** @brief Largest bin count either histogram variant can produce. */
#define CV_HIST_MAX_BINS 96u

/**
 * @brief Build a normalized (sum = 1) intensity/color histogram over a ROI.
 *
 * Grayscale/mask images produce a 32-bin histogram (pixel >> 3). RGB565
 * images produce a 96-bin histogram: bins [0,32) hold the 5-bit red
 * channel, [32,64) the 6-bit green channel folded to 32 bins, and [64,96)
 * the 5-bit blue channel.
 *
 * @param[in] img Source image view (GRAYSCALE/MASK, U8, or RGB565, U16).
 * @param[in] roi Region of interest; clamped to the image bounds.
 * @param[out] out Buffer of at least CV_HIST_MAX_BINS floats.
 * @param[out] out_nbins Number of bins actually written (32 or 96).
 * @return EMBEDDIP_OK on success; EMBEDDIP_ERROR_NULL_PTR if img/out/out_nbins
 *         is NULL; EMBEDDIP_ERROR_INVALID_FORMAT if the image format/depth is
 *         unsupported; EMBEDDIP_ERROR_INVALID_SIZE if the clamped ROI is empty.
 */
embeddip_status_t cv_hist_build(const ImageView *img, Rectangle roi,
                                 float *out, uint32_t *out_nbins);

/**
 * @brief Bhattacharyya similarity between two normalized histograms.
 *
 * Computed as exp(-k * sqrt(max(0, 1 - sum(sqrt(p_i * q_i))))).
 *
 * @param[in] p First normalized histogram.
 * @param[in] q Second normalized histogram.
 * @param[in] nbins Number of bins in @p p and @p q.
 * @return Similarity in [0, 1]; higher means more similar. 0 if p or q is
 *         NULL or nbins is 0.
 */
float cv_hist_bhattacharyya(const float *p, const float *q, uint32_t nbins);

/** @brief Grayscale/mask histogram bin for a pixel value (bins [0, 32)). */
static inline uint32_t cv_hist_gray_bin(uint8_t px)
{
    return (uint32_t)px >> 3;
}

/** @brief RGB565 histogram bin for a pixel's 5-bit red channel (bins [0, 32)). */
static inline uint32_t cv_hist_rgb565_bin_r(uint16_t px)
{
    return ((uint32_t)px >> 11) & 0x1Fu;
}

/** @brief RGB565 histogram bin for a pixel's 6-bit green channel, folded to 32 bins ([32, 64)). */
static inline uint32_t cv_hist_rgb565_bin_g(uint16_t px)
{
    return 32u + ((((uint32_t)px >> 5) & 0x3Fu) >> 1);
}

/** @brief RGB565 histogram bin for a pixel's 5-bit blue channel (bins [64, 96)). */
static inline uint32_t cv_hist_rgb565_bin_b(uint16_t px)
{
    return 64u + ((uint32_t)px & 0x1Fu);
}

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_TRACK_HIST_H */

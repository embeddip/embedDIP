// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_HOG_H
#define EMBEDDIP_CV_HOG_H

#include <stddef.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Orientation bins per cell (unsigned, 0..180 degrees). */
#define CV_HOG_BINS 9u
/** Cells per block edge; blocks are 2x2 cells. */
#define CV_HOG_BLOCK_CELLS 2u
/** Values per block descriptor (2 * 2 * 9). */
#define CV_HOG_BLOCK_SIZE (CV_HOG_BLOCK_CELLS * CV_HOG_BLOCK_CELLS * CV_HOG_BINS)

/**
 * @brief HOG extraction parameters.
 */
typedef struct {
    uint16_t cell_size;  /**< Cell edge in pixels (> 0). */
    float l2_hys_clip;   /**< L2-Hys clip threshold (e.g. 0.2). */
} CvHogConfig;

/**
 * @brief Compute the descriptor length for a ROI and configuration.
 *
 * @param[in] roi Region of interest, at least 2x2 cells.
 * @param[in] config Extraction configuration.
 * @param[out] out_length Descriptor length in floats.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_hog_descriptor_size(Rectangle roi, const CvHogConfig *config,
                                         size_t *out_length);

/**
 * @brief Extract a HOG descriptor from a grayscale ROI.
 *
 * @param[in] src Valid 8-bit grayscale or mask image view.
 * @param[in] roi Region of interest inside the image, at least 2x2 cells.
 * @param[in] config Extraction configuration.
 * @param[out] descriptor Caller-owned output buffer.
 * @param[in] descriptor_capacity Capacity of @p descriptor in floats.
 * @param[out] out_length Number of floats written.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_hog_extract(const ImageView *src, Rectangle roi,
                                 const CvHogConfig *config, float *descriptor,
                                 size_t descriptor_capacity, size_t *out_length);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_HOG_H */

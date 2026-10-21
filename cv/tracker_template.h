// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_TRACKER_TEMPLATE_H
#define EMBEDDIP_CV_TRACKER_TEMPLATE_H

#include <stdbool.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum template patch width in pixels. */
#define CV_TEMPLATE_MAX_WIDTH 64u
/** Maximum template patch height in pixels. */
#define CV_TEMPLATE_MAX_HEIGHT 64u

/**
 * @brief Stored template patch and match state.
 */
typedef struct {
    uint8_t patch[CV_TEMPLATE_MAX_HEIGHT][CV_TEMPLATE_MAX_WIDTH];
    uint16_t width;
    uint16_t height;
    bool initialized;
} CvTemplateState;

/**
 * @brief Capture a template patch from a region of an 8-bit grayscale image.
 *
 * @param[out] state Template state to populate.
 * @param[in] src Grayscale (or mask) image view to copy the patch from.
 * @param[in] roi Region to copy; must fit within src and within
 *            CV_TEMPLATE_MAX_WIDTH x CV_TEMPLATE_MAX_HEIGHT.
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state or src is
 *         NULL, EMBEDDIP_ERROR_INVALID_FORMAT if src is not grayscale/mask,
 *         EMBEDDIP_ERROR_INVALID_SIZE if roi is out of bounds or too large.
 */
embeddip_status_t cv_template_set(CvTemplateState *state, const ImageView *src, Rectangle roi);

/**
 * @brief Find the best-matching location of the stored template in a new frame.
 *
 * Brute-force scan computing sum-of-absolute-differences at every candidate
 * origin; returns the origin with the lowest SAD score.
 *
 * @param[in] state Previously captured template (via cv_template_set).
 * @param[in] frame Grayscale (or mask) image view to search.
 * @param[out] out_box Best-match bounding box (same size as the template).
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if any pointer is
 *         NULL, EMBEDDIP_ERROR_NOT_INITIALIZED if cv_template_set was not
 *         called first, EMBEDDIP_ERROR_INVALID_SIZE if frame is smaller than
 *         the template.
 */
embeddip_status_t cv_template_match(const CvTemplateState *state, const ImageView *frame,
                                     Rectangle *out_box);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_TRACKER_TEMPLATE_H */

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_IMAGE_GRAY_H
#define EMBEDDIP_CV_IMAGE_GRAY_H

#include <stdint.h>

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate a non-owning 8-bit grayscale or mask image view.
 *
 * @param[in] view Image view to validate.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_gray_view_validate(const ImageView *view);

/**
 * @brief Read one pixel from a non-owning 8-bit grayscale or mask image view.
 *
 * @param[in] view Image view to read.
 * @param[in] x Horizontal pixel coordinate.
 * @param[in] y Vertical pixel coordinate.
 * @param[out] out_pixel Destination for the pixel value.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_gray_pixel_u8(const ImageView *view, uint32_t x, uint32_t y,
                                    uint8_t *out_pixel);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_IMAGE_GRAY_H */

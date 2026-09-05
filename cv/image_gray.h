// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_IMAGE_GRAY_H
#define EMBEDDIP_CV_IMAGE_GRAY_H

#include "core/error.h"
#include "core/image.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check whether a format is grayscale or mask (8-bit single channel).
 *
 * @param[in] f Format to check.
 * @return true if grayscale or mask.
 */
static inline bool cv_format_is_gray(ImageFormat f)
{
    return f == IMAGE_FORMAT_GRAYSCALE || f == IMAGE_FORMAT_MASK;
}

/**
 * @brief Check whether a format is grayscale, mask, or packed RGB565.
 *
 * @param[in] f Format to check.
 * @return true if grayscale, mask, or RGB565.
 */
static inline bool cv_format_is_gray_or_rgb565(ImageFormat f)
{
    return cv_format_is_gray(f) || f == IMAGE_FORMAT_RGB565;
}

/**
 * @brief Clamp a rectangle to [0, width) x [0, height).
 *
 * @param[in] roi Rectangle to clamp.
 * @param[in] w Bound width.
 * @param[in] h Bound height.
 * @param[out] out Clamped rectangle; only written when the clamp is non-empty.
 * @return false if the clamped rectangle is empty (roi lies fully outside
 *         the [0, w) x [0, h) bounds, or was empty to begin with).
 */
static inline bool cv_clamp_roi(Rectangle roi, uint32_t w, uint32_t h, Rectangle *out)
{
    int32_t x0 = roi.x < 0 ? 0 : roi.x;
    int32_t y0 = roi.y < 0 ? 0 : roi.y;
    int32_t x1 = roi.x + roi.width;
    int32_t y1 = roi.y + roi.height;
    if (x1 > (int32_t)w) {
        x1 = (int32_t)w;
    }
    if (y1 > (int32_t)h) {
        y1 = (int32_t)h;
    }
    if (x1 <= x0 || y1 <= y0) {
        return false;
    }
    out->x = x0;
    out->y = y0;
    out->width = x1 - x0;
    out->height = y1 - y0;
    return true;
}

/**
 * @brief Clamp a box_width x box_height box centered at (cx,cy) into
 *        [0, frame_width) x [0, frame_height), preferring to shift the box
 *        (not shrink it) when it would otherwise cross a low edge, then a
 *        high edge.
 *
 * @param[in] cx Box center x.
 * @param[in] cy Box center y.
 * @param[in] box_width Box width.
 * @param[in] box_height Box height.
 * @param[in] frame_width Bound width.
 * @param[in] frame_height Bound height.
 * @return The clamped box (same width/height, shifted origin).
 */
static inline Rectangle cv_clamp_centered_box(int32_t cx,
                                              int32_t cy,
                                              int32_t box_width,
                                              int32_t box_height,
                                              uint32_t frame_width,
                                              uint32_t frame_height)
{
    Rectangle roi = {cx - box_width / 2, cy - box_height / 2, box_width, box_height};
    if (roi.x < 0) {
        roi.x = 0;
    }
    if (roi.y < 0) {
        roi.y = 0;
    }
    if ((uint32_t)(roi.x + roi.width) > frame_width) {
        roi.x = (int32_t)frame_width - roi.width;
    }
    if ((uint32_t)(roi.y + roi.height) > frame_height) {
        roi.y = (int32_t)frame_height - roi.height;
    }
    return roi;
}

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
embeddip_status_t
cv_gray_pixel_u8(const ImageView *view, uint32_t x, uint32_t y, uint8_t *out_pixel);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_IMAGE_GRAY_H */

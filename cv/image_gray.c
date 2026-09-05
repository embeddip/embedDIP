// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/image_gray.h"

#include <stddef.h>
#include <stdint.h>

embeddip_status_t cv_gray_view_validate(const ImageView *view)
{
    if (view == NULL || view->pixels == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (view->width == 0u || view->height == 0u || view->row_stride_bytes < view->width) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    if (view->format != IMAGE_FORMAT_GRAYSCALE && view->format != IMAGE_FORMAT_MASK) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }
    if (view->depth != IMAGE_DEPTH_U8) {
        return EMBEDDIP_ERROR_INVALID_DEPTH;
    }

    return EMBEDDIP_OK;
}

embeddip_status_t cv_gray_pixel_u8(const ImageView *view, uint32_t x, uint32_t y,
                                    uint8_t *out_pixel)
{
    embeddip_status_t status;
    size_t offset;

    if (out_pixel == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    status = cv_gray_view_validate(view);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    if (x >= view->width || y >= view->height) {
        return EMBEDDIP_ERROR_OUT_OF_RANGE;
    }

    offset = ((size_t)view->row_stride_bytes * (size_t)y) + (size_t)x;
    *out_pixel = view->pixels[offset];
    return EMBEDDIP_OK;
}

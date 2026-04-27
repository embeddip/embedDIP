// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "imgwarp.h"

#include "board/common.h"
#include "core/error.h"

#include <stddef.h>
#include <stdint.h>

embeddip_status_t resize(Image *src, Image *dst, int width, int height)
{
    if (src == NULL || dst == NULL || width <= 0 || height <= 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    // Allocate channels if missing
    if (isChalsEmpty(dst)) {
        embeddip_status_t status = createChals(dst, 1);  // only 1 channel for grayscale
        if (status != EMBEDDIP_OK) {
            return status;
        }
    }

    float width_ratio = (float)src->width / (float)width;
    float height_ratio = (float)src->height / (float)height;

    // Clear output buffer (optional: here we fill with white 255.0f)
    for (uint32_t i = 0; i < (uint32_t)(width * height); i++) {
        dst->chals->ch[0][i] = 255.0f;
    }

    // Resize using nearest-neighbor interpolation
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int nearest_x = (int)(x * width_ratio);
            int nearest_y = (int)(y * height_ratio);

            if (nearest_x >= (int)src->width)
                nearest_x = (int)src->width - 1;
            if (nearest_y >= (int)src->height)
                nearest_y = (int)src->height - 1;

            dst->chals->ch[0][y * width + x] =
                (float)((uint8_t *)src->pixels)[nearest_y * src->width + nearest_x];
        }
    }

    // Update output image metadata
    dst->width = (uint32_t)width;
    dst->height = (uint32_t)height;
    dst->size = (uint32_t)(width * height);
    dst->log = IMAGE_DATA_CH0;

    return EMBEDDIP_OK;
}

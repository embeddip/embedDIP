// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/tracker_template.h"

#include <stdint.h>

static bool template_format_ok(ImageFormat fmt)
{
    return fmt == IMAGE_FORMAT_GRAYSCALE || fmt == IMAGE_FORMAT_MASK;
}

embeddip_status_t cv_template_set(CvTemplateState *state, const ImageView *src, Rectangle roi)
{
    if (state == NULL || src == NULL || src->pixels == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!template_format_ok(src->format)) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }
    if (roi.width <= 0 || roi.height <= 0 || (uint32_t)roi.width > CV_TEMPLATE_MAX_WIDTH ||
        (uint32_t)roi.height > CV_TEMPLATE_MAX_HEIGHT) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    if (roi.x < 0 || roi.y < 0 || (uint32_t)(roi.x + roi.width) > src->width ||
        (uint32_t)(roi.y + roi.height) > src->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    for (int32_t row = 0; row < roi.height; ++row) {
        const uint8_t *src_row = src->pixels + (size_t)(roi.y + row) * src->row_stride_bytes;
        for (int32_t col = 0; col < roi.width; ++col) {
            state->patch[row][col] = src_row[roi.x + col];
        }
    }

    state->width = (uint16_t)roi.width;
    state->height = (uint16_t)roi.height;
    state->initialized = true;
    return EMBEDDIP_OK;
}

/* Sum of absolute differences between the stored patch and a frame region. */
static uint32_t template_sad_at(const CvTemplateState *state, const ImageView *frame,
                                int32_t origin_x, int32_t origin_y)
{
    uint32_t sad = 0u;
    for (uint16_t row = 0u; row < state->height; ++row) {
        const uint8_t *frame_row =
            frame->pixels + (size_t)(origin_y + row) * frame->row_stride_bytes;
        for (uint16_t col = 0u; col < state->width; ++col) {
            int32_t diff = (int32_t)frame_row[origin_x + col] - (int32_t)state->patch[row][col];
            sad += (uint32_t)(diff < 0 ? -diff : diff);
        }
    }
    return sad;
}

embeddip_status_t cv_template_match(const CvTemplateState *state, const ImageView *frame,
                                     Rectangle *out_box)
{
    if (state == NULL || frame == NULL || out_box == NULL || frame->pixels == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!state->initialized) {
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }
    if (frame->width < state->width || frame->height < state->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    int32_t max_x = (int32_t)frame->width - (int32_t)state->width;
    int32_t max_y = (int32_t)frame->height - (int32_t)state->height;
    uint32_t best_sad = UINT32_MAX;
    int32_t best_x = 0;
    int32_t best_y = 0;

    for (int32_t y = 0; y <= max_y; ++y) {
        for (int32_t x = 0; x <= max_x; ++x) {
            uint32_t sad = template_sad_at(state, frame, x, y);
            if (sad < best_sad) {
                best_sad = sad;
                best_x = x;
                best_y = y;
            }
        }
    }

    out_box->x = best_x;
    out_box->y = best_y;
    out_box->width = state->width;
    out_box->height = state->height;
    return EMBEDDIP_OK;
}

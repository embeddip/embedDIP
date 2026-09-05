// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/integral.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "cv/image_gray.h"

static embeddip_status_t cv_integral_table_validate(const CvIntegralU32 *table)
{
    if (table == NULL || table->values == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (table->width == 0u || table->height == 0u ||
        table->row_stride_values < table->width) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    return EMBEDDIP_OK;
}

static int cv_integral_span_fits_size(uint32_t row_stride, uint32_t width,
                                      uint32_t height, size_t element_size)
{
    size_t rows_before_last = (size_t)height - 1u;
    size_t last_row_width = (size_t)width - 1u;
    size_t max_element_index = SIZE_MAX / element_size;

    return last_row_width <= max_element_index &&
           rows_before_last <=
               (max_element_index - last_row_width) / (size_t)row_stride;
}

embeddip_status_t cv_integral_u8_u32(const ImageView *src, CvIntegralU32 *dst)
{
    embeddip_status_t status;
    uint64_t pixel_count;
    uint32_t y;

    status = cv_gray_view_validate(src);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    status = cv_integral_table_validate(dst);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    if (dst->width != src->width || dst->height != src->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    pixel_count = (uint64_t)src->width * (uint64_t)src->height;
    if (pixel_count > (uint64_t)UINT32_MAX / (uint64_t)UINT8_MAX) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }
    if (!cv_integral_span_fits_size(src->row_stride_bytes, src->width, src->height,
                                    sizeof(*src->pixels)) ||
        !cv_integral_span_fits_size(dst->row_stride_values, dst->width, dst->height,
                                    sizeof(*dst->values))) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }

    for (y = 0u; y < src->height; ++y) {
        size_t src_row = (size_t)y * (size_t)src->row_stride_bytes;
        size_t dst_row = (size_t)y * (size_t)dst->row_stride_values;
        uint64_t row_sum = 0u;
        uint32_t x;

        for (x = 0u; x < src->width; ++x) {
            uint64_t value;

            row_sum += (uint64_t)src->pixels[src_row + (size_t)x];
            value = row_sum;
            if (y > 0u) {
                size_t preceding_row = dst_row - (size_t)dst->row_stride_values;
                value += (uint64_t)dst->values[preceding_row + (size_t)x];
            }
            if (value > (uint64_t)UINT32_MAX) {
                return EMBEDDIP_ERROR_OVERFLOW;
            }
            dst->values[dst_row + (size_t)x] = (uint32_t)value;
        }
    }

    return EMBEDDIP_OK;
}

embeddip_status_t cv_integral_sum_u32(const CvIntegralU32 *table, Rectangle roi,
                                      uint64_t *out_sum)
{
    embeddip_status_t status;
    uint64_t right;
    uint64_t bottom;
    uint32_t x0;
    uint32_t y0;
    uint32_t x1;
    uint32_t y1;
    uint64_t top_left = 0u;
    uint64_t above = 0u;
    uint64_t left = 0u;
    uint64_t bottom_right;

    if (out_sum == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    status = cv_integral_table_validate(table);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    if (!cv_integral_span_fits_size(table->row_stride_values, table->width,
                                    table->height, sizeof(*table->values))) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }
    if (roi.x < 0 || roi.y < 0 || roi.width <= 0 || roi.height <= 0) {
        return EMBEDDIP_ERROR_OUT_OF_RANGE;
    }

    right = (uint64_t)(uint32_t)roi.x + (uint64_t)(uint32_t)roi.width;
    bottom = (uint64_t)(uint32_t)roi.y + (uint64_t)(uint32_t)roi.height;
    if (right > (uint64_t)table->width || bottom > (uint64_t)table->height) {
        return EMBEDDIP_ERROR_OUT_OF_RANGE;
    }

    x0 = (uint32_t)roi.x;
    y0 = (uint32_t)roi.y;
    x1 = (uint32_t)(right - 1u);
    y1 = (uint32_t)(bottom - 1u);
    bottom_right = table->values[(size_t)y1 * (size_t)table->row_stride_values + x1];
    if (y0 > 0u) {
        above = table->values[((size_t)y0 - 1u) * (size_t)table->row_stride_values + x1];
    }
    if (x0 > 0u) {
        left = table->values[(size_t)y1 * (size_t)table->row_stride_values + x0 - 1u];
    }
    if (x0 > 0u && y0 > 0u) {
        top_left = table->values[((size_t)y0 - 1u) *
                                     (size_t)table->row_stride_values +
                                 x0 - 1u];
    }

    *out_sum = bottom_right + top_left - above - left;
    return EMBEDDIP_OK;
}

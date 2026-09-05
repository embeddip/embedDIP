// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/hog.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "cv/image_gray.h"

#define CV_HOG_EPS 1e-6f
#define CV_HOG_PI 3.14159265358979323846f

/* Read a pixel with coordinates clamped to the image (replicate border). */
static uint8_t hog_pixel_clamped(const ImageView *src, int32_t px, int32_t py)
{
    if (px < 0) {
        px = 0;
    } else if (px >= (int32_t)src->width) {
        px = (int32_t)src->width - 1;
    }
    if (py < 0) {
        py = 0;
    } else if (py >= (int32_t)src->height) {
        py = (int32_t)src->height - 1;
    }
    return src->pixels[(size_t)py * (size_t)src->row_stride_bytes + (size_t)px];
}

/* Accumulate a single cell's 9-bin orientation histogram. */
static void hog_cell_histogram(const ImageView *src, int32_t cell_x0, int32_t cell_y0,
                               uint16_t cell_size, float hist[CV_HOG_BINS])
{
    const float bin_width = CV_HOG_PI / (float)CV_HOG_BINS;
    uint16_t cy;

    for (uint16_t b = 0u; b < CV_HOG_BINS; ++b) {
        hist[b] = 0.0f;
    }

    for (cy = 0u; cy < cell_size; ++cy) {
        int32_t py = cell_y0 + (int32_t)cy;
        uint16_t cx;
        for (cx = 0u; cx < cell_size; ++cx) {
            int32_t px = cell_x0 + (int32_t)cx;
            float gx = (float)hog_pixel_clamped(src, px + 1, py) -
                       (float)hog_pixel_clamped(src, px - 1, py);
            float gy = (float)hog_pixel_clamped(src, px, py + 1) -
                       (float)hog_pixel_clamped(src, px, py - 1);
            float mag = sqrtf(gx * gx + gy * gy);
            float angle = atan2f(gy, gx); /* [-pi, pi] */
            float bin_f;
            int lo;
            int hi;
            float frac;

            if (angle < 0.0f) {
                angle += CV_HOG_PI; /* unsigned orientation, [0, pi) */
            }
            bin_f = angle / bin_width; /* [0, 9) */
            lo = (int)bin_f;
            if (lo >= (int)CV_HOG_BINS) {
                lo = (int)CV_HOG_BINS - 1; /* guard against angle == pi rounding */
            }
            frac = bin_f - (float)lo;
            hi = (lo + 1) % (int)CV_HOG_BINS;
            hist[lo] += mag * (1.0f - frac);
            hist[hi] += mag * frac;
        }
    }
}

static embeddip_status_t hog_geometry(Rectangle roi, const CvHogConfig *config,
                                      size_t *cells_x, size_t *cells_y,
                                      size_t *out_length)
{
    size_t cx;
    size_t cy;
    size_t blocks_x;
    size_t blocks_y;

    if (config == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (config->cell_size == 0u) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (roi.width <= 0 || roi.height <= 0) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    cx = (size_t)roi.width / (size_t)config->cell_size;
    cy = (size_t)roi.height / (size_t)config->cell_size;
    if (cx < CV_HOG_BLOCK_CELLS || cy < CV_HOG_BLOCK_CELLS) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    blocks_x = cx - (CV_HOG_BLOCK_CELLS - 1u);
    blocks_y = cy - (CV_HOG_BLOCK_CELLS - 1u);
    /* blocks_x * blocks_y * 36 with size_t overflow guard. */
    if (blocks_x != 0u && blocks_y > SIZE_MAX / blocks_x) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }
    {
        size_t blocks = blocks_x * blocks_y;
        if (blocks != 0u && CV_HOG_BLOCK_SIZE > SIZE_MAX / blocks) {
            return EMBEDDIP_ERROR_OVERFLOW;
        }
        *out_length = blocks * (size_t)CV_HOG_BLOCK_SIZE;
    }
    *cells_x = cx;
    *cells_y = cy;
    return EMBEDDIP_OK;
}

embeddip_status_t cv_hog_descriptor_size(Rectangle roi, const CvHogConfig *config,
                                         size_t *out_length)
{
    size_t cells_x;
    size_t cells_y;

    if (out_length == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    return hog_geometry(roi, config, &cells_x, &cells_y, out_length);
}

embeddip_status_t cv_hog_extract(const ImageView *src, Rectangle roi,
                                 const CvHogConfig *config, float *descriptor,
                                 size_t descriptor_capacity, size_t *out_length)
{
    embeddip_status_t status;
    size_t cells_x;
    size_t cells_y;
    size_t length;
    size_t blocks_x;
    size_t out = 0u;
    size_t by;

    if (descriptor == NULL || out_length == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    status = cv_gray_view_validate(src);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    status = hog_geometry(roi, config, &cells_x, &cells_y, &length);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    /* ROI must lie inside the image. */
    if (roi.x < 0 || roi.y < 0 ||
        (int64_t)roi.x + (int64_t)roi.width > (int64_t)src->width ||
        (int64_t)roi.y + (int64_t)roi.height > (int64_t)src->height) {
        return EMBEDDIP_ERROR_OUT_OF_RANGE;
    }
    if (descriptor_capacity < length) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    blocks_x = cells_x - (CV_HOG_BLOCK_CELLS - 1u);
    (void)blocks_x;

    for (by = 0u; by + CV_HOG_BLOCK_CELLS <= cells_y; ++by) {
        size_t bx;
        for (bx = 0u; bx + CV_HOG_BLOCK_CELLS <= cells_x; ++bx) {
            float block[CV_HOG_BLOCK_SIZE];
            size_t idx = 0u;
            double norm_sq = 0.0;
            float inv_norm;
            uint16_t r;
            uint16_t c;
            size_t k;

            /* Gather the 2x2 cell histograms of this block, row-major. */
            for (r = 0u; r < CV_HOG_BLOCK_CELLS; ++r) {
                for (c = 0u; c < CV_HOG_BLOCK_CELLS; ++c) {
                    int32_t cell_x0 =
                        roi.x + (int32_t)((bx + c) * config->cell_size);
                    int32_t cell_y0 =
                        roi.y + (int32_t)((by + r) * config->cell_size);
                    hog_cell_histogram(src, cell_x0, cell_y0, config->cell_size,
                                       &block[idx]);
                    idx += CV_HOG_BINS;
                }
            }

            /* L2-Hys: normalize, clip, renormalize. */
            for (k = 0u; k < CV_HOG_BLOCK_SIZE; ++k) {
                norm_sq += (double)block[k] * (double)block[k];
            }
            inv_norm = 1.0f / sqrtf((float)norm_sq + CV_HOG_EPS * CV_HOG_EPS);
            for (k = 0u; k < CV_HOG_BLOCK_SIZE; ++k) {
                float v = block[k] * inv_norm;
                if (v > config->l2_hys_clip) {
                    v = config->l2_hys_clip;
                }
                block[k] = v;
            }
            norm_sq = 0.0;
            for (k = 0u; k < CV_HOG_BLOCK_SIZE; ++k) {
                norm_sq += (double)block[k] * (double)block[k];
            }
            inv_norm = 1.0f / sqrtf((float)norm_sq + CV_HOG_EPS * CV_HOG_EPS);
            for (k = 0u; k < CV_HOG_BLOCK_SIZE; ++k) {
                descriptor[out + k] = block[k] * inv_norm;
            }
            out += CV_HOG_BLOCK_SIZE;
        }
    }

    *out_length = out;
    return EMBEDDIP_OK;
}

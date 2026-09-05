// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/track_hist.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

/** Bhattacharyya exponent scale; tuned so identical hists -> >0.99 and
 * disjoint hists -> <0.8 (see tests/test_cv_track_hist.c). */
#define CV_HIST_BHATTA_K 5.0f

embeddip_status_t cv_hist_build(const ImageView *img, Rectangle roi, float *out,
                                 uint32_t *out_nbins)
{
    if (img == NULL || out == NULL || out_nbins == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    bool is_gray = (img->format == IMAGE_FORMAT_GRAYSCALE || img->format == IMAGE_FORMAT_MASK) &&
                   img->depth == IMAGE_DEPTH_U8;
    bool is_rgb565 = img->format == IMAGE_FORMAT_RGB565 && img->depth == IMAGE_DEPTH_U16;
    if (!is_gray && !is_rgb565) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    /* Clamp roi to image bounds. */
    int32_t x0 = roi.x < 0 ? 0 : roi.x;
    int32_t y0 = roi.y < 0 ? 0 : roi.y;
    int32_t x1 = roi.x + roi.width;
    int32_t y1 = roi.y + roi.height;
    if (x1 > (int32_t)img->width) {
        x1 = (int32_t)img->width;
    }
    if (y1 > (int32_t)img->height) {
        y1 = (int32_t)img->height;
    }
    if (x1 <= x0 || y1 <= y0) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    uint32_t nbins = is_gray ? CV_HIST_GRAY_BINS : CV_HIST_COLOR_BINS;
    memset(out, 0, nbins * sizeof(float));

    uint32_t total = 0;
    for (int32_t y = y0; y < y1; y++) {
        const uint8_t *row = img->pixels + (uint32_t)y * img->row_stride_bytes;
        if (is_gray) {
            for (int32_t x = x0; x < x1; x++) {
                out[cv_hist_gray_bin(row[x])]++;
                total++;
            }
        } else {
            const uint16_t *row16 = (const uint16_t *)row;
            for (int32_t x = x0; x < x1; x++) {
                uint16_t px = row16[x];
                out[cv_hist_rgb565_bin_r(px)]++;
                out[cv_hist_rgb565_bin_g(px)]++;
                out[cv_hist_rgb565_bin_b(px)]++;
                total += 3;
            }
        }
    }

    if (total > 0) {
        float inv_total = 1.0f / (float)total;
        for (uint32_t i = 0; i < nbins; i++) {
            out[i] *= inv_total;
        }
    }

    *out_nbins = nbins;
    return EMBEDDIP_OK;
}

float cv_hist_bhattacharyya(const float *p, const float *q, uint32_t nbins)
{
    if (p == NULL || q == NULL || nbins == 0u) {
        return 0.0f;
    }

    float bc = 0.0f;
    for (uint32_t i = 0; i < nbins; i++) {
        bc += sqrtf(p[i] * q[i]);
    }

    float d = sqrtf(fmaxf(0.0f, 1.0f - bc));
    return expf(-CV_HIST_BHATTA_K * d);
}

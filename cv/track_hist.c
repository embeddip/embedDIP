// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/track_hist.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "cv/image_gray.h"

/** Bhattacharyya exponent scale; tuned so identical hists -> >0.99 and
 * disjoint hists -> <0.8 (see tests/test_cv_track_hist.c). */
#define CV_HIST_BHATTA_K 5.0f

embeddip_status_t
cv_hist_build(const ImageView *img, Rectangle roi, float *out, uint32_t *out_nbins)
{
    if (img == NULL || out == NULL || out_nbins == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!cv_format_is_gray_or_rgb565(img->format)) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }
    bool is_gray = cv_format_is_gray(img->format);
    if (img->depth != (is_gray ? IMAGE_DEPTH_U8 : IMAGE_DEPTH_U16)) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    /* Clamp roi to image bounds. */
    Rectangle clamped;
    if (!cv_clamp_roi(roi, img->width, img->height, &clamped)) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    int32_t x0 = clamped.x;
    int32_t y0 = clamped.y;
    int32_t x1 = clamped.x + clamped.width;
    int32_t y1 = clamped.y + clamped.height;

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

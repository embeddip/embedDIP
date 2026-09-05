// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/tracker_meanshift.h"

#include <math.h>

#include "cv/image_gray.h"

/** Default mode-seek iteration cap; converges well within this for the
 * shift magnitudes expected between consecutive frames. */
#define CV_MEANSHIFT_DEFAULT_MAX_ITERS 5u

static bool is_supported_format(const ImageView *img)
{
    if (!cv_format_is_gray_or_rgb565(img->format)) {
        return false;
    }
    return cv_format_is_gray(img->format) ? img->depth == IMAGE_DEPTH_U8
                                          : img->depth == IMAGE_DEPTH_U16;
}

/** Grayscale/mask bin index for pixel (x,y); uses cv/track_hist's mapping. */
static uint32_t gray_pixel_bin(const ImageView *img, int32_t x, int32_t y)
{
    const uint8_t *row = img->pixels + (uint32_t)y * img->row_stride_bytes;
    return cv_hist_gray_bin(row[x]);
}

embeddip_status_t cv_meanshift_init(CvMeanShiftState *state, const ImageView *frame, Rectangle roi)
{
    if (state == NULL || frame == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!is_supported_format(frame)) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }
    if (roi.width <= 0 || roi.height <= 0) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    /* Clamp roi to frame bounds first, matching cv_hist_build's own clamp,
     * so the stored box geometry always matches the region the histogram
     * was built over (and always fits inside the frame). */
    if (!cv_clamp_roi(roi, frame->width, frame->height, &roi)) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    embeddip_status_t st = cv_hist_build(frame, roi, state->template_hist, &state->hist_nbins);
    if (st != EMBEDDIP_OK) {
        return st;
    }

    state->box_width = roi.width;
    state->box_height = roi.height;
    state->center_x = roi.x + roi.width / 2;
    state->center_y = roi.y + roi.height / 2;
    state->max_iters = CV_MEANSHIFT_DEFAULT_MAX_ITERS;
    state->initialized = true;
    return EMBEDDIP_OK;
}

embeddip_status_t
cv_meanshift_update(CvMeanShiftState *state, const ImageView *frame, Rectangle *out_box)
{
    if (state == NULL || frame == NULL || out_box == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!state->initialized) {
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }
    if (!is_supported_format(frame)) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    bool is_gray = cv_format_is_gray(frame->format);
    int32_t hw = state->box_width / 2;
    int32_t hh = state->box_height / 2;
    /* ponytail: search box padded to 2x the tracked box so a shift of up to
     * one box dimension per frame is still visible to the centroid; widen
     * further if the tracked object can move faster between frames. */
    int32_t search_hw = state->box_width;
    int32_t search_hh = state->box_height;

    for (uint16_t iter = 0; iter < state->max_iters; iter++) {
        int32_t sx0 = state->center_x - search_hw;
        int32_t sy0 = state->center_y - search_hh;
        int32_t sx1 = state->center_x + search_hw;
        int32_t sy1 = state->center_y + search_hh;
        if (sx0 < 0)
            sx0 = 0;
        if (sy0 < 0)
            sy0 = 0;
        if (sx1 > (int32_t)frame->width)
            sx1 = (int32_t)frame->width;
        if (sy1 > (int32_t)frame->height)
            sy1 = (int32_t)frame->height;

        double sum_x = 0.0, sum_y = 0.0, sum_w = 0.0;
        for (int32_t y = sy0; y < sy1; y++) {
            for (int32_t x = sx0; x < sx1; x++) {
                if (is_gray) {
                    uint32_t bin = gray_pixel_bin(frame, x, y);
                    float w = sqrtf(state->template_hist[bin]);
                    sum_x += (double)x * w;
                    sum_y += (double)y * w;
                    sum_w += (double)w;
                } else {
                    const uint16_t *row16 =
                        (const uint16_t *)(frame->pixels + (uint32_t)y * frame->row_stride_bytes);
                    uint16_t px = row16[x];
                    float w = sqrtf(state->template_hist[cv_hist_rgb565_bin_r(px)]) +
                              sqrtf(state->template_hist[cv_hist_rgb565_bin_g(px)]) +
                              sqrtf(state->template_hist[cv_hist_rgb565_bin_b(px)]);
                    sum_x += (double)x * w;
                    sum_y += (double)y * w;
                    sum_w += (double)w;
                }
            }
        }

        if (sum_w <= 0.0) {
            break;
        }

        int32_t new_cx = (int32_t)lround(sum_x / sum_w);
        int32_t new_cy = (int32_t)lround(sum_y / sum_w);
        double dx = (double)(new_cx - state->center_x);
        double dy = (double)(new_cy - state->center_y);
        state->center_x = new_cx;
        state->center_y = new_cy;

        if ((dx * dx + dy * dy) < 1.0) {
            break;
        }
    }

    /* Clamp so the box stays inside the frame. */
    if (state->center_x - hw < 0)
        state->center_x = hw;
    if (state->center_y - hh < 0)
        state->center_y = hh;
    if (state->center_x + hw > (int32_t)frame->width)
        state->center_x = (int32_t)frame->width - hw;
    if (state->center_y + hh > (int32_t)frame->height)
        state->center_y = (int32_t)frame->height - hh;

    out_box->x = state->center_x - hw;
    out_box->y = state->center_y - hh;
    out_box->width = state->box_width;
    out_box->height = state->box_height;
    return EMBEDDIP_OK;
}

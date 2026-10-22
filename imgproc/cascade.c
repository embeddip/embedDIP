// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cascade.h"

embeddip_status_t computeIntegralImage(const Image *src, uint32_t *integral)
{
    if (!src || !src->pixels || !integral)
        return EMBEDDIP_ERROR_NULL_PTR;

    const uint8_t *buf = (const uint8_t *)src->pixels;
    int width = (int)src->width;
    int height = (int)src->height;
    int stride = width + 1;

    for (int x = 0; x <= width; ++x)
        integral[x] = 0;
    for (int y = 1; y <= height; ++y) {
        integral[y * stride] = 0;
        uint32_t row_sum = 0;
        for (int x = 1; x <= width; ++x) {
            row_sum += buf[(y - 1) * width + (x - 1)];
            integral[y * stride + x] = integral[(y - 1) * stride + x] + row_sum;
        }
    }
    return EMBEDDIP_OK;
}

static uint32_t rectSum(const uint32_t *integral, int stride, int x, int y, int w, int h)
{
    return integral[(y + h) * stride + (x + w)] - integral[y * stride + (x + w)] -
           integral[(y + h) * stride + x] + integral[y * stride + x];
}

static int cascadePass(const uint32_t *integral, int stride, int x, int y, int window, int32_t split_threshold)
{
    int half = window / 2;
    int area = half * window;
    if (area <= 0)
        return 0;

    uint32_t left = rectSum(integral, stride, x, y, half, window);
    uint32_t right = rectSum(integral, stride, x + half, y, half, window);
    int32_t feature1 = ((int32_t)left - (int32_t)right) / area;
    if (feature1 > -split_threshold && feature1 < split_threshold)
        return 0; /* stage 1: no clear split */

    uint32_t top = rectSum(integral, stride, x, y, window, half);
    uint32_t bottom = rectSum(integral, stride, x, y + half, window, half);
    int32_t feature2 = ((int32_t)top - (int32_t)bottom) / area;
    if (feature2 > -split_threshold && feature2 < split_threshold)
        return 0; /* stage 2: no clear split */

    return 1;
}

embeddip_status_t detectHaarCascade(const uint32_t *integral,
                                    uint32_t integral_width,
                                    uint32_t integral_height,
                                    int window,
                                    int step,
                                    int32_t split_threshold,
                                    Rectangle *detections,
                                    int max_detections,
                                    int *num_detections)
{
    if (!integral || !detections || !num_detections)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (window <= 0 || step <= 0 || max_detections <= 0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    int stride = (int)integral_width + 1;
    int width = (int)integral_width;
    int height = (int)integral_height;
    int n = 0;

    for (int y = 0; y <= height - window && n < max_detections; y += step) {
        for (int x = 0; x <= width - window && n < max_detections; x += step) {
            if (cascadePass(integral, stride, x, y, window, split_threshold)) {
                detections[n].x = x;
                detections[n].y = y;
                detections[n].width = window;
                detections[n].height = window;
                n++;
            }
        }
    }
    *num_detections = n;
    return EMBEDDIP_OK;
}

embeddip_status_t nonMaxSuppressRects(Rectangle *detections,
                                      int num_detections,
                                      int merge_dist,
                                      int *num_kept)
{
    if (!detections || !num_kept)
        return EMBEDDIP_ERROR_NULL_PTR;

    int kept = 0;
    for (int i = 0; i < num_detections; ++i) {
        int cx = detections[i].x + detections[i].width / 2;
        int cy = detections[i].y + detections[i].height / 2;
        int duplicate = 0;

        for (int j = 0; j < kept; ++j) {
            int kcx = detections[j].x + detections[j].width / 2;
            int kcy = detections[j].y + detections[j].height / 2;
            int dx = cx - kcx, dy = cy - kcy;
            if (dx * dx + dy * dy <= merge_dist * merge_dist) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate)
            detections[kept++] = detections[i];
    }
    *num_kept = kept;
    return EMBEDDIP_OK;
}

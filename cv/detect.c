// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/detect.h"

#include <stddef.h>
#include <stdint.h>

embeddip_status_t cv_detect_scan(const CvIntegralU32 *table,
                                 const CvHaarCascade *cascade,
                                 const CvScanConfig *scan, CvDetection *out,
                                 size_t out_capacity, size_t *out_count)
{
    int64_t max_x;
    int64_t max_y;
    int64_t y;

    if (table == NULL || cascade == NULL || scan == NULL || out == NULL ||
        out_count == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (scan->window_width == 0u || scan->window_height == 0u || scan->step_x == 0u ||
        scan->step_y == 0u) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (*out_count > out_capacity) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    /* Window larger than the table cannot be scanned; report zero new passes. */
    if ((uint64_t)scan->window_width > (uint64_t)table->width ||
        (uint64_t)scan->window_height > (uint64_t)table->height) {
        return EMBEDDIP_OK;
    }

    max_x = (int64_t)table->width - (int64_t)scan->window_width;
    max_y = (int64_t)table->height - (int64_t)scan->window_height;

    for (y = 0; y <= max_y; y += (int64_t)scan->step_y) {
        int64_t x;
        for (x = 0; x <= max_x; x += (int64_t)scan->step_x) {
            bool detected = false;
            int32_t score = 0;
            embeddip_status_t status;

            status = cv_haar_cascade_score(table, (int32_t)x, (int32_t)y, cascade,
                                           &detected, &score);
            if (status != EMBEDDIP_OK) {
                return status;
            }
            if (!detected) {
                continue;
            }
            if (*out_count >= out_capacity) {
                return EMBEDDIP_OK; /* buffer full: stop, keep what we have */
            }
            out[*out_count].box.x = (int32_t)x;
            out[*out_count].box.y = (int32_t)y;
            out[*out_count].box.width = (int32_t)scan->window_width;
            out[*out_count].box.height = (int32_t)scan->window_height;
            out[*out_count].score = score;
            ++(*out_count);
        }
    }

    return EMBEDDIP_OK;
}

static float detect_iou(const Rectangle *a, const Rectangle *b)
{
    int32_t ax1 = a->x;
    int32_t ay1 = a->y;
    int32_t ax2 = a->x + a->width;
    int32_t ay2 = a->y + a->height;
    int32_t bx1 = b->x;
    int32_t by1 = b->y;
    int32_t bx2 = b->x + b->width;
    int32_t by2 = b->y + b->height;

    int32_t ix1 = (ax1 > bx1) ? ax1 : bx1;
    int32_t iy1 = (ay1 > by1) ? ay1 : by1;
    int32_t ix2 = (ax2 < bx2) ? ax2 : bx2;
    int32_t iy2 = (ay2 < by2) ? ay2 : by2;

    int64_t iw = (int64_t)ix2 - (int64_t)ix1;
    int64_t ih = (int64_t)iy2 - (int64_t)iy1;
    int64_t inter;
    int64_t area_a;
    int64_t area_b;
    int64_t uni;

    if (iw <= 0 || ih <= 0) {
        return 0.0f;
    }
    inter = iw * ih;
    area_a = (int64_t)a->width * (int64_t)a->height;
    area_b = (int64_t)b->width * (int64_t)b->height;
    uni = area_a + area_b - inter;
    if (uni <= 0) {
        return 0.0f;
    }
    return (float)((double)inter / (double)uni);
}

embeddip_status_t cv_detect_nms(CvDetection *detections, size_t count,
                                float iou_threshold, size_t *out_kept)
{
    size_t i;
    size_t kept = 0u;

    if (detections == NULL || out_kept == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (iou_threshold < 0.0f || iou_threshold > 1.0f) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (count == 0u) {
        *out_kept = 0u;
        return EMBEDDIP_OK;
    }

    /* Stable selection sort by descending score; ties keep earlier index. */
    for (i = 0u; i + 1u < count; ++i) {
        size_t best = i;
        size_t j;
        for (j = i + 1u; j < count; ++j) {
            if (detections[j].score > detections[best].score) {
                best = j;
            }
        }
        if (best != i) {
            /* Rotate [i..best] right by one to preserve order of equal scores. */
            CvDetection tmp = detections[best];
            size_t k;
            for (k = best; k > i; --k) {
                detections[k] = detections[k - 1u];
            }
            detections[i] = tmp;
        }
    }

    /* Greedy suppression: keep a box unless it overlaps a kept box too much. */
    for (i = 0u; i < count; ++i) {
        size_t s;
        int suppressed = 0;
        for (s = 0u; s < kept; ++s) {
            if (detect_iou(&detections[i].box, &detections[s].box) > iou_threshold) {
                suppressed = 1;
                break;
            }
        }
        if (!suppressed) {
            if (kept != i) {
                detections[kept] = detections[i];
            }
            ++kept;
        }
    }

    *out_kept = kept;
    return EMBEDDIP_OK;
}

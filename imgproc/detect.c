// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "detect.h"

#include <math.h>

static float sigmoidf_(float x)
{
    return 1.0f / (1.0f + expf(-x));
}

float boxIoU(const ScoredBox *a, const ScoredBox *b)
{
    float ax2 = a->x + a->w, ay2 = a->y + a->h;
    float bx2 = b->x + b->w, by2 = b->y + b->h;
    float ix1 = fmaxf(a->x, b->x), iy1 = fmaxf(a->y, b->y);
    float ix2 = fminf(ax2, bx2), iy2 = fminf(ay2, by2);
    float iw = fmaxf(0.0f, ix2 - ix1), ih = fmaxf(0.0f, iy2 - iy1);
    float inter = iw * ih;
    float uni = a->w * a->h + b->w * b->h - inter;
    return (uni > 0.0f) ? (inter / uni) : 0.0f;
}

embeddip_status_t decodeFomoGrid(const int8_t *output,
                                 int grid,
                                 float scale,
                                 int8_t zero_point,
                                 float conf_threshold,
                                 ScoredBox *detections,
                                 int max_detections,
                                 int *num_detections)
{
    if (!output || !detections || !num_detections)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (grid <= 0 || max_detections <= 0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    int n = 0;
    for (int r = 0; r < grid; ++r) {
        for (int c = 0; c < grid && n < max_detections; ++c) {
            const int8_t *q = output + (r * grid + c) * 6;
            float cf = sigmoidf_((q[4] - zero_point) * scale) * sigmoidf_((q[5] - zero_point) * scale);
            if (cf < conf_threshold)
                continue;

            float bw = expf((q[2] - zero_point) * scale) / grid;
            float bh = expf((q[3] - zero_point) * scale) / grid;
            detections[n].x = (c + sigmoidf_((q[0] - zero_point) * scale)) / grid - bw / 2.0f;
            detections[n].y = (r + sigmoidf_((q[1] - zero_point) * scale)) / grid - bh / 2.0f;
            detections[n].w = bw;
            detections[n].h = bh;
            detections[n].score = cf;
            n++;
        }
    }
    *num_detections = n;
    return EMBEDDIP_OK;
}

embeddip_status_t nonMaxSuppressBoxes(ScoredBox *boxes, int num_boxes, float iou_threshold, int *num_kept)
{
    if (!boxes || !num_kept)
        return EMBEDDIP_ERROR_NULL_PTR;

    int kept = 0;
    for (int i = 0; i < num_boxes; ++i) {
        int best = -1;
        for (int j = 0; j < num_boxes; ++j) {
            if (boxes[j].score < 0.0f)
                continue; /* already consumed */
            if (best < 0 || boxes[j].score > boxes[best].score)
                best = j;
        }
        if (best < 0)
            break;

        int suppressed = 0;
        for (int k = 0; k < kept; ++k) {
            if (boxIoU(&boxes[best], &boxes[k]) >= iou_threshold) {
                suppressed = 1;
                break;
            }
        }
        ScoredBox picked = boxes[best];
        boxes[best].score = -1.0f; /* mark consumed */
        if (!suppressed)
            boxes[kept++] = picked;
    }
    *num_kept = kept;
    return EMBEDDIP_OK;
}

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "hog.h"

#include <math.h>

#define EMBEDDIP_KNN_MAX_SAMPLES 256
#define EMBEDDIP_KNN_MAX_CLASSES 64

static int sampleClamped(const uint8_t *buf, int width, int height, int x, int y)
{
    if (x < 0)
        x = 0;
    if (x >= width)
        x = width - 1;
    if (y < 0)
        y = 0;
    if (y >= height)
        y = height - 1;
    return buf[y * width + x];
}

embeddip_status_t computeHogDescriptor(const Image *src, int cells, int bins, float *descriptor)
{
    if (!src || !src->pixels || !descriptor)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (cells <= 0 || bins <= 0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    const uint8_t *buf = (const uint8_t *)src->pixels;
    int width = (int)src->width;
    int height = (int)src->height;
    int cell_w = width / cells;
    int cell_h = height / cells;
    if (cell_w <= 0 || cell_h <= 0)
        return EMBEDDIP_ERROR_INVALID_SIZE;

    for (int i = 0; i < cells * cells * bins; ++i)
        descriptor[i] = 0.0f;

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            float gx = (float)sampleClamped(buf, width, height, x + 1, y) -
                       (float)sampleClamped(buf, width, height, x - 1, y);
            float gy = (float)sampleClamped(buf, width, height, x, y + 1) -
                       (float)sampleClamped(buf, width, height, x, y - 1);
            float mag = fabsf(gx) + fabsf(gy);
            if (mag < 1.0f)
                continue;

            float ang = atan2f(gy, gx);
            if (ang < 0.0f)
                ang += (float)M_PI;
            int bin = (int)(ang / ((float)M_PI / bins));
            if (bin >= bins)
                bin = bins - 1;

            int cell_x = x / cell_w;
            if (cell_x >= cells)
                cell_x = cells - 1;
            int cell_y = y / cell_h;
            if (cell_y >= cells)
                cell_y = cells - 1;

            descriptor[(cell_y * cells + cell_x) * bins + bin] += mag;
        }
    }
    return EMBEDDIP_OK;
}

embeddip_status_t knnClassify(const float *query,
                              const float *train_set,
                              const int *labels,
                              int num_samples,
                              int desc_len,
                              int k,
                              int num_classes,
                              int *out_label)
{
    if (!query || !train_set || !labels || !out_label)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (num_samples <= 0 || num_samples > EMBEDDIP_KNN_MAX_SAMPLES || desc_len <= 0 || k <= 0 ||
        k > num_samples || num_classes <= 0 || num_classes > EMBEDDIP_KNN_MAX_CLASSES)
        return EMBEDDIP_ERROR_INVALID_ARG;

    float dist[EMBEDDIP_KNN_MAX_SAMPLES];
    for (int s = 0; s < num_samples; ++s) {
        float sum = 0.0f;
        const float *row = &train_set[s * desc_len];
        for (int b = 0; b < desc_len; ++b) {
            float d = query[b] - row[b];
            sum += d * d;
        }
        dist[s] = sum;
    }

    int votes[EMBEDDIP_KNN_MAX_CLASSES] = {0};
    for (int kk = 0; kk < k; ++kk) {
        int best = -1;
        for (int s = 0; s < num_samples; ++s) {
            if (best < 0 || dist[s] < dist[best])
                best = s;
        }
        votes[labels[best]]++;
        dist[best] = 1e30f; /* remove from further consideration */
    }

    int best_label = 0;
    for (int c = 1; c < num_classes; ++c) {
        if (votes[c] > votes[best_label])
            best_label = c;
    }
    *out_label = best_label;
    return EMBEDDIP_OK;
}

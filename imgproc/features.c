// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "features.h"

#define EMBEDDIP_BRIEF_PAIRS EMBEDDIP_BRIEF_DESC_BITS

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

static int isFastCorner(const uint8_t *buf, int width, int height, int x, int y, uint8_t threshold)
{
    static const int8_t ring_dx[8] = {3, 2, -2, -3, -3, -2, 2, 3};
    static const int8_t ring_dy[8] = {0, 2, 2, 0, 0, -2, -2, 0};
    int c = sampleClamped(buf, width, height, x, y);
    int brighter = 0, darker = 0;

    for (int i = 0; i < 8; ++i) {
        int v = sampleClamped(buf, width, height, x + ring_dx[i], y + ring_dy[i]);
        if (v > c + threshold)
            brighter++;
        else if (v < c - threshold)
            darker++;
    }
    return (brighter >= 6) || (darker >= 6);
}

embeddip_status_t detectFastKeypoints(const Image *src,
                                      uint8_t threshold,
                                      Keypoint *keypoints,
                                      int max_keypoints,
                                      int *num_keypoints)
{
    if (!src || !src->pixels || !keypoints || !num_keypoints)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (src->width < 8 || src->height < 8 || max_keypoints <= 0)
        return EMBEDDIP_ERROR_INVALID_SIZE;

    const uint8_t *buf = (const uint8_t *)src->pixels;
    int width = (int)src->width;
    int height = (int)src->height;
    int n = 0;

    for (int y = 4; y < height - 4 && n < max_keypoints; ++y) {
        for (int x = 4; x < width - 4 && n < max_keypoints; ++x) {
            if (isFastCorner(buf, width, height, x, y, threshold)) {
                keypoints[n].x = (int16_t)x;
                keypoints[n].y = (int16_t)y;
                n++;
            }
        }
    }
    *num_keypoints = n;
    return EMBEDDIP_OK;
}

embeddip_status_t computeBriefDescriptors(const Image *src,
                                          const Keypoint *keypoints,
                                          int num_keypoints,
                                          uint32_t *descriptors)
{
    if (!src || !src->pixels || !keypoints || !descriptors)
        return EMBEDDIP_ERROR_NULL_PTR;

    const uint8_t *buf = (const uint8_t *)src->pixels;
    int width = (int)src->width;
    int height = (int)src->height;

    for (int k = 0; k < num_keypoints; ++k) {
        int kx = keypoints[k].x;
        int ky = keypoints[k].y;
        uint32_t desc = 0;

        for (int i = 0; i < EMBEDDIP_BRIEF_PAIRS; ++i) {
            int dx1 = (i * 7) % 9 - 4, dy1 = (i * 13) % 9 - 4;
            int dx2 = (i * 5 + 3) % 9 - 4, dy2 = (i * 11 + 2) % 9 - 4;
            int a = sampleClamped(buf, width, height, kx + dx1, ky + dy1);
            int b = sampleClamped(buf, width, height, kx + dx2, ky + dy2);
            if (a < b)
                desc |= (1u << i);
        }
        descriptors[k] = desc;
    }
    return EMBEDDIP_OK;
}

int hammingDistance32(uint32_t a, uint32_t b)
{
    uint32_t x = a ^ b;
    int count = 0;
    while (x) {
        count += (int)(x & 1u);
        x >>= 1;
    }
    return count;
}

embeddip_status_t matchDescriptors(const uint32_t *descA,
                                   int numA,
                                   const uint32_t *descB,
                                   int numB,
                                   int max_hamming,
                                   int *num_matches)
{
    if (!descA || !descB || !num_matches)
        return EMBEDDIP_ERROR_NULL_PTR;

    int matches = 0;
    for (int i = 0; i < numA; ++i) {
        int best = 33, best_j = -1;
        for (int j = 0; j < numB; ++j) {
            int d = hammingDistance32(descA[i], descB[j]);
            if (d < best) {
                best = d;
                best_j = j;
            }
        }
        if (best_j >= 0 && best <= max_hamming)
            matches++;
    }
    *num_matches = matches;
    return EMBEDDIP_OK;
}

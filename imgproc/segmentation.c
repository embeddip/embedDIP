// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "segmentation.h"

#include "board/common.h"
#include "core/error.h"
#include "core/image.h"
#include "core/memory_manager.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper macros */
#define MAX_ITER 10
#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))
#define STACK_SIZE 65536

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

/* ============================= Helper Functions ============================= */

/* Read pixel idx as normalized 3-vector in the *native* space.
   HSI: [H,S,I] in [0,1]  (hue wrap handled in distance)
   YUV: [Y,U,V] in [0,1]
   RGB: [R,G,B] in [0,1] (RGB565 unpacked) */
static inline void read_vec3_norm(const Image *img, int idx, float v[3])
{
    const uint8_t *p8 = (const uint8_t *)img->pixels;

    switch (img->format) {
    case IMAGE_FORMAT_HSI: {
        int off = idx * 3;
        v[0] = p8[off + 0] / 255.0f;
        v[1] = p8[off + 1] / 255.0f;
        v[2] = p8[off + 2] / 255.0f;
    } break;

    case IMAGE_FORMAT_YUV: {
        int off = idx * 3;
        v[0] = p8[off + 0] / 255.0f;
        v[1] = p8[off + 1] / 255.0f;
        v[2] = p8[off + 2] / 255.0f;
    } break;

    case IMAGE_FORMAT_RGB888: {
        int off = idx * 3;
        v[0] = p8[off + 0] / 255.0f;
        v[1] = p8[off + 1] / 255.0f;
        v[2] = p8[off + 2] / 255.0f;
    } break;

    case IMAGE_FORMAT_RGB565: {
        const uint16_t *p16 = (const uint16_t *)img->pixels;
        uint16_t px = p16[idx];
        float r = (float)((px >> 11) & 0x1F) / 31.0f;
        float g = (float)((px >> 5) & 0x3F) / 63.0f;
        float b = (float)(px & 0x1F) / 31.0f;
        v[0] = r;
        v[1] = g;
        v[2] = b;
    } break;

    default:
        v[0] = v[1] = v[2] = 0.0f;
        break;
    }
}

/* Write a normalized 3-vector (0..1) back to the image format. */
static inline void write_vec3_norm(Image *img, int idx, const float v[3])
{
    uint8_t *p8 = (uint8_t *)img->pixels;

    switch (img->format) {
    case IMAGE_FORMAT_HSI:
    case IMAGE_FORMAT_YUV:
    case IMAGE_FORMAT_RGB888: {
        int off = idx * 3;
        p8[off + 0] = (uint8_t)CLAMP((int)lrintf(v[0] * 255.0f), 0, 255);
        p8[off + 1] = (uint8_t)CLAMP((int)lrintf(v[1] * 255.0f), 0, 255);
        p8[off + 2] = (uint8_t)CLAMP((int)lrintf(v[2] * 255.0f), 0, 255);
    } break;

    case IMAGE_FORMAT_RGB565: {
        uint16_t *p16 = (uint16_t *)img->pixels;
        int r5 = CLAMP((int)lrintf(v[0] * 31.0f), 0, 31);
        int g6 = CLAMP((int)lrintf(v[1] * 63.0f), 0, 63);
        int b5 = CLAMP((int)lrintf(v[2] * 31.0f), 0, 31);
        uint16_t px = (uint16_t)((r5 << 11) | (g6 << 5) | (b5));
        p16[idx] = px;
    } break;

    default:
        break;
    }
}

/* Distance with special hue wrap for HSI; Euclidean otherwise.
   a,b are normalized 0..1 vectors in native space. */
static inline float vec_distance_native(const float a[3], const float b[3], ImageFormat fmt)
{
    if (fmt == IMAGE_FORMAT_HSI) {
        float dh_raw = fabsf(a[0] - b[0]);
        float dh = fminf(dh_raw, 1.0f - dh_raw);
        float ds = a[1] - b[1];
        float di = a[2] - b[2];
        return dh * dh + ds * ds + di * di; /* squared distance (faster) */
    } else {
        float d0 = a[0] - b[0];
        float d1 = a[1] - b[1];
        float d2 = a[2] - b[2];
        return d0 * d0 + d1 * d1 + d2 * d2; /* squared distance */
    }
}

/* Clamp/normalize center components to [0,1] after averaging. */
static inline void center_divide(float c[3], int cnt)
{
    if (cnt > 0) {
        float inv = 1.0f / (float)cnt;
        c[0] *= inv;
        c[1] *= inv;
        c[2] *= inv;
        c[0] = fminf(fmaxf(c[0], 0.0f), 1.0f);
        c[1] = fminf(fmaxf(c[1], 0.0f), 1.0f);
        c[2] = fminf(fmaxf(c[2], 0.0f), 1.0f);
    }
}

/* Distance between color vectors with special hue wrap for HSI */
static inline float color_distance(const float a[3], const float b[3], ImageFormat fmt)
{
    if (fmt == IMAGE_FORMAT_HSI) {
        /* Hue wrap-around on a[0]/b[0] assumed in [0,1] */
        float dh_raw = fabsf(a[0] - b[0]);
        float dh = fminf(dh_raw, 1.0f - dh_raw);
        float ds = a[1] - b[1];
        float di = a[2] - b[2];
        return sqrtf(dh * dh + ds * ds + di * di);
    } else {
        /* Straight Euclidean in native space (already normalized 0..1) */
        float d0 = a[0] - b[0];
        float d1 = a[1] - b[1];
        float d2 = a[2] - b[2];
        return sqrtf(d0 * d0 + d1 * d1 + d2 * d2);
    }
}

/* ============================= K-Means Segmentation ============================= */

/**
 * @brief Segments a grayscale image using K-means clustering.
 *
 * @param[in]  inImg   Pointer to input grayscale image.
 * @param[out] outImg  Pointer to output segmented image.
 * @param[in]  k       Number of clusters (e.g., 2 for foreground/background).
 */
embeddip_status_t grayscaleKMeans(const Image *src, Image *dst, int k)
{
    if (!src || !dst || !src->pixels || k <= 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    if (isChalsEmpty(dst)) {
        createChals(dst, 1);
        dst->is_chals = 1;
    }

    int size = src->size;
    const uint8_t *src_data = src->pixels;
    float *dst_data = dst->chals->ch[0];

    float *centers = (float *)memory_alloc(k * sizeof(float));
    int *labels = (int *)memory_alloc(size * sizeof(int));
    int *counts = (int *)memory_alloc(k * sizeof(int));
    float *sums = (float *)memory_alloc(k * sizeof(float));

    if (!centers || !labels || !counts || !sums) {
        // Cleanup any successful allocations
        if (centers)
            memory_free(centers);
        if (labels)
            memory_free(labels);
        if (counts)
            memory_free(counts);
        if (sums)
            memory_free(sums);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // Initialize cluster centers evenly spaced
    for (int i = 0; i < k; ++i)
        centers[i] = (255.0f / (k - 1)) * i;

    for (int iter = 0; iter < MAX_ITER; ++iter) {
        // Reset accumulators
        for (int j = 0; j < k; ++j) {
            counts[j] = 0;
            sums[j] = 0.0f;
        }

        // Assign labels
        for (int i = 0; i < size; ++i) {
            float minDist = 1e9f;
            int best = 0;

            for (int j = 0; j < k; ++j) {
                float dist = fabsf((float)src_data[i] - centers[j]);
                if (dist < minDist) {
                    minDist = dist;
                    best = j;
                }
            }

            // Safety clamp
            best = CLAMP(best, 0, k - 1);
            labels[i] = best;
            sums[best] += (float)src_data[i];
            counts[best]++;
        }

        // Update centers, reinitialize empty clusters
        for (int j = 0; j < k; ++j) {
            if (counts[j] > 0)
                centers[j] = sums[j] / counts[j];
            else
                centers[j] = (float)(rand() % 256);  // reinitialize randomly
        }
    }

    // Final image: assign cluster center as output
    for (int i = 0; i < size; ++i) {
        int c = CLAMP(labels[i], 0, k - 1);
        dst_data[i] = (float)centers[c];
    }

    dst->log = IMAGE_DATA_CH0;

    // Cleanup
    memory_free(centers);
    memory_free(labels);
    memory_free(counts);
    memory_free(sums);

    return EMBEDDIP_OK;
}

/**
 * @brief Segments an HSI image using K-means clustering.
 *
 * @param[in]  inImg   Pointer to input HSI image.
 * @param[out] outImg  Pointer to output segmented HSI image.
 * @param[in]  k       Number of clusters.
 */
embeddip_status_t colorKMeans_old(const Image *inImg, Image *outImg, int k)
{
    // Input validation
    if (!inImg || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!inImg->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (k <= 0 || k > 255) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    // Support RGB888, HSI, and YUV (all 3-channel formats)
    if (inImg->format != IMAGE_FORMAT_RGB888 && inImg->format != IMAGE_FORMAT_HSI &&
        inImg->format != IMAGE_FORMAT_YUV) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (outImg->format != inImg->format) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    int size = inImg->size;
    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    typedef struct {
        float c0, c1, c2;  // Generic: R/H/Y, G/S/U, B/I/V
    } color_t;

    color_t *centers = (color_t *)memory_alloc(k * sizeof(color_t));
    int *labels = (int *)memory_alloc(size * sizeof(int));
    int *counts = (int *)memory_alloc(k * sizeof(int));
    color_t *sums = (color_t *)memory_alloc(k * sizeof(color_t));

    if (!centers || !labels || !counts || !sums) {
        // Cleanup any successful allocations
        if (centers)
            memory_free(centers);
        if (labels)
            memory_free(labels);
        if (counts)
            memory_free(counts);
        if (sums)
            memory_free(sums);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // Check if format is HSI (needs hue wraparound)
    bool is_hsi = (inImg->format == IMAGE_FORMAT_HSI);

    // Initialize cluster centers evenly spaced in third channel (I/B/V)
    for (int i = 0; i < k; ++i) {
        centers[i].c0 = (float)(rand() % 256);
        centers[i].c1 = (float)(rand() % 256);
        centers[i].c2 = (255.0f / (k - 1)) * i;
    }

    for (int iter = 0; iter < MAX_ITER; ++iter) {
        // Reset sums and counts
        for (int j = 0; j < k; ++j) {
            counts[j] = 0;
            sums[j].c0 = sums[j].c1 = sums[j].c2 = 0.0f;
        }

        // Assignment step
        for (int i = 0; i < size; ++i) {
            int idx = i * 3;
            float ch0 = in[idx];
            float ch1 = in[idx + 1];
            float ch2 = in[idx + 2];

            float minDist = 1e9f;
            int best = 0;

            for (int j = 0; j < k; ++j) {
                float d0 = fabsf(ch0 - centers[j].c0);
                // HSI format: wraparound hue distance for first channel
                if (is_hsi && d0 > 128)
                    d0 = 256 - d0;

                float d1 = ch1 - centers[j].c1;
                float d2 = ch2 - centers[j].c2;

                float dist = d0 * d0 + d1 * d1 + d2 * d2;

                if (dist < minDist) {
                    minDist = dist;
                    best = j;
                }
            }

            labels[i] = best;
            sums[best].c0 += ch0;
            sums[best].c1 += ch1;
            sums[best].c2 += ch2;
            counts[best]++;
        }

        // Update step
        for (int j = 0; j < k; ++j) {
            if (counts[j] > 0) {
                centers[j].c0 = sums[j].c0 / counts[j];
                centers[j].c1 = sums[j].c1 / counts[j];
                centers[j].c2 = sums[j].c2 / counts[j];
            } else {
                // Reinitialize empty cluster randomly
                centers[j].c0 = (float)(rand() % 256);
                centers[j].c1 = (float)(rand() % 256);
                centers[j].c2 = (float)(rand() % 256);
            }
        }
    }

    // Final assignment - replace each pixel with its cluster center
    for (int i = 0; i < size; ++i) {
        int idx = i * 3;
        int c = CLAMP(labels[i], 0, k - 1);

        out[idx + 0] = (uint8_t)centers[c].c0;
        out[idx + 1] = (uint8_t)centers[c].c1;
        out[idx + 2] = (uint8_t)centers[c].c2;
    }

    memory_free(centers);
    memory_free(labels);
    memory_free(counts);
    memory_free(sums);

    return EMBEDDIP_OK;
}

/**
 * @brief Segments an image using K-means in its native color space.
 *
 * @param[in]  inImg   Input image
 * @param[out] outImg  Output segmented image (same format as input)
 * @param[in]  k       Number of clusters (>=1, <= number of pixels)
 */
embeddip_status_t colorKMeans(const Image *inImg, Image *outImg, int k)
{
    // Input validation
    if (!inImg || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!inImg->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (k <= 0 || k > 255) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    // Support RGB888, RGB565, HSI, and YUV
    if (inImg->format != IMAGE_FORMAT_RGB888 && inImg->format != IMAGE_FORMAT_RGB565 &&
        inImg->format != IMAGE_FORMAT_HSI && inImg->format != IMAGE_FORMAT_YUV) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->format != outImg->format) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    const int N = inImg->size;
    if (k > N)
        k = N;

    /* Buffers */
    int *labels = (int *)memory_alloc((size_t)N * sizeof(int));
    int *counts = (int *)memory_alloc((size_t)k * sizeof(int));
    float *centers = (float *)memory_alloc((size_t)k * 3 * sizeof(float)); /* k x 3 */
    float *sums = (float *)memory_alloc((size_t)k * 3 * sizeof(float));    /* k x 3 */

    if (!labels || !counts || !centers || !sums) {
        // Cleanup any successful allocations
        if (labels)
            memory_free(labels);
        if (counts)
            memory_free(counts);
        if (centers)
            memory_free(centers);
        if (sums)
            memory_free(sums);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    /* --- Initialization: pick k random pixels as initial centers --- */
    for (int j = 0; j < k; ++j) {
        int rnd = rand() % N;
        read_vec3_norm(inImg, rnd, &centers[3 * j]);
        /* Ensure numeric sanity */
        centers[3 * j + 0] = fminf(fmaxf(centers[3 * j + 0], 0.0f), 1.0f);
        centers[3 * j + 1] = fminf(fmaxf(centers[3 * j + 1], 0.0f), 1.0f);
        centers[3 * j + 2] = fminf(fmaxf(centers[3 * j + 2], 0.0f), 1.0f);
    }

    /* --- Iterations --- */
    for (int iter = 0; iter < MAX_ITER; ++iter) {
        /* reset sums/counts */
        for (int j = 0; j < k; ++j) {
            counts[j] = 0;
            sums[3 * j + 0] = sums[3 * j + 1] = sums[3 * j + 2] = 0.0f;
        }

        /* assignment */
        for (int i = 0; i < N; ++i) {
            float v[3];
            read_vec3_norm(inImg, i, v);

            float bestD = 1e30f;
            int bestC = 0;
            for (int j = 0; j < k; ++j) {
                float d = vec_distance_native(v, &centers[3 * j], inImg->format);
                if (d < bestD) {
                    bestD = d;
                    bestC = j;
                }
            }
            labels[i] = bestC;
            sums[3 * bestC + 0] += v[0];
            sums[3 * bestC + 1] += v[1];
            sums[3 * bestC + 2] += v[2];
            counts[bestC] += 1;
        }

        /* update */
        for (int j = 0; j < k; ++j) {
            if (counts[j] > 0) {
                float c[3] = {sums[3 * j + 0], sums[3 * j + 1], sums[3 * j + 2]};
                center_divide(c, counts[j]);
                centers[3 * j + 0] = c[0];
                centers[3 * j + 1] = c[1];
                centers[3 * j + 2] = c[2];
            } else {
                /* empty cluster: re-seed from a random pixel */
                int rnd = rand() % N;
                read_vec3_norm(inImg, rnd, &centers[3 * j]);
            }
        }
    }

    /* --- Write result: paint each pixel with its cluster center in SAME format --- */
    for (int i = 0; i < N; ++i) {
        int c = CLAMP(labels[i], 0, k - 1);
        write_vec3_norm(outImg, i, &centers[3 * c]);
    }

    memory_free(labels);
    memory_free(counts);
    memory_free(centers);
    memory_free(sums);

    return EMBEDDIP_OK;
}

/* ============================= Region Growing ============================= */

/**
 * @brief Segments a grayscale image using region growing with multiple seed points.
 *
 * @param[in]  inImg      Pointer to input grayscale image.
 * @param[out] outImg     Pointer to output binary image (0 for background, 255 for region).
 * @param[in]  seeds      Array of seed points.
 * @param[in]  numSeeds   Number of seed points.
 * @param[in]  tolerance  Intensity difference threshold for region inclusion.
 */
embeddip_status_t grayscaleRegionGrowing(const Image *inImg,
                                         Image *outImg,
                                         const Point *seeds,
                                         int numSeeds,
                                         uint8_t tolerance)
{
    // Input validation
    if (!inImg || !outImg || !seeds) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!inImg->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (inImg->format != IMAGE_FORMAT_GRAYSCALE || outImg->format != IMAGE_FORMAT_GRAYSCALE) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    if (numSeeds <= 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    const int width = inImg->width;
    const int height = inImg->height;
    const uint8_t *src = inImg->pixels;
    uint8_t *dst = outImg->pixels;
    memset(dst, 0, inImg->size);

    bool *visited = (bool *)memory_alloc(sizeof(bool) * inImg->size);
    if (!visited) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    memset(visited, 0, inImg->size);

    Point *stack = (Point *)memory_alloc(sizeof(Point) * STACK_SIZE);
    if (!stack) {
        memory_free(visited);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    int top = 0;
    int dx[4] = {0, -1, 1, 0};
    int dy[4] = {-1, 0, 0, 1};
    float regionMean = 0.0f;
    int regionCount = 0;

    // Global multi-seed initialization (single visited map and single adaptive model).
    for (int s = 0; s < numSeeds; ++s) {
        int seedX = seeds[s].x;
        int seedY = seeds[s].y;
        if (seedX < 0 || seedX >= width || seedY < 0 || seedY >= height)
            continue;

        int seedIndex = seedY * width + seedX;
        if (visited[seedIndex])
            continue;

        if (top >= STACK_SIZE) {
            memory_free(visited);
            memory_free(stack);
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }

        stack[top++] = seeds[s];
        visited[seedIndex] = true;
        dst[seedIndex] = 255;

        regionCount++;
        regionMean += ((float)src[seedIndex] - regionMean) / (float)regionCount;
    }

    if (regionCount == 0) {
        memory_free(visited);
        memory_free(stack);
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    while (top > 0) {
        Point p = stack[--top];

        for (int i = 0; i < 4; ++i) {
            int nx = p.x + dx[i];
            int ny = p.y + dy[i];
            int nidx = ny * width + nx;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height && !visited[nidx]) {
                uint8_t neighborValue = src[nidx];
                if (abs((int)neighborValue - (int)regionMean) <= tolerance) {
                    visited[nidx] = true;
                    dst[nidx] = 255;

                    regionCount++;
                    regionMean += ((float)neighborValue - regionMean) / (float)regionCount;

                    if (top >= STACK_SIZE) {
                        memory_free(visited);
                        memory_free(stack);
                        return EMBEDDIP_ERROR_OUT_OF_MEMORY;  // Stack overflow
                    }
                    stack[top++] = (Point){nx, ny};
                }
            }
        }
    }

    memory_free(visited);
    memory_free(stack);
    return EMBEDDIP_OK;
}

/**
 * @brief Performs region growing on an HSI image using color similarity.
 *
 * @param[in]  inImg      Pointer to input HSI image (IMAGE_FORMAT_HSI).
 * @param[out] outMask    Pointer to output HSI image
 * @param[in]  seedX      X coordinate of seed point.
 * @param[in]  seedY      Y coordinate of seed point.
 * @param[in]  tolerance  Threshold on HSI distance (e.g., 0.1–0.4 typical).
 */
embeddip_status_t
colorRegionGrowing_single(const Image *inImg, Image *outImg, int seedX, int seedY, float tolerance)
{
    // Input validation
    if (!inImg || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!inImg->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (inImg->format != IMAGE_FORMAT_HSI || outImg->format != IMAGE_FORMAT_HSI) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    const int width = inImg->width;
    const int height = inImg->height;

    // Validate seed coordinates
    if (seedX < 0 || seedX >= width || seedY < 0 || seedY >= height) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    const uint8_t *src = (const uint8_t *)inImg->pixels;
    uint8_t *dst = (uint8_t *)outImg->pixels;

    memset(dst, 0, inImg->size * 3);  // Set all HSI values in output to 0 (black)

    bool *visited = (bool *)memory_alloc(sizeof(bool) * inImg->size);
    if (!visited) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    memset(visited, 0, inImg->size);

    Point *stack = (Point *)memory_alloc(sizeof(Point) * STACK_SIZE);
    if (!stack) {
        memory_free(visited);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    int top = 0;

    int seedIndex = seedY * width + seedX;
    float regionMean[3];
    read_vec3_norm(inImg, seedIndex, regionMean);
    int regionCount = 1;

    stack[top++] = (Point){seedX, seedY};
    visited[seedIndex] = true;

    // Copy seed pixel to output
    dst[seedIndex * 3] = src[seedIndex * 3];
    dst[seedIndex * 3 + 1] = src[seedIndex * 3 + 1];
    dst[seedIndex * 3 + 2] = src[seedIndex * 3 + 2];

    const int dx[4] = {0, -1, 1, 0};
    const int dy[4] = {-1, 0, 0, 1};

    while (top > 0) {
        Point p = stack[--top];

        for (int d = 0; d < 4; ++d) {
            int nx = p.x + dx[d];
            int ny = p.y + dy[d];
            int nidx = ny * width + nx;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height && !visited[nidx]) {
                float v[3];
                read_vec3_norm(inImg, nidx, v);
                float dist = color_distance(v, regionMean, inImg->format);

                if (dist <= tolerance) {
                    visited[nidx] = true;

                    // Copy pixel to output
                    dst[nidx * 3] = src[nidx * 3];
                    dst[nidx * 3 + 1] = src[nidx * 3 + 1];
                    dst[nidx * 3 + 2] = src[nidx * 3 + 2];

                    // Update running region mean (adaptive region growing).
                    regionCount++;
                    regionMean[0] += (v[0] - regionMean[0]) / (float)regionCount;
                    regionMean[1] += (v[1] - regionMean[1]) / (float)regionCount;
                    regionMean[2] += (v[2] - regionMean[2]) / (float)regionCount;

                    stack[top++] = (Point){nx, ny};

                    if (top >= STACK_SIZE) {
                        memory_free(visited);
                        memory_free(stack);
                        return EMBEDDIP_ERROR_OUT_OF_MEMORY;  // Stack overflow
                    }
                }
            }
        }
    }

    memory_free(visited);
    memory_free(stack);
    return EMBEDDIP_OK;
}

/**
 * @brief Performs region growing using color similarity with multiple seeds.
 *
 * @param[in]  inImg      Pointer to input image.
 * @param[out] outImg     Pointer to output binary mask (1-channel).
 * @param[in]  seeds      Array of seed points.
 * @param[in]  numSeeds   Number of seed points.
 * @param[in]  tolerance  Threshold on color distance.
 */
embeddip_status_t colorRegionGrowing(const Image *inImg,
                                     Image *outImg,
                                     const Point *seeds,
                                     int numSeeds,
                                     float tolerance)
{
    // Input validation
    if (!inImg || !outImg || !seeds) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!inImg->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    bool outputColorful = (outImg->format == IMAGE_FORMAT_RGB888);

    if (!outputColorful && outImg->format != IMAGE_FORMAT_GRAYSCALE) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (!outputColorful && outImg->depth != 1) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    if (numSeeds <= 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    const int width = inImg->width;
    const int height = inImg->height;
    const int outDepth = outputColorful ? 3 : 1;
    const uint8_t *inData = (const uint8_t *)inImg->pixels;
    const int inDepth = inImg->depth;

    uint8_t *outData = (uint8_t *)outImg->pixels;
    /* Clear output */
    memset(outImg->pixels, 0, (size_t)(outImg->size * outDepth));

    /* Allocate visited */
    bool *visited = (bool *)memory_alloc((size_t)inImg->size * sizeof(bool));
    if (!visited) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    /* Allocate stack */
    Point *stack = (Point *)memory_alloc((size_t)STACK_SIZE * sizeof(Point));
    if (!stack) {
        memory_free(visited);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    const int dx[4] = {0, -1, 1, 0};
    const int dy[4] = {-1, 0, 0, 1};

    memset(visited, 0, (size_t)inImg->size * sizeof(bool));

    // Global multi-seed region: one visited map and one adaptive region model.
    int top = 0;
    float regionMean[3] = {0.0f, 0.0f, 0.0f};
    int regionCount = 0;

    for (int s = 0; s < numSeeds; ++s) {
        int seedX = seeds[s].x;
        int seedY = seeds[s].y;
        if ((unsigned)seedX >= (unsigned)width || (unsigned)seedY >= (unsigned)height)
            continue;

        int seedIndex = seedY * width + seedX;
        if (visited[seedIndex])
            continue;

        if (top >= STACK_SIZE) {
            memory_free(visited);
            memory_free(stack);
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
        stack[top++] = (Point){seedX, seedY};
        visited[seedIndex] = true;

        float v[3];
        read_vec3_norm(inImg, seedIndex, v);
        regionCount++;
        regionMean[0] += (v[0] - regionMean[0]) / (float)regionCount;
        regionMean[1] += (v[1] - regionMean[1]) / (float)regionCount;
        regionMean[2] += (v[2] - regionMean[2]) / (float)regionCount;

        if (outputColorful) {
            outData[seedIndex * 3 + 0] = (uint8_t)CLAMP((int)lrintf(v[0] * 255.0f), 0, 255);
            outData[seedIndex * 3 + 1] = (uint8_t)CLAMP((int)lrintf(v[1] * 255.0f), 0, 255);
            outData[seedIndex * 3 + 2] = (uint8_t)CLAMP((int)lrintf(v[2] * 255.0f), 0, 255);
        } else {
            outData[seedIndex] = 255;
        }
    }

    if (regionCount == 0) {
        memory_free(visited);
        memory_free(stack);
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    while (top > 0) {
        Point p = stack[--top];

        for (int d = 0; d < 4; ++d) {
            int nx = p.x + dx[d];
            int ny = p.y + dy[d];

            if ((unsigned)nx >= (unsigned)width || (unsigned)ny >= (unsigned)height)
                continue;

            int nidx = ny * width + nx;
            if (visited[nidx])
                continue;

            float v[3];
            read_vec3_norm(inImg, nidx, v);

            float dist = color_distance(v, regionMean, inImg->format);
            if (dist <= tolerance) {
                visited[nidx] = true;

                if (outputColorful) {
                    outData[nidx * 3 + 0] = (uint8_t)CLAMP((int)lrintf(v[0] * 255.0f), 0, 255);
                    outData[nidx * 3 + 1] = (uint8_t)CLAMP((int)lrintf(v[1] * 255.0f), 0, 255);
                    outData[nidx * 3 + 2] = (uint8_t)CLAMP((int)lrintf(v[2] * 255.0f), 0, 255);
                } else {
                    outData[nidx] = 255;
                }

                regionCount++;
                regionMean[0] += (v[0] - regionMean[0]) / (float)regionCount;
                regionMean[1] += (v[1] - regionMean[1]) / (float)regionCount;
                regionMean[2] += (v[2] - regionMean[2]) / (float)regionCount;

                if (top >= STACK_SIZE) {
                    memory_free(visited);
                    memory_free(stack);
                    return EMBEDDIP_ERROR_OUT_OF_MEMORY;  // Stack overflow
                }
                stack[top++] = (Point){nx, ny};
            }
        }
    }

    memory_free(visited);
    memory_free(stack);
    return EMBEDDIP_OK;
}

/* ============================= GrabCut Segmentation ============================= */

#define FOREGROUND 255
#define BACKGROUND 0
#define GMM_COMPONENTS 2
#define MAX_ITER_GRABCUT 5

typedef struct {
    float weight;
    float mean;
    float variance;
} GMMComponent;

static float gaussian_prob(float x, float mean, float var)
{
    float diff = x - mean;
    return (1.0f / sqrtf(2.0f * M_PI * var)) * expf(-(diff * diff) / (2.0f * var));
}

/**
 * @brief Performs a simplified GrabCut-inspired segmentation on a grayscale image using a
 * rectangular ROI.
 *
 * @param[in]  inImg      Input grayscale image.
 * @param[out] outImg     Output binary image (0 = background, 255 = foreground).
 * @param[in]  roi        Rectangleangular region of interest.
 * @param[in]  iterations Number of refinement iterations.
 */
embeddip_status_t grabCutLite(const Image *src, Image *mask, Rectangle roi, int iterations)
{
    // Input validation
    if (!src || !mask) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!src->pixels || !mask->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (src->format != IMAGE_FORMAT_GRAYSCALE) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (mask->format != IMAGE_FORMAT_MASK && mask->format != IMAGE_FORMAT_GRAYSCALE) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (src->width != mask->width || src->height != mask->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    if (iterations <= 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    const int width = src->width;
    const int height = src->height;
    const int size = width * height;
    const uint8_t *src_data = src->pixels;
    uint8_t *dst = mask->pixels;

    // Temporary mask: 0 = background, 1 = probable, 2 = foreground
    uint8_t *temp_mask = (uint8_t *)memory_alloc(size);
    if (!temp_mask) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // Initialize mask: ROI = probable, rest = background
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            if (x >= roi.x && x < (roi.x + roi.width) && y >= roi.y && y < (roi.y + roi.height)) {
                temp_mask[idx] = 1;  // Probable
            } else {
                temp_mask[idx] = 0;  // Background
            }
        }
    }

    // Iterative refinement
    for (int iter = 0; iter < iterations; ++iter) {
        uint32_t fgSum = 0, fgCount = 0;
        uint32_t bgSum = 0, bgCount = 0;

        // Compute FG/BG means based on confirmed pixels
        for (int i = 0; i < size; ++i) {
            if (temp_mask[i] == 2)  // FG
            {
                fgSum += src_data[i];
                fgCount++;
            } else if (temp_mask[i] == 0)  // BG
            {
                bgSum += src_data[i];
                bgCount++;
            }
        }

        // Bootstrap: if no FG yet, use probable region for FG stats
        if (fgCount == 0) {
            for (int i = 0; i < size; ++i) {
                if (temp_mask[i] == 1) {
                    fgSum += src_data[i];
                    fgCount++;
                }
            }
        }

        if (fgCount == 0 || bgCount == 0)
            break;

        uint8_t fgMean = (uint8_t)(fgSum / fgCount);
        uint8_t bgMean = (uint8_t)(bgSum / bgCount);

        // Reset ROI pixels back to "probable" each iteration
        for (int y = roi.y; y < roi.y + roi.height; ++y) {
            for (int x = roi.x; x < roi.x + roi.width; ++x) {
                int idx = y * width + x;
                temp_mask[idx] = 1;
            }
        }

        // Reclassify probable pixels based on updated means
        for (int i = 0; i < size; ++i) {
            if (temp_mask[i] == 1) {
                int dFg = abs((int)src_data[i] - (int)fgMean);
                int dBg = abs((int)src_data[i] - (int)bgMean);
                temp_mask[i] = (dFg < dBg) ? 2 : 0;
            }
        }
    }

    // Final binary output mask
    for (int i = 0; i < size; ++i) {
        dst[i] = (temp_mask[i] == 2) ? 255 : 0;
    }

    memory_free(temp_mask);
    mask->log = IMAGE_DATA_PIXELS;
    return EMBEDDIP_OK;
}

typedef struct {
    int to;
    int next;
    float cap;
} gc_edge_t;

typedef struct {
    int n;
    int source;
    int sink;
    int *head;
    gc_edge_t *edges;
    int edge_count;
    int edge_cap;
    int *level;
    int *it;
    int *queue;
    uint8_t *seen;
} gc_graph_t;

static int gc_init(gc_graph_t *g, int n, int edge_cap)
{
    g->n = n;
    g->source = n - 2;
    g->sink = n - 1;
    g->edge_count = 0;
    g->edge_cap = edge_cap;
    g->head = (int *)memory_alloc((size_t)n * sizeof(int));
    g->edges = (gc_edge_t *)memory_alloc((size_t)edge_cap * sizeof(gc_edge_t));
    g->level = (int *)memory_alloc((size_t)n * sizeof(int));
    g->it = (int *)memory_alloc((size_t)n * sizeof(int));
    g->queue = (int *)memory_alloc((size_t)n * sizeof(int));
    g->seen = (uint8_t *)memory_alloc((size_t)n);
    if (!g->head || !g->edges || !g->level || !g->it || !g->queue || !g->seen) {
        return -1;
    }
    for (int i = 0; i < n; ++i)
        g->head[i] = -1;
    return 0;
}

static void gc_free(gc_graph_t *g)
{
    if (g->head)
        memory_free(g->head);
    if (g->edges)
        memory_free(g->edges);
    if (g->level)
        memory_free(g->level);
    if (g->it)
        memory_free(g->it);
    if (g->queue)
        memory_free(g->queue);
    if (g->seen)
        memory_free(g->seen);
    memset(g, 0, sizeof(*g));
}

static int gc_add_edge(gc_graph_t *g, int u, int v, float cap)
{
    if (g->edge_count + 2 > g->edge_cap)
        return -1;
    g->edges[g->edge_count] = (gc_edge_t){.to = v, .next = g->head[u], .cap = cap};
    g->head[u] = g->edge_count++;
    g->edges[g->edge_count] = (gc_edge_t){.to = u, .next = g->head[v], .cap = 0.0f};
    g->head[v] = g->edge_count++;
    return 0;
}

static int gc_add_undirected(gc_graph_t *g, int u, int v, float cap)
{
    if (gc_add_edge(g, u, v, cap) != 0)
        return -1;
    if (gc_add_edge(g, v, u, cap) != 0)
        return -1;
    return 0;
}

static int gc_bfs(gc_graph_t *g)
{
    for (int i = 0; i < g->n; ++i)
        g->level[i] = -1;
    int qh = 0, qt = 0;
    g->level[g->source] = 0;
    g->queue[qt++] = g->source;
    while (qh < qt) {
        int u = g->queue[qh++];
        for (int ei = g->head[u]; ei != -1; ei = g->edges[ei].next) {
            gc_edge_t *e = &g->edges[ei];
            if (e->cap > 1e-6f && g->level[e->to] < 0) {
                g->level[e->to] = g->level[u] + 1;
                g->queue[qt++] = e->to;
            }
        }
    }
    return g->level[g->sink] >= 0;
}

static float gc_dfs(gc_graph_t *g, int u, float f)
{
    if (u == g->sink)
        return f;
    for (int *pei = &g->it[u]; *pei != -1; *pei = g->edges[*pei].next) {
        int ei = *pei;
        gc_edge_t *e = &g->edges[ei];
        if (e->cap <= 1e-6f || g->level[e->to] != g->level[u] + 1)
            continue;
        float pushed = gc_dfs(g, e->to, fminf(f, e->cap));
        if (pushed > 1e-6f) {
            e->cap -= pushed;
            g->edges[ei ^ 1].cap += pushed;
            return pushed;
        }
    }
    return 0.0f;
}

static float gc_maxflow(gc_graph_t *g)
{
    float flow = 0.0f;
    while (gc_bfs(g)) {
        for (int i = 0; i < g->n; ++i)
            g->it[i] = g->head[i];
        while (1) {
            float pushed = gc_dfs(g, g->source, 1e20f);
            if (pushed <= 1e-6f)
                break;
            flow += pushed;
        }
    }
    return flow;
}

static void gc_mark_source_side(gc_graph_t *g)
{
    memset(g->seen, 0, (size_t)g->n);
    int qh = 0, qt = 0;
    g->seen[g->source] = 1;
    g->queue[qt++] = g->source;
    while (qh < qt) {
        int u = g->queue[qh++];
        for (int ei = g->head[u]; ei != -1; ei = g->edges[ei].next) {
            gc_edge_t *e = &g->edges[ei];
            if (e->cap > 1e-6f && !g->seen[e->to]) {
                g->seen[e->to] = 1;
                g->queue[qt++] = e->to;
            }
        }
    }
}

embeddip_status_t grabCut(const Image *src, Image *mask, Rectangle roi, int max_iter)
{
    if (!src || !mask || !src->pixels || !mask->pixels)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (src->format != IMAGE_FORMAT_GRAYSCALE)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (mask->format != IMAGE_FORMAT_MASK && mask->format != IMAGE_FORMAT_GRAYSCALE)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (src->width != mask->width || src->height != mask->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (max_iter <= 0)
        max_iter = MAX_ITER_GRABCUT;

    const int width = (int)src->width;
    const int height = (int)src->height;
    const uint8_t *src_data = (const uint8_t *)src->pixels;
    uint8_t *mask_data = (uint8_t *)mask->pixels;
    memset(mask_data, BACKGROUND, (size_t)width * (size_t)height);

    int x0 = roi.x < 0 ? 0 : roi.x;
    int y0 = roi.y < 0 ? 0 : roi.y;
    int x1 = roi.x + roi.width;
    int y1 = roi.y + roi.height;
    if (x1 > width)
        x1 = width;
    if (y1 > height)
        y1 = height;
    if (x0 >= x1 || y0 >= y1)
        return EMBEDDIP_ERROR_INVALID_ARG;

    // Downsample ROI for embedded memory/perf while still using graph-cut.
    int ds = 2;
    const int target_max_nodes = 7000;
    int sw = (x1 - x0 + ds - 1) / ds;
    int sh = (y1 - y0 + ds - 1) / ds;
    while (sw * sh > target_max_nodes && ds < 16) {
        ds *= 2;
        sw = (x1 - x0 + ds - 1) / ds;
        sh = (y1 - y0 + ds - 1) / ds;
    }
    const int sn = sw * sh;
    uint8_t *labels = (uint8_t *)memory_alloc((size_t)sn);  // 0=BG, 1=FG
    uint8_t *small = (uint8_t *)memory_alloc((size_t)sn);
    if (!labels || !small) {
        if (labels)
            memory_free(labels);
        if (small)
            memory_free(small);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    for (int sy = 0; sy < sh; ++sy) {
        for (int sx = 0; sx < sw; ++sx) {
            int xx = x0 + sx * ds;
            int yy = y0 + sy * ds;
            if (xx >= width)
                xx = width - 1;
            if (yy >= height)
                yy = height - 1;
            small[sy * sw + sx] = src_data[yy * width + xx];
        }
    }

    int border = ((sw < sh) ? sw : sh) / 10;
    if (border < 2)
        border = 2;
    for (int sy = 0; sy < sh; ++sy) {
        for (int sx = 0; sx < sw; ++sx) {
            int near_left = sx < border;
            int near_right = (sw - 1 - sx) < border;
            int near_top = sy < border;
            int near_bottom = (sh - 1 - sy) < border;
            labels[sy * sw + sx] = (near_left || near_right || near_top || near_bottom) ? 0u : 1u;
        }
    }

    const float lambda = 25.0f;
    const float hard_cap = 1e6f;

    for (int iter = 0; iter < max_iter; ++iter) {
        float fg_sum = 0.0f, fg_sqsum = 0.0f, fg_cnt = 0.0f;
        float bg_sum = 0.0f, bg_sqsum = 0.0f, bg_cnt = 0.0f;
        for (int i = 0; i < sn; ++i) {
            float v = (float)small[i];
            if (labels[i]) {
                fg_sum += v;
                fg_sqsum += v * v;
                fg_cnt += 1.0f;
            } else {
                bg_sum += v;
                bg_sqsum += v * v;
                bg_cnt += 1.0f;
            }
        }
        if (fg_cnt < 1.0f || bg_cnt < 1.0f)
            break;

        float mu_fg = fg_sum / fg_cnt;
        float mu_bg = bg_sum / bg_cnt;
        float var_fg = fmaxf((fg_sqsum / fg_cnt) - mu_fg * mu_fg, 25.0f);
        float var_bg = fmaxf((bg_sqsum / bg_cnt) - mu_bg * mu_bg, 25.0f);

        float d2_sum = 0.0f;
        int d2_cnt = 0;
        for (int y = 0; y < sh; ++y) {
            for (int x = 0; x < sw; ++x) {
                int i = y * sw + x;
                if (x + 1 < sw) {
                    float d = (float)small[i] - (float)small[i + 1];
                    d2_sum += d * d;
                    d2_cnt++;
                }
                if (y + 1 < sh) {
                    float d = (float)small[i] - (float)small[i + sw];
                    d2_sum += d * d;
                    d2_cnt++;
                }
            }
        }
        float beta = 1.0f / (2.0f * (d2_sum / (float)(d2_cnt + 1)) + 1e-6f);

        gc_graph_t g = {0};
        int node_n = sn + 2;
        // Terminal edges: ~4*sn, n-links: ~4*((sw-1)*sh + sw*(sh-1)), plus border t-links.
        // Worst-case storage bound with current representation:
        // - terminal links: 2 add_edge/pixel => 4*sn edges
        // - smoothness links: 4 edges per right/down neighbor pair
        //   pairs = (sw-1)*sh + sw*(sh-1) = 2*sn - sw - sh
        //   => 4*(2*sn - sw - sh) edges
        // - hard-ring t-links: up to 1 add_edge/pixel in worst case => 2*sn edges
        // Total <= 14*sn - 4*(sw+sh), add safety margin.
        int edge_cap = 14 * sn + 512;
        if (gc_init(&g, node_n, edge_cap) != 0) {
            gc_free(&g);
            memory_free(labels);
            memory_free(small);
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }

        for (int y = 0; y < sh; ++y) {
            for (int x = 0; x < sw; ++x) {
                int p = y * sw + x;
                float pix = (float)small[p];
                float dbg = 0.5f * logf(var_bg) + ((pix - mu_bg) * (pix - mu_bg)) / (2.0f * var_bg);
                float dfg = 0.5f * logf(var_fg) + ((pix - mu_fg) * (pix - mu_fg)) / (2.0f * var_fg);
                if (dbg < 0.0f)
                    dbg = 0.0f;
                if (dfg < 0.0f)
                    dfg = 0.0f;

                if (gc_add_edge(&g, g.source, p, dbg + 1e-3f) != 0 ||
                    gc_add_edge(&g, p, g.sink, dfg + 1e-3f) != 0) {
                    gc_free(&g);
                    memory_free(labels);
                    memory_free(small);
                    return EMBEDDIP_ERROR_OUT_OF_MEMORY;
                }

                int near_left = x < border;
                int near_right = (sw - 1 - x) < border;
                int near_top = y < border;
                int near_bottom = (sh - 1 - y) < border;
                int is_ring = near_left || near_right || near_top || near_bottom;
                if (is_ring) {
                    if (gc_add_edge(&g, p, g.sink, hard_cap) != 0) {
                        gc_free(&g);
                        memory_free(labels);
                        memory_free(small);
                        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
                    }
                }

                if (x + 1 < sw) {
                    int q = p + 1;
                    float d = (float)small[p] - (float)small[q];
                    float w = lambda * expf(-beta * d * d) + 1e-3f;
                    if (gc_add_undirected(&g, p, q, w) != 0) {
                        gc_free(&g);
                        memory_free(labels);
                        memory_free(small);
                        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
                    }
                }
                if (y + 1 < sh) {
                    int q = p + sw;
                    float d = (float)small[p] - (float)small[q];
                    float w = lambda * expf(-beta * d * d) + 1e-3f;
                    if (gc_add_undirected(&g, p, q, w) != 0) {
                        gc_free(&g);
                        memory_free(labels);
                        memory_free(small);
                        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
                    }
                }
            }
        }

        gc_maxflow(&g);
        gc_mark_source_side(&g);
        for (int i = 0; i < sn; ++i) {
            labels[i] = g.seen[i] ? 1u : 0u;
        }
        gc_free(&g);
    }

    memset(mask_data, BACKGROUND, (size_t)width * (size_t)height);
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            int sx = (x - x0) / ds;
            int sy = (y - y0) / ds;
            if (sx >= sw)
                sx = sw - 1;
            if (sy >= sh)
                sy = sh - 1;
            int si = sy * sw + sx;
            mask_data[y * width + x] = labels[si] ? FOREGROUND : BACKGROUND;
        }
    }
    mask->log = IMAGE_DATA_PIXELS;

    memory_free(labels);
    memory_free(small);
    return EMBEDDIP_OK;
}

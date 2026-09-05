// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "imgproc/corner.h"

#include "core/memory_manager.h"

#include <math.h>
#include <stddef.h>

/* Sobel gradients into caller float buffers; 1-pixel border left at 0. */
static void sobel_gradients(const uint8_t *pix, uint32_t stride, int W, int H,
                            float *ix, float *iy)
{
    for (int i = 0; i < W * H; ++i) {
        ix[i] = 0.0f;
        iy[i] = 0.0f;
    }
    for (int y = 1; y < H - 1; ++y) {
        const uint8_t *r0 = pix + (uint32_t)(y - 1) * stride;
        const uint8_t *r1 = pix + (uint32_t)(y)*stride;
        const uint8_t *r2 = pix + (uint32_t)(y + 1) * stride;
        for (int x = 1; x < W - 1; ++x) {
            float gx = (float)(r0[x + 1] + 2 * r1[x + 1] + r2[x + 1]) -
                       (float)(r0[x - 1] + 2 * r1[x - 1] + r2[x - 1]);
            float gy = (float)(r2[x - 1] + 2 * r2[x] + r2[x + 1]) -
                       (float)(r0[x - 1] + 2 * r0[x] + r0[x + 1]);
            ix[y * W + x] = gx;
            iy[y * W + x] = gy;
        }
    }
}

embeddip_status_t cv_corner_detect(const ImageView *src, const CvCornerConfig *cfg,
                                   CvCorner *out, size_t out_capacity, size_t *out_count)
{
    if (!src || !cfg || !out || !out_count || !src->pixels)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (src->format != IMAGE_FORMAT_GRAYSCALE && src->format != IMAGE_FORMAT_MASK)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (src->depth != IMAGE_DEPTH_U8)
        return EMBEDDIP_ERROR_INVALID_DEPTH;
    if (cfg->block_size < 1 || (cfg->block_size % 2) == 0 || cfg->nms_radius < 1)
        return EMBEDDIP_ERROR_INVALID_ARG;
    if (out_capacity == 0)
        return EMBEDDIP_ERROR_INVALID_ARG;
    if (src->width < 3 || src->height < 3)
        return EMBEDDIP_ERROR_INVALID_SIZE;

    *out_count = 0;
    const int W = (int)src->width, H = (int)src->height;
    const int hb = cfg->block_size / 2;
    const int r = cfg->nms_radius;
    const size_t N = (size_t)W * (size_t)H;

    float *ix = (float *)memory_alloc(N * sizeof(float));
    float *iy = (float *)memory_alloc(N * sizeof(float));
    float *resp = (float *)memory_alloc(N * sizeof(float));
    if (!ix || !iy || !resp) {
        memory_free(ix);
        memory_free(iy);
        memory_free(resp);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    for (size_t i = 0; i < N; ++i)
        resp[i] = 0.0f;

    sobel_gradients((const uint8_t *)src->pixels, src->row_stride_bytes, W, H, ix, iy);

    /* Structure-tensor response over a block_size window. Valid region leaves a
     * (hb + 1) margin: hb for the window, +1 because Sobel zeros the border.
     * ponytail: naive O(N*block^2) window sum; add a float integral image if
     * block_size grows large. */
    const int x0 = hb + 1, x1 = W - 2 - hb;
    const int y0 = hb + 1, y1 = H - 2 - hb;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            float sxx = 0.0f, syy = 0.0f, sxy = 0.0f;
            for (int dy = -hb; dy <= hb; ++dy) {
                const float *rix = ix + (size_t)(y + dy) * W + x;
                const float *riy = iy + (size_t)(y + dy) * W + x;
                for (int dx = -hb; dx <= hb; ++dx) {
                    float fx = rix[dx], fy = riy[dx];
                    sxx += fx * fx;
                    syy += fy * fy;
                    sxy += fx * fy;
                }
            }
            float det = sxx * syy - sxy * sxy;
            float tr = sxx + syy;
            float R;
            if (cfg->method == CV_CORNER_SHI_TOMASI) {
                float disc = tr * tr - 4.0f * det;
                if (disc < 0.0f)
                    disc = 0.0f;
                R = 0.5f * (tr - sqrtf(disc)); /* smaller eigenvalue */
            } else {
                R = det - cfg->k * tr * tr;
            }
            resp[(size_t)y * W + x] = R;
        }
    }

    /* Threshold + strict local-maximum NMS; keep the top-scoring corners when
     * more survive than out_capacity (replace the current weakest). */
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            float R = resp[(size_t)y * W + x];
            if (R <= cfg->threshold)
                continue;
            int is_max = 1;
            for (int dy = -r; dy <= r && is_max; ++dy) {
                int ny = y + dy;
                if (ny < 0 || ny >= H)
                    continue;
                for (int dx = -r; dx <= r; ++dx) {
                    int nx = x + dx;
                    if ((dx == 0 && dy == 0) || nx < 0 || nx >= W)
                        continue;
                    float Rn = resp[(size_t)ny * W + nx];
                    /* strictly greater wins; on ties the earlier (scan-order)
                     * pixel is kept so a plateau yields a single corner. */
                    if (Rn > R || (Rn == R && ((size_t)ny * W + nx) < ((size_t)y * W + x))) {
                        is_max = 0;
                        break;
                    }
                }
            }
            if (!is_max)
                continue;

            CvCorner c = {.x = x, .y = y, .score = R};
            if (*out_count < out_capacity) {
                out[(*out_count)++] = c;
            } else {
                size_t mini = 0;
                for (size_t i = 1; i < out_capacity; ++i)
                    if (out[i].score < out[mini].score)
                        mini = i;
                if (R > out[mini].score)
                    out[mini] = c;
            }
        }
    }

    /* Sort survivors by descending score (selection sort; count is small). */
    for (size_t i = 0; i < *out_count; ++i) {
        size_t best = i;
        for (size_t j = i + 1; j < *out_count; ++j)
            if (out[j].score > out[best].score)
                best = j;
        if (best != i) {
            CvCorner t = out[i];
            out[i] = out[best];
            out[best] = t;
        }
    }

    memory_free(ix);
    memory_free(iy);
    memory_free(resp);
    return EMBEDDIP_OK;
}

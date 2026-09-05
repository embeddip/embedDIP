// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/keypoint.h"

#include "core/memory_manager.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

/* Radius-3 Bresenham circle, 16 points, clockwise from top. */
static const int8_t FAST_DX[16] = {0, 1, 2, 3, 3, 3, 2, 1, 0, -1, -2, -3, -3, -3, -2, -1};
static const int8_t FAST_DY[16] = {-3, -3, -2, -1, 0, 1, 2, 3, 3, 3, 2, 1, 0, -1, -2, -3};
#define FAST_ARC 9
#define FAST_MARGIN 3

static inline uint8_t px(const uint8_t *p, uint32_t stride, int x, int y)
{
    return p[(uint32_t)y * stride + (uint32_t)x];
}

/* FAST corner test at (x,y); returns 1 and sets *score if a corner. */
static int fast_test(const uint8_t *p, uint32_t stride, int x, int y, int t, float *score)
{
    int c = px(p, stride, x, y);
    int hi = c + t, lo = c - t;
    uint8_t ring[16];
    int bright = 0, dark = 0;
    for (int i = 0; i < 16; ++i) {
        ring[i] = px(p, stride, x + FAST_DX[i], y + FAST_DY[i]);
        bright += (ring[i] > hi);
        dark += (ring[i] < lo);
    }
    if (bright < FAST_ARC && dark < FAST_ARC)
        return 0;

    /* Longest contiguous arc (circular) satisfying each predicate. */
    int best_b = 0, best_d = 0, run_b = 0, run_d = 0;
    for (int i = 0; i < 32; ++i) { /* wrap once for circularity */
        uint8_t v = ring[i & 15];
        run_b = (v > hi) ? run_b + 1 : 0;
        run_d = (v < lo) ? run_d + 1 : 0;
        if (run_b > best_b)
            best_b = run_b;
        if (run_d > best_d)
            best_d = run_d;
    }
    if (best_b < FAST_ARC && best_d < FAST_ARC)
        return 0;

    float s = 0.0f;
    for (int i = 0; i < 16; ++i) {
        int d = (int)ring[i] - c;
        s += (d < 0) ? -d : d;
    }
    *score = s;
    return 1;
}

embeddip_status_t cv_fast_detect(const ImageView *src, uint8_t threshold, bool nonmax,
                                 CvKeypoint *out, size_t out_capacity, size_t *out_count)
{
    if (!src || !out || !out_count || !src->pixels)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (src->format != IMAGE_FORMAT_GRAYSCALE && src->format != IMAGE_FORMAT_MASK)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (src->depth != IMAGE_DEPTH_U8)
        return EMBEDDIP_ERROR_INVALID_DEPTH;
    if (out_capacity == 0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    *out_count = 0;
    const int W = (int)src->width, H = (int)src->height;
    if (W < 2 * FAST_MARGIN + 1 || H < 2 * FAST_MARGIN + 1)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    const uint8_t *p = (const uint8_t *)src->pixels;
    const uint32_t stride = src->row_stride_bytes;
    const int t = (int)threshold;

    float *score = NULL;
    if (nonmax) {
        score = (float *)memory_alloc((size_t)W * H * sizeof(float));
        if (!score)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        for (size_t i = 0; i < (size_t)W * H; ++i)
            score[i] = 0.0f;
    }

    /* First pass: score every corner (also fills the score map for NMS). */
    for (int y = FAST_MARGIN; y < H - FAST_MARGIN; ++y) {
        for (int x = FAST_MARGIN; x < W - FAST_MARGIN; ++x) {
            float s;
            if (!fast_test(p, stride, x, y, t, &s))
                continue;
            if (nonmax) {
                score[y * W + x] = s;
                continue; /* emission happens in the NMS pass */
            }
            CvKeypoint k = {.x = x, .y = y, .score = s};
            if (*out_count < out_capacity) {
                out[(*out_count)++] = k;
            } else {
                size_t mini = 0;
                for (size_t i = 1; i < out_capacity; ++i)
                    if (out[i].score < out[mini].score)
                        mini = i;
                if (s > out[mini].score)
                    out[mini] = k;
            }
        }
    }

    if (nonmax) {
        for (int y = FAST_MARGIN; y < H - FAST_MARGIN; ++y) {
            for (int x = FAST_MARGIN; x < W - FAST_MARGIN; ++x) {
                float s = score[y * W + x];
                if (s <= 0.0f)
                    continue;
                int is_max = 1;
                for (int dy = -1; dy <= 1 && is_max; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0)
                            continue;
                        float sn = score[(y + dy) * W + (x + dx)];
                        if (sn > s || (sn == s && ((y + dy) * W + (x + dx)) < (y * W + x))) {
                            is_max = 0;
                            break;
                        }
                    }
                if (!is_max)
                    continue;
                CvKeypoint k = {.x = x, .y = y, .score = s};
                if (*out_count < out_capacity) {
                    out[(*out_count)++] = k;
                } else {
                    size_t mini = 0;
                    for (size_t i = 1; i < out_capacity; ++i)
                        if (out[i].score < out[mini].score)
                            mini = i;
                    if (s > out[mini].score)
                        out[mini] = k;
                }
            }
        }
        memory_free(score);
    }

    /* Sort survivors by descending score. */
    for (size_t i = 0; i < *out_count; ++i) {
        size_t best = i;
        for (size_t j = i + 1; j < *out_count; ++j)
            if (out[j].score > out[best].score)
                best = j;
        if (best != i) {
            CvKeypoint tmp = out[i];
            out[i] = out[best];
            out[best] = tmp;
        }
    }
    return EMBEDDIP_OK;
}

/* -------------------------------------------------------------------------- */
/* BRIEF-256                                                                  */
/* -------------------------------------------------------------------------- */
#define BRIEF_PATCH 15 /* half-width; patch is 31x31 */

/* Fixed sampling pattern: 256 (x1,y1,x2,y2) offsets in [-15,15], generated by a
 * deterministic LCG so descriptors are reproducible across calls without a
 * large hardcoded table. */
static int8_t g_pattern[256][4];
static int g_pattern_ready = 0;

static void brief_init_pattern(void)
{
    uint32_t s = 0x1234567u;
    for (int i = 0; i < 256; ++i)
        for (int j = 0; j < 4; ++j) {
            s = s * 1103515245u + 12345u;
            int v = (int)((s >> 16) % (2 * BRIEF_PATCH + 1)) - BRIEF_PATCH;
            g_pattern[i][j] = (int8_t)v;
        }
    g_pattern_ready = 1;
}

embeddip_status_t cv_keypoint_orient(const ImageView *src, CvKeypoint *kps, size_t count)
{
    if (!src || !kps || !src->pixels)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (src->format != IMAGE_FORMAT_GRAYSCALE && src->format != IMAGE_FORMAT_MASK)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (src->depth != IMAGE_DEPTH_U8)
        return EMBEDDIP_ERROR_INVALID_DEPTH;

    const uint8_t *p = (const uint8_t *)src->pixels;
    const uint32_t stride = src->row_stride_bytes;
    const int W = (int)src->width, H = (int)src->height;
    const int R = BRIEF_PATCH; /* orientation patch radius = descriptor patch */

    for (size_t n = 0; n < count; ++n) {
        int cx = kps[n].x, cy = kps[n].y;
        if (cx < R || cy < R || cx >= W - R || cy >= H - R) {
            kps[n].angle = 0.0f;
            continue;
        }
        long m10 = 0, m01 = 0;
        for (int dy = -R; dy <= R; ++dy)
            for (int dx = -R; dx <= R; ++dx) {
                if (dx * dx + dy * dy > R * R)
                    continue; /* circular patch */
                int v = px(p, stride, cx + dx, cy + dy);
                m10 += (long)dx * v;
                m01 += (long)dy * v;
            }
        kps[n].angle = atan2f((float)m01, (float)m10);
    }
    return EMBEDDIP_OK;
}

embeddip_status_t cv_brief_describe(const ImageView *src, const CvKeypoint *kps, size_t count,
                                    uint8_t *desc, size_t desc_capacity_bytes)
{
    if (!src || !kps || !desc || !src->pixels)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (src->format != IMAGE_FORMAT_GRAYSCALE && src->format != IMAGE_FORMAT_MASK)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (src->depth != IMAGE_DEPTH_U8)
        return EMBEDDIP_ERROR_INVALID_DEPTH;
    if (desc_capacity_bytes < count * CV_BRIEF_BYTES)
        return EMBEDDIP_ERROR_INVALID_SIZE;

    if (!g_pattern_ready)
        brief_init_pattern();

    const uint8_t *p = (const uint8_t *)src->pixels;
    const uint32_t stride = src->row_stride_bytes;
    const int W = (int)src->width, H = (int)src->height;

    for (size_t n = 0; n < count; ++n) {
        uint8_t *d = desc + n * CV_BRIEF_BYTES;
        for (int b = 0; b < CV_BRIEF_BYTES; ++b)
            d[b] = 0;
        int cx = kps[n].x, cy = kps[n].y;
        if (cx < BRIEF_PATCH || cy < BRIEF_PATCH || cx >= W - BRIEF_PATCH ||
            cy >= H - BRIEF_PATCH)
            continue; /* too close to border -> all-zero descriptor */
        /* Steer the sampling pattern by the keypoint orientation (rBRIEF).
         * angle == 0 -> cos=1,sin=0 -> plain BRIEF. Rotated samples can reach
         * ~R*sqrt(2), so clamp to the image bounds. */
        float ca = cosf(kps[n].angle), sa = sinf(kps[n].angle);
        for (int i = 0; i < 256; ++i) {
            const int8_t *q = g_pattern[i];
            int x1 = cx + (int)lroundf(q[0] * ca - q[1] * sa);
            int y1 = cy + (int)lroundf(q[0] * sa + q[1] * ca);
            int x2 = cx + (int)lroundf(q[2] * ca - q[3] * sa);
            int y2 = cy + (int)lroundf(q[2] * sa + q[3] * ca);
            x1 = x1 < 0 ? 0 : (x1 >= W ? W - 1 : x1);
            y1 = y1 < 0 ? 0 : (y1 >= H ? H - 1 : y1);
            x2 = x2 < 0 ? 0 : (x2 >= W ? W - 1 : x2);
            y2 = y2 < 0 ? 0 : (y2 >= H ? H - 1 : y2);
            if (px(p, stride, x1, y1) < px(p, stride, x2, y2))
                d[i >> 3] |= (uint8_t)(1u << (i & 7));
        }
    }
    return EMBEDDIP_OK;
}

/* -------------------------------------------------------------------------- */
/* Scale-invariant ORB (image pyramid)                                        */
/* -------------------------------------------------------------------------- */

/* 2x2-average downsample: dst is (W/2)x(H/2). */
static void downsample2(const uint8_t *src, int W, int H, uint8_t *dst)
{
    int w2 = W / 2, h2 = H / 2;
    for (int y = 0; y < h2; ++y)
        for (int x = 0; x < w2; ++x) {
            const uint8_t *r0 = src + (size_t)(2 * y) * W + 2 * x;
            const uint8_t *r1 = r0 + W;
            dst[y * w2 + x] = (uint8_t)((r0[0] + r0[1] + r1[0] + r1[1] + 2) / 4);
        }
}

/* Insert one keypoint + its descriptor, keeping the top-scoring out_capacity. */
static void orb_insert(CvKeypoint *out, uint8_t *desc, size_t *count, size_t cap,
                       CvKeypoint kp, const uint8_t *kpdesc)
{
    if (*count < cap) {
        out[*count] = kp;
        memcpy(desc + *count * CV_BRIEF_BYTES, kpdesc, CV_BRIEF_BYTES);
        (*count)++;
        return;
    }
    size_t mini = 0;
    for (size_t i = 1; i < cap; ++i)
        if (out[i].score < out[mini].score)
            mini = i;
    if (kp.score > out[mini].score) {
        out[mini] = kp;
        memcpy(desc + mini * CV_BRIEF_BYTES, kpdesc, CV_BRIEF_BYTES);
    }
}

embeddip_status_t cv_orb_detect_and_describe(const ImageView *src, const CvOrbConfig *cfg,
                                             CvKeypoint *out, uint8_t *desc,
                                             size_t out_capacity, size_t *out_count)
{
    if (!src || !cfg || !out || !desc || !out_count || !src->pixels)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (src->format != IMAGE_FORMAT_GRAYSCALE && src->format != IMAGE_FORMAT_MASK)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (src->depth != IMAGE_DEPTH_U8)
        return EMBEDDIP_ERROR_INVALID_DEPTH;
    if (cfg->nlevels < 1 || out_capacity == 0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    *out_count = 0;
    int W = (int)src->width, H = (int)src->height;

    /* Contiguous level-0 copy (collapses any row stride). */
    uint8_t *cur = (uint8_t *)memory_alloc((size_t)W * H);
    CvKeypoint *tmp = (CvKeypoint *)memory_alloc(out_capacity * sizeof(CvKeypoint));
    uint8_t *tmpd = (uint8_t *)memory_alloc(out_capacity * CV_BRIEF_BYTES);
    if (!cur || !tmp || !tmpd) {
        memory_free(cur);
        memory_free(tmp);
        memory_free(tmpd);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    const uint8_t *sp = (const uint8_t *)src->pixels;
    for (int y = 0; y < H; ++y)
        memcpy(cur + (size_t)y * W, sp + (size_t)y * src->row_stride_bytes, (size_t)W);

    int curW = W, curH = H, scale = 1;
    for (int L = 0; L < cfg->nlevels; ++L) {
        if (curW < 2 * FAST_MARGIN + 1 || curH < 2 * FAST_MARGIN + 1)
            break;
        ImageView lv = {.pixels = cur, .width = (uint32_t)curW, .height = (uint32_t)curH,
                        .row_stride_bytes = (uint32_t)curW, .format = IMAGE_FORMAT_GRAYSCALE,
                        .depth = IMAGE_DEPTH_U8, .region = EMBEDDIP_MEMORY_REGION_DEFAULT,
                        .flags = 0u};
        size_t nl = 0;
        cv_fast_detect(&lv, cfg->fast_threshold, true, tmp, out_capacity, &nl);
        cv_keypoint_orient(&lv, tmp, nl);
        cv_brief_describe(&lv, tmp, nl, tmpd, out_capacity * CV_BRIEF_BYTES);
        for (size_t i = 0; i < nl; ++i) {
            CvKeypoint k = tmp[i];
            k.x *= scale; /* map to level-0 coordinates */
            k.y *= scale;
            orb_insert(out, desc, out_count, out_capacity, k, tmpd + i * CV_BRIEF_BYTES);
        }
        if (L + 1 < cfg->nlevels) {
            uint8_t *nxt = (uint8_t *)memory_alloc((size_t)(curW / 2) * (curH / 2));
            if (!nxt)
                break;
            downsample2(cur, curW, curH, nxt);
            memory_free(cur);
            cur = nxt;
            curW /= 2;
            curH /= 2;
            scale *= 2;
        }
    }
    memory_free(cur);
    memory_free(tmp);
    memory_free(tmpd);

    /* Sort by descending score. */
    for (size_t i = 0; i < *out_count; ++i) {
        size_t best = i;
        for (size_t j = i + 1; j < *out_count; ++j)
            if (out[j].score > out[best].score)
                best = j;
        if (best != i) {
            CvKeypoint tk = out[i];
            out[i] = out[best];
            out[best] = tk;
            uint8_t tb[CV_BRIEF_BYTES];
            memcpy(tb, desc + i * CV_BRIEF_BYTES, CV_BRIEF_BYTES);
            memcpy(desc + i * CV_BRIEF_BYTES, desc + best * CV_BRIEF_BYTES, CV_BRIEF_BYTES);
            memcpy(desc + best * CV_BRIEF_BYTES, tb, CV_BRIEF_BYTES);
        }
    }
    return EMBEDDIP_OK;
}

/* -------------------------------------------------------------------------- */
/* Hamming match                                                              */
/* -------------------------------------------------------------------------- */
static uint32_t hamming(const uint8_t *a, const uint8_t *b)
{
    uint32_t d = 0;
    for (int i = 0; i < CV_BRIEF_BYTES; ++i) {
        uint8_t x = a[i] ^ b[i];
#if defined(__GNUC__)
        d += (uint32_t)__builtin_popcount(x);
#else
        while (x) {
            d += x & 1u;
            x >>= 1;
        }
#endif
    }
    return d;
}

embeddip_status_t cv_hamming_match(const uint8_t *query, size_t query_count,
                                   const uint8_t *train, size_t train_count,
                                   uint32_t max_distance, CvMatch *out, size_t out_capacity,
                                   size_t *out_count)
{
    if (!query || !train || !out || !out_count)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (out_capacity == 0)
        return EMBEDDIP_ERROR_INVALID_ARG;
    *out_count = 0;
    if (train_count == 0)
        return EMBEDDIP_OK;

    for (size_t q = 0; q < query_count && *out_count < out_capacity; ++q) {
        uint32_t best = 257, best_i = 0;
        for (size_t tr = 0; tr < train_count; ++tr) {
            uint32_t dist = hamming(query + q * CV_BRIEF_BYTES, train + tr * CV_BRIEF_BYTES);
            if (dist < best) {
                best = dist;
                best_i = (uint32_t)tr;
            }
        }
        if (best <= max_distance) {
            out[*out_count].query_idx = (uint32_t)q;
            out[*out_count].train_idx = best_i;
            out[*out_count].distance = best;
            (*out_count)++;
        }
    }
    return EMBEDDIP_OK;
}

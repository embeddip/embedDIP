// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/detect.h"

#include "core/memory_manager.h"

#include <math.h>
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

/* Round a non-negative float to int. */
static int32_t round_i(float v)
{
    return (int32_t)(v + 0.5f);
}

embeddip_status_t cv_detect_fomo_decode(const float *grid, int grid_w, int grid_h,
                                        int num_classes, int bg_class, float threshold,
                                        int in_width, int in_height, CvNnDetection *out,
                                        size_t out_capacity, size_t *out_count)
{
    if (grid == NULL || out == NULL || out_count == NULL)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (grid_w <= 0 || grid_h <= 0 || in_width <= 0 || in_height <= 0)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (num_classes <= 1 || out_capacity == 0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    *out_count = 0;
    const int G = grid_w * grid_h;

    int *cls = (int *)memory_alloc((size_t)G * sizeof(int));
    float *sc = (float *)memory_alloc((size_t)G * sizeof(float));
    int *stack = (int *)memory_alloc((size_t)G * sizeof(int));
    if (!cls || !sc || !stack) {
        memory_free(cls);
        memory_free(sc);
        memory_free(stack);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    /* Per-cell arg-max; mark inactive cells with class -1. */
    for (int i = 0; i < G; ++i) {
        const float *p = grid + (size_t)i * num_classes;
        int best = 0;
        for (int c = 1; c < num_classes; ++c)
            if (p[c] > p[best])
                best = c;
        if (best == bg_class || p[best] <= threshold) {
            cls[i] = -1;
        } else {
            cls[i] = best;
            sc[i] = p[best];
        }
    }

    const float cw = (float)in_width / (float)grid_w;
    const float ch = (float)in_height / (float)grid_h;

    /* Flood-fill 8-connected same-class active cells into detections. */
    for (int start = 0; start < G; ++start) {
        if (cls[start] < 0)
            continue;
        int group_cls = cls[start];
        int top = 0;
        stack[top++] = start;
        cls[start] = -1 - group_cls - 1; /* mark visited (encode class negatively) */

        int minx = grid_w, miny = grid_h, maxx = -1, maxy = -1;
        float best_score = 0.0f;
        while (top > 0) {
            int idx = stack[--top];
            int gx = idx % grid_w, gy = idx / grid_w;
            if (gx < minx) minx = gx;
            if (gy < miny) miny = gy;
            if (gx > maxx) maxx = gx;
            if (gy > maxy) maxy = gy;
            if (sc[idx] > best_score)
                best_score = sc[idx];
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0)
                        continue;
                    int nx = gx + dx, ny = gy + dy;
                    if (nx < 0 || ny < 0 || nx >= grid_w || ny >= grid_h)
                        continue;
                    int nidx = ny * grid_w + nx;
                    if (cls[nidx] == group_cls) {
                        cls[nidx] = -1 - group_cls - 1;
                        stack[top++] = nidx;
                    }
                }
        }

        CvNnDetection d;
        d.cls = group_cls;
        d.score = best_score;
        d.box.x = round_i((float)minx * cw);
        d.box.y = round_i((float)miny * ch);
        d.box.width = round_i((float)(maxx + 1) * cw) - d.box.x;
        d.box.height = round_i((float)(maxy + 1) * ch) - d.box.y;

        if (*out_count < out_capacity) {
            out[(*out_count)++] = d;
        } else {
            size_t mini = 0;
            for (size_t i = 1; i < out_capacity; ++i)
                if (out[i].score < out[mini].score)
                    mini = i;
            if (d.score > out[mini].score)
                out[mini] = d;
        }
    }

    /* Sort detections by descending score. */
    for (size_t i = 0; i < *out_count; ++i) {
        size_t best = i;
        for (size_t j = i + 1; j < *out_count; ++j)
            if (out[j].score > out[best].score)
                best = j;
        if (best != i) {
            CvNnDetection t = out[i];
            out[i] = out[best];
            out[best] = t;
        }
    }

    memory_free(cls);
    memory_free(sc);
    memory_free(stack);
    return EMBEDDIP_OK;
}

/* -------------------------------------------------------------------------- */
/* NN detection-head decoders (YOLO, SSD)                                     */
/* -------------------------------------------------------------------------- */

static float nn_sigmoid(float x) { return 1.0f / (1.0f + expf(-x)); }

/* Insert keeping the top-scoring out_capacity detections. */
static void nn_insert(CvNnDetection *out, size_t *cnt, size_t cap, CvNnDetection d)
{
    if (*cnt < cap) {
        out[(*cnt)++] = d;
        return;
    }
    size_t mini = 0;
    for (size_t i = 1; i < cap; ++i)
        if (out[i].score < out[mini].score)
            mini = i;
    if (d.score > out[mini].score)
        out[mini] = d;
}

/* Greedy class-aware NMS in place: sort by descending score, keep a box unless
 * it overlaps an already-kept box of the SAME class by more than iou. */
static void nn_nms(CvNnDetection *d, size_t *n, float iou)
{
    for (size_t i = 0; i + 1 < *n; ++i) {
        size_t best = i;
        for (size_t j = i + 1; j < *n; ++j)
            if (d[j].score > d[best].score)
                best = j;
        if (best != i) {
            CvNnDetection t = d[i];
            d[i] = d[best];
            d[best] = t;
        }
    }
    size_t kept = 0;
    for (size_t i = 0; i < *n; ++i) {
        int suppressed = 0;
        for (size_t s = 0; s < kept; ++s)
            if (d[s].cls == d[i].cls && detect_iou(&d[i].box, &d[s].box) > iou) {
                suppressed = 1;
                break;
            }
        if (!suppressed)
            d[kept++] = d[i];
    }
    *n = kept;
}

embeddip_status_t cv_detect_yolo_decode(const float *pred, const CvYoloConfig *cfg,
                                        CvNnDetection *out, size_t out_capacity,
                                        size_t *out_count)
{
    if (pred == NULL || cfg == NULL || cfg->anchors == NULL || out == NULL || out_count == NULL)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (cfg->grid_w <= 0 || cfg->grid_h <= 0 || cfg->in_width <= 0 || cfg->in_height <= 0)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (cfg->num_anchors <= 0 || cfg->num_classes <= 0 || out_capacity == 0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    *out_count = 0;
    const int gw = cfg->grid_w, gh = cfg->grid_h, na = cfg->num_anchors, nc = cfg->num_classes;
    const int stride = 5 + nc;

    for (int r = 0; r < gh; ++r)
        for (int c = 0; c < gw; ++c)
            for (int a = 0; a < na; ++a) {
                const float *p = pred + ((size_t)(r * gw + c) * na + a) * stride;
                float obj = nn_sigmoid(p[4]);
                int best = 0;
                for (int k = 1; k < nc; ++k)
                    if (p[5 + k] > p[5 + best])
                        best = k;
                float conf = obj * nn_sigmoid(p[5 + best]);
                if (conf <= cfg->conf_threshold)
                    continue;
                float bx = ((float)c + nn_sigmoid(p[0])) / gw * cfg->in_width;
                float by = ((float)r + nn_sigmoid(p[1])) / gh * cfg->in_height;
                float bw = cfg->anchors[2 * a] * expf(p[2]) / gw * cfg->in_width;
                float bh = cfg->anchors[2 * a + 1] * expf(p[3]) / gh * cfg->in_height;
                CvNnDetection d = {.cls = best, .score = conf};
                d.box.x = round_i(bx - bw / 2.0f);
                d.box.y = round_i(by - bh / 2.0f);
                d.box.width = round_i(bw);
                d.box.height = round_i(bh);
                nn_insert(out, out_count, out_capacity, d);
            }
    nn_nms(out, out_count, cfg->iou_threshold);
    return EMBEDDIP_OK;
}

embeddip_status_t cv_detect_ssd_decode(const float *loc, const float *conf,
                                       const CvSsdConfig *cfg, CvNnDetection *out,
                                       size_t out_capacity, size_t *out_count)
{
    if (loc == NULL || conf == NULL || cfg == NULL || cfg->priors == NULL || out == NULL ||
        out_count == NULL)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (cfg->in_width <= 0 || cfg->in_height <= 0 || cfg->num_priors <= 0)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (cfg->num_classes <= 1 || out_capacity == 0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    *out_count = 0;
    const int nc = cfg->num_classes;

    for (int i = 0; i < cfg->num_priors; ++i) {
        const float *cf = conf + (size_t)i * nc;
        int best = 1; /* skip background at index 0 */
        for (int k = 2; k < nc; ++k)
            if (cf[k] > cf[best])
                best = k;
        float score = cf[best];
        if (score <= cfg->conf_threshold)
            continue;
        const float *pr = cfg->priors + (size_t)i * 4;
        const float *lc = loc + (size_t)i * 4;
        float cx = pr[0] + lc[0] * cfg->var_xy * pr[2];
        float cy = pr[1] + lc[1] * cfg->var_xy * pr[3];
        float w = pr[2] * expf(lc[2] * cfg->var_wh);
        float h = pr[3] * expf(lc[3] * cfg->var_wh);
        CvNnDetection d = {.cls = best, .score = score};
        d.box.x = round_i((cx - w / 2.0f) * cfg->in_width);
        d.box.y = round_i((cy - h / 2.0f) * cfg->in_height);
        d.box.width = round_i(w * cfg->in_width);
        d.box.height = round_i(h * cfg->in_height);
        nn_insert(out, out_count, out_capacity, d);
    }
    nn_nms(out, out_count, cfg->iou_threshold);
    return EMBEDDIP_OK;
}

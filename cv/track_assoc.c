// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/track_assoc.h"

#include <string.h>

float cv_track_assoc_iou(Rectangle a, Rectangle b)
{
    int32_t ix0 = a.x > b.x ? a.x : b.x;
    int32_t iy0 = a.y > b.y ? a.y : b.y;
    int32_t ix1 = (a.x + a.width) < (b.x + b.width) ? (a.x + a.width) : (b.x + b.width);
    int32_t iy1 = (a.y + a.height) < (b.y + b.height) ? (a.y + a.height) : (b.y + b.height);

    int32_t iw = ix1 - ix0;
    int32_t ih = iy1 - iy0;
    if (iw <= 0 || ih <= 0) {
        return 0.0f;
    }

    float inter = (float)iw * (float)ih;
    float area_a = (float)a.width * (float)a.height;
    float area_b = (float)b.width * (float)b.height;
    float uni = area_a + area_b - inter;
    if (uni <= 0.0f) {
        return 0.0f;
    }

    return inter / uni;
}

embeddip_status_t cv_track_assoc_init(CvTrackAssoc *t, float iou_threshold, uint16_t max_misses)
{
    if (t == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    memset(t, 0, sizeof(*t));
    t->next_id = 0;
    t->iou_threshold = iou_threshold;
    t->max_misses = max_misses;

    return EMBEDDIP_OK;
}

embeddip_status_t cv_track_assoc_step(CvTrackAssoc *t, const CvDetection *dets, size_t det_count)
{
    if (t == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (det_count > 0 && dets == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    /* 1. Predict all active tracks. */
    for (size_t i = 0; i < CV_TRACK_ASSOC_MAX; i++) {
        CvTrack *trk = &t->tracks[i];
        if (!trk->active) {
            continue;
        }
        embeddip_status_t st = cv_kalman_predict(&trk->kf, &trk->box);
        if (embeddip_failed(st)) {
            return st;
        }
    }

    /* 2. Greedy IoU matching: repeatedly pick the highest-IoU unused pair. */
    bool track_matched[CV_TRACK_ASSOC_MAX] = { false };
    /* ponytail: fixed local buffer sized to CV_TRACK_ASSOC_MAX detections per frame;
     * detections beyond that limit are ignored this frame (neither matched nor
     * spawned) since the spawn loop below is bounded by the same match_det_limit.
     * Bump if a use case needs more per-frame dets. */
    bool det_matched[CV_TRACK_ASSOC_MAX] = { false };
    size_t match_det_limit = det_count < CV_TRACK_ASSOC_MAX ? det_count : CV_TRACK_ASSOC_MAX;

    for (;;) {
        float best_iou = 0.0f;
        size_t best_trk = CV_TRACK_ASSOC_MAX;
        size_t best_det = CV_TRACK_ASSOC_MAX;

        for (size_t i = 0; i < CV_TRACK_ASSOC_MAX; i++) {
            if (!t->tracks[i].active || track_matched[i]) {
                continue;
            }
            for (size_t j = 0; j < match_det_limit; j++) {
                if (det_matched[j]) {
                    continue;
                }
                float iou = cv_track_assoc_iou(t->tracks[i].box, dets[j].box);
                if (iou >= t->iou_threshold && iou > best_iou) {
                    best_iou = iou;
                    best_trk = i;
                    best_det = j;
                }
            }
        }

        if (best_trk == CV_TRACK_ASSOC_MAX) {
            break; /* no more eligible pairs */
        }

        track_matched[best_trk] = true;
        det_matched[best_det] = true;

        CvTrack *trk = &t->tracks[best_trk];
        embeddip_status_t st = cv_kalman_update(&trk->kf, dets[best_det].box);
        if (embeddip_failed(st)) {
            return st;
        }
        trk->box = dets[best_det].box;
        trk->misses = 0;
        trk->age++;
    }

    /* 3. Unmatched detections: spawn new tracks into free slots. */
    for (size_t j = 0; j < match_det_limit; j++) {
        if (det_matched[j]) {
            continue;
        }
        for (size_t i = 0; i < CV_TRACK_ASSOC_MAX; i++) {
            CvTrack *trk = &t->tracks[i];
            if (trk->active) {
                continue;
            }
            embeddip_status_t st = cv_kalman_init(&trk->kf, dets[j].box);
            if (embeddip_failed(st)) {
                break;
            }
            trk->box = dets[j].box;
            trk->id = t->next_id++;
            trk->active = true;
            trk->age = 0;
            trk->misses = 0;
            break;
        }
        /* No free slot: detection is silently dropped. */
    }

    /* 4. Unmatched tracks: age misses, drop if past the threshold. */
    for (size_t i = 0; i < CV_TRACK_ASSOC_MAX; i++) {
        CvTrack *trk = &t->tracks[i];
        if (!trk->active || track_matched[i]) {
            continue;
        }
        trk->misses++;
        if (trk->misses > t->max_misses) {
            trk->active = false;
        }
    }

    return EMBEDDIP_OK;
}

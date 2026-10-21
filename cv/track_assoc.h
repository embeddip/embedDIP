// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_TRACK_ASSOC_H
#define EMBEDDIP_CV_TRACK_ASSOC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"
#include "cv/detect.h"
#include "cv/tracker_kalman.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum number of simultaneously tracked objects. */
#define CV_TRACK_ASSOC_MAX 16u

/**
 * @brief One SORT-style track: a Kalman filter plus bookkeeping.
 */
typedef struct {
    CvKalmanState kf; /**< Backing Kalman filter for this track. */
    Rectangle box;    /**< Last estimate (predicted or corrected). */
    int32_t id;       /**< Stable track id, >= 0. */
    uint16_t age;     /**< Frames since spawn. */
    uint16_t misses;  /**< Consecutive frames without a match. */
    bool active;      /**< Whether this slot holds a live track. */
} CvTrack;

/**
 * @brief Fixed-capacity set of tracks with greedy IoU association state.
 */
typedef struct {
    CvTrack tracks[CV_TRACK_ASSOC_MAX]; /**< Track slots (active/inactive). */
    int32_t next_id;              /**< Next id to assign to a spawned track. */
    float iou_threshold;          /**< Match if IoU >= this (e.g. 0.3). */
    uint16_t max_misses;          /**< Drop track after this many misses (e.g. 5). */
} CvTrackAssoc;

/**
 * @brief Initialize a tracker with empty (inactive) track slots.
 *
 * @param[out] t Tracker to initialize.
 * @param[in] iou_threshold Minimum IoU to consider a track/detection matched.
 * @param[in] max_misses Consecutive missed frames after which a track is dropped.
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if t is NULL.
 */
embeddip_status_t cv_track_assoc_init(CvTrackAssoc *t, float iou_threshold, uint16_t max_misses);

/**
 * @brief Advance the tracker by one frame.
 *
 * Predicts all active tracks, greedily matches them to detections by IoU,
 * corrects matched tracks, spawns new tracks for unmatched detections, and
 * ages/drops tracks that went unmatched.
 *
 * @param[in,out] t Tracker to advance.
 * @param[in] dets Detections for this frame (may be NULL if det_count is 0).
 * @param[in] det_count Number of detections in @p dets.
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if t is NULL, or
 *         an error code propagated from the underlying Kalman filter calls.
 */
embeddip_status_t cv_track_assoc_step(CvTrackAssoc *t, const CvDetection *dets, size_t det_count);

/**
 * @brief Intersection-over-union of two rectangles.
 *
 * @param a First rectangle.
 * @param b Second rectangle.
 * @return IoU in [0, 1]; 0 if the rectangles do not overlap or the union
 *         area is non-positive. Exposed for testing.
 */
float cv_track_assoc_iou(Rectangle a, Rectangle b);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_TRACK_ASSOC_H */

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_HAAR_H
#define EMBEDDIP_CV_HAAR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"
#include "cv/integral.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum rectangles per Haar feature. */
#define CV_HAAR_MAX_RECTS 3u

/**
 * @brief One weighted rectangle of a Haar feature, in window-relative pixels.
 */
typedef struct {
    int16_t x;         /**< Left offset from the window origin. */
    int16_t y;         /**< Top offset from the window origin. */
    uint16_t width;    /**< Rectangle width in pixels. */
    uint16_t height;   /**< Rectangle height in pixels. */
    int16_t weight_q8; /**< Rectangle weight in Q8 fixed point. */
} CvHaarRect;

/**
 * @brief Single-feature weak classifier with a two-way decision stump.
 */
typedef struct {
    uint8_t rectangle_count;               /**< Rectangles in use (1..3). */
    CvHaarRect rectangles[CV_HAAR_MAX_RECTS]; /**< Feature rectangles. */
    int32_t threshold_q8;                  /**< Decision threshold in Q8. */
    int32_t left_value;                    /**< Value when response < threshold. */
    int32_t right_value;                   /**< Value when response >= threshold. */
} CvHaarWeakClassifier;

/**
 * @brief Stage grouping weak classifiers behind a summed threshold.
 */
typedef struct {
    const CvHaarWeakClassifier *weak; /**< Non-owning weak classifier array. */
    size_t weak_count;                /**< Number of weak classifiers. */
    int32_t threshold;                /**< Minimum weak-value sum to pass. */
} CvHaarStage;

/**
 * @brief Cascade of stages evaluated in order.
 */
typedef struct {
    const CvHaarStage *stages; /**< Non-owning stage array. */
    size_t stage_count;        /**< Number of stages. */
    Rectangle window;          /**< Detection window size, for callers. */
} CvHaarCascade;

/**
 * @brief Evaluate a Haar feature response at a window origin.
 *
 * @param[in] table Integral image to sample.
 * @param[in] origin_x Window origin x in the integral image.
 * @param[in] origin_y Window origin y in the integral image.
 * @param[in] rectangles Feature rectangles, window-relative.
 * @param[in] rectangle_count Rectangles in use (1..3).
 * @param[out] out_response_q8 Weighted response in Q8 fixed point.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_haar_feature_response(const CvIntegralU32 *table, int32_t origin_x,
                                           int32_t origin_y,
                                           const CvHaarRect *rectangles,
                                           uint8_t rectangle_count,
                                           int32_t *out_response_q8);

/**
 * @brief Evaluate a full cascade at a window origin.
 *
 * @param[in] table Integral image to sample.
 * @param[in] origin_x Window origin x in the integral image.
 * @param[in] origin_y Window origin y in the integral image.
 * @param[in] cascade Cascade model, read-only.
 * @param[out] out_detected true when every stage passes.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_haar_cascade_eval(const CvIntegralU32 *table, int32_t origin_x,
                                       int32_t origin_y, const CvHaarCascade *cascade,
                                       bool *out_detected);

/**
 * @brief Evaluate a cascade and also report a confidence score.
 *
 * The score is the sum of stage margins (stage sum minus stage threshold) over
 * the stages evaluated before a failure, or over all stages on a pass. Higher
 * scores indicate stronger detections; use it to rank overlapping windows.
 *
 * @param[in] table Integral image to sample.
 * @param[in] origin_x Window origin x in the integral image.
 * @param[in] origin_y Window origin y in the integral image.
 * @param[in] cascade Cascade model, read-only.
 * @param[out] out_detected true when every stage passes (may be NULL).
 * @param[out] out_score Accumulated stage margin (may be NULL).
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_haar_cascade_score(const CvIntegralU32 *table, int32_t origin_x,
                                        int32_t origin_y, const CvHaarCascade *cascade,
                                        bool *out_detected, int32_t *out_score);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_HAAR_H */

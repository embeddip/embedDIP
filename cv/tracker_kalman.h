// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_TRACKER_KALMAN_H
#define EMBEDDIP_CV_TRACKER_KALMAN_H

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Constant-acceleration Kalman filter state for 2D bounding-box tracking.
 *
 * State vector is [x, y, vx, vy, ax, ay]; box width/height pass through
 * unfiltered (only the box center is estimated).
 */
typedef struct {
    float state[6];      /**< [x, y, vx, vy, ax, ay] */
    float covariance[36]; /**< 6x6 error covariance, row-major */
    int32_t box_width;
    int32_t box_height;
    bool initialized;
} CvKalmanState;

/**
 * @brief Initialize the filter with an initial bounding box (zero velocity/accel).
 *
 * @param[out] state Filter state to initialize.
 * @param[in] initial_box Initial bounding box; center seeds position state.
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state is NULL,
 *         EMBEDDIP_ERROR_INVALID_SIZE if initial_box has non-positive width/height.
 */
embeddip_status_t cv_kalman_init(CvKalmanState *state, Rectangle initial_box);

/**
 * @brief Predict the next box position using the constant-acceleration model.
 *
 * Does not consume a measurement; call cv_kalman_update separately when a
 * measurement is available. Advances internal state by one step.
 *
 * @param[in,out] state Filter state (advanced in place).
 * @param[out] out_box Predicted bounding box (width/height unchanged from init).
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state or out_box
 *         is NULL, EMBEDDIP_ERROR_NOT_INITIALIZED if cv_kalman_init was not
 *         called first.
 */
embeddip_status_t cv_kalman_predict(CvKalmanState *state, Rectangle *out_box);

/**
 * @brief Correct the filter state with a new measurement (e.g. a detector box).
 *
 * @param[in,out] state Filter state (corrected in place).
 * @param[in] measured_box Observed bounding box this frame.
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state is NULL,
 *         EMBEDDIP_ERROR_NOT_INITIALIZED if cv_kalman_init was not called first.
 */
embeddip_status_t cv_kalman_update(CvKalmanState *state, Rectangle measured_box);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_TRACKER_KALMAN_H */

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/tracker_kalman.h"

#include <stdbool.h>
#include <string.h>

/* Constant-acceleration state transition: x' = x + vx + 0.5*ax, etc. */
static const float kF[36] = {
    1.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

/* Process noise, diagonal only (small constant, matches original HexAccel_noise_mag). */
#define KALMAN_PROCESS_NOISE 0.01f
/* Measurement noise, diagonal only (tuned down from original tkn_x/tkn_y=1.0f to meet tracking-accuracy tolerance). */
#define KALMAN_MEASUREMENT_NOISE 0.1f

embeddip_status_t cv_kalman_init(CvKalmanState *state, Rectangle initial_box)
{
    if (state == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (initial_box.width <= 0 || initial_box.height <= 0) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    memset(state->state, 0, sizeof(state->state));
    state->state[0] = (float)initial_box.x + (float)initial_box.width / 2.0f;
    state->state[1] = (float)initial_box.y + (float)initial_box.height / 2.0f;

    memset(state->covariance, 0, sizeof(state->covariance));
    for (int i = 0; i < 6; ++i) {
        state->covariance[i * 6 + i] = KALMAN_PROCESS_NOISE;
    }

    state->box_width = initial_box.width;
    state->box_height = initial_box.height;
    state->initialized = true;
    return EMBEDDIP_OK;
}

/* new_state = F * state (6x6 * 6x1), hand-unrolled since dim is fixed. */
static void kalman_apply_transition(const float *s, float *out)
{
    for (int r = 0; r < 6; ++r) {
        float sum = 0.0f;
        for (int c = 0; c < 6; ++c) {
            sum += kF[r * 6 + c] * s[c];
        }
        out[r] = sum;
    }
}

embeddip_status_t cv_kalman_predict(CvKalmanState *state, Rectangle *out_box)
{
    if (state == NULL || out_box == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!state->initialized) {
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }

    float predicted[6];
    kalman_apply_transition(state->state, predicted);
    memcpy(state->state, predicted, sizeof(predicted));

    /* Process noise added to position/velocity/accel covariance diagonal. */
    for (int i = 0; i < 6; ++i) {
        state->covariance[i * 6 + i] += KALMAN_PROCESS_NOISE;
    }

    out_box->x = (int32_t)(state->state[0] - (float)state->box_width / 2.0f);
    out_box->y = (int32_t)(state->state[1] - (float)state->box_height / 2.0f);
    out_box->width = state->box_width;
    out_box->height = state->box_height;
    return EMBEDDIP_OK;
}

embeddip_status_t cv_kalman_update(CvKalmanState *state, Rectangle measured_box)
{
    if (state == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!state->initialized) {
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }

    float measured_x = (float)measured_box.x + (float)measured_box.width / 2.0f;
    float measured_y = (float)measured_box.y + (float)measured_box.height / 2.0f;

    /* Scalar Kalman gain per axis (position only): K = P / (P + R). */
    float px = state->covariance[0];
    float py = state->covariance[1 * 6 + 1];
    float kx = px / (px + KALMAN_MEASUREMENT_NOISE);
    float ky = py / (py + KALMAN_MEASUREMENT_NOISE);

    state->state[0] += kx * (measured_x - state->state[0]);
    state->state[1] += ky * (measured_y - state->state[1]);

    state->covariance[0] *= (1.0f - kx);
    state->covariance[1 * 6 + 1] *= (1.0f - ky);

    if (measured_box.width > 0 && measured_box.height > 0) {
        state->box_width = measured_box.width;
        state->box_height = measured_box.height;
    }
    return EMBEDDIP_OK;
}

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_TRACKER_MEANSHIFT_H
#define EMBEDDIP_CV_TRACKER_MEANSHIFT_H

#include <stdbool.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"
#include "cv/track_hist.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mean-shift tracker state: target histogram + current box.
 */
typedef struct {
    float template_hist[CV_HIST_MAX_BINS]; /**< Normalized target histogram. */
    uint32_t hist_nbins;                   /**< Bins in use (32 gray / 96 RGB565). */
    int32_t box_width;                     /**< Tracked box width in pixels. */
    int32_t box_height;                    /**< Tracked box height in pixels. */
    int32_t center_x;                      /**< Current box center X. */
    int32_t center_y;                      /**< Current box center Y. */
    uint16_t max_iters;                    /**< Mode-seek iteration cap (default 5). */
    bool initialized;                      /**< True once cv_meanshift_init succeeded. */
} CvMeanShiftState;

/**
 * @brief Initialize a mean-shift tracker from a target ROI in @p frame.
 *
 * Builds the target's normalized histogram (via cv_hist_build) and stores
 * the initial box center/size. @p max_iters defaults to 5.
 *
 * @param[out] state Tracker state to initialize.
 * @param[in] frame Source frame (GRAYSCALE/MASK U8, or RGB565 U16).
 * @param[in] roi Initial target box; must have positive width/height.
 * @return EMBEDDIP_OK on success; EMBEDDIP_ERROR_NULL_PTR if state/frame is
 *         NULL; EMBEDDIP_ERROR_INVALID_FORMAT if the frame format/depth is
 *         unsupported; EMBEDDIP_ERROR_INVALID_SIZE if roi is empty/out of
 *         bounds.
 */
embeddip_status_t cv_meanshift_init(CvMeanShiftState *state, const ImageView *frame, Rectangle roi);

/**
 * @brief Advance the tracker one frame via classic mean-shift mode-seeking.
 *
 * Iterates up to state->max_iters times: backprojects the target histogram
 * over a search box centered on the current position (padded to twice the
 * box size, clamped to the frame, so a shift up to one box dimension can be
 * recovered), weights each pixel by sqrtf(template_hist[bin]), and moves the
 * center to the weighted centroid. Stops early once the shift is < 1 px.
 * The box is re-clamped so it stays inside the frame after each move.
 *
 * @param[in,out] state Tracker state (center is updated in place).
 * @param[in] frame Current frame; same format as used in cv_meanshift_init.
 * @param[out] out_box Converged box (center ± size/2).
 * @return EMBEDDIP_OK on success; EMBEDDIP_ERROR_NULL_PTR if state/frame/
 *         out_box is NULL; EMBEDDIP_ERROR_NOT_INITIALIZED if state was not
 *         initialized.
 */
embeddip_status_t cv_meanshift_update(CvMeanShiftState *state, const ImageView *frame,
                                       Rectangle *out_box);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_TRACKER_MEANSHIFT_H */

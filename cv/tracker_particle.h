// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_TRACKER_PARTICLE_H
#define EMBEDDIP_CV_TRACKER_PARTICLE_H

#include <stdbool.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"
#include "cv/track_hist.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Max particle_count supported by the histogram-likelihood path
 * (cv_particle_init_hist / cv_particle_update's SIR resample). Weight and
 * resample scratch for that path live on the stack (no heap use), so
 * particle_count is bounded to keep stack usage bounded too. Not enforced
 * on the legacy gradient-weighting path.
 */
#define CV_PARTICLE_MAX_COUNT 256u

/**
 * @brief Particle-filter tracker state.
 *
 * Particles are stored as caller-owned (x, y) float pairs in
 * @c particle_buffer (capacity particle_count * 2 floats), so the module
 * never allocates memory.
 */
typedef struct {
    float *particle_buffer;  /**< Caller-owned, particle_count * 2 floats: [x0,y0,x1,y1,...] */
    uint16_t particle_count;
    int32_t box_width;
    int32_t box_height;
    uint32_t rng_state; /**< xorshift32 state, seeded at init */
    bool initialized;
    float template_hist[CV_HIST_MAX_BINS]; /**< Target histogram, filled by cv_particle_init_hist */
    uint32_t hist_nbins; /**< 0 = legacy gradient weighting; >0 = histogram-Bhattacharyya weighting */
} CvParticleState;

/**
 * @brief Seed the particle filter around an initial bounding box.
 *
 * @param[out] state Filter state to initialize.
 * @param[in] particle_count Number of particles (> 0); particle_buffer must
 *            have capacity particle_count * 2 floats.
 * @param[in] particle_buffer Caller-owned scratch buffer for particle positions.
 * @param[in] roi Initial bounding box; particles are seeded around its center.
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state or
 *         particle_buffer is NULL, EMBEDDIP_ERROR_INVALID_ARG if
 *         particle_count is 0, EMBEDDIP_ERROR_INVALID_SIZE if roi has
 *         non-positive width/height.
 */
embeddip_status_t cv_particle_init(CvParticleState *state, uint16_t particle_count,
                                   float *particle_buffer, Rectangle roi);

/**
 * @brief Seed the particle filter (as cv_particle_init) and capture a target
 *        histogram from the init frame, switching cv_particle_update to the
 *        histogram-Bhattacharyya likelihood (ref [1] SIR-PF) instead of the
 *        legacy gradient weighting.
 *
 * @param[out] state Filter state to initialize.
 * @param[in] particle_count Number of particles (> 0 and <=
 *            CV_PARTICLE_MAX_COUNT); particle_buffer must have capacity
 *            particle_count * 2 floats.
 * @param[in] particle_buffer Caller-owned scratch buffer for particle positions.
 * @param[in] frame Grayscale/mask (U8) or RGB565 (U16) image view to sample
 *            the target histogram from.
 * @param[in] roi Initial bounding box; also the histogram sampling region.
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state,
 *         particle_buffer, or frame is NULL, EMBEDDIP_ERROR_INVALID_ARG if
 *         particle_count is 0 or exceeds CV_PARTICLE_MAX_COUNT,
 *         EMBEDDIP_ERROR_INVALID_SIZE if roi has non-positive width/height,
 *         EMBEDDIP_ERROR_INVALID_FORMAT if frame is not grayscale/mask/RGB565.
 */
embeddip_status_t cv_particle_init_hist(CvParticleState *state, uint16_t particle_count,
                                        float *particle_buffer, const ImageView *frame,
                                        Rectangle roi);

/**
 * @brief Diffuse particles, weight them, and return the weighted-centroid
 *        bounding box.
 *
 * Weighting depends on how @p state was initialized: cv_particle_init uses
 * local gradient strength (legacy path); cv_particle_init_hist uses
 * histogram-Bhattacharyya similarity against the captured target histogram,
 * followed by SIR resampling of @c particle_buffer.
 *
 * @param[in,out] state Filter state (particles updated in place).
 * @param[in] frame Image view to search; grayscale/mask for the legacy path,
 *            grayscale/mask/RGB565 for the histogram path (matching the
 *            format cv_particle_init_hist was called with).
 * @param[out] out_box Weighted-centroid bounding box (same size as init roi).
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state, frame, or
 *         out_box is NULL, EMBEDDIP_ERROR_NOT_INITIALIZED if cv_particle_init
 *         /cv_particle_init_hist was not called first, EMBEDDIP_ERROR_INVALID_FORMAT
 *         if frame's format doesn't match the active path.
 */
embeddip_status_t cv_particle_update(CvParticleState *state, const ImageView *frame,
                                     Rectangle *out_box);

/**
 * @brief Release the particle filter's association with its buffer.
 *
 * The particle buffer is caller-owned (no heap allocation happens inside
 * this module), so this only clears @p state so it can no longer be used
 * without re-initializing; it does not free @c particle_buffer itself.
 *
 * @param[in,out] state Filter state to release; NULL is a no-op.
 */
void cv_particle_free(CvParticleState *state);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_TRACKER_PARTICLE_H */

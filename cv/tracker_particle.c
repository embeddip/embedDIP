// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/tracker_particle.h"

#include "core/memory_manager.h"
#include "imgproc/filter.h"

#include <stdint.h>
#include <string.h>

#include "cv/image_gray.h"

/* xorshift32: fast, deterministic, no external RNG dependency. */
static uint32_t xorshift32(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

/* Uniform float in [0, 1]. */
static float xorshift_unit(uint32_t *state)
{
    uint32_t r = xorshift32(state);
    return (float)r / (float)UINT32_MAX;
}

/* Uniform float in [-range, range]. */
static float xorshift_range(uint32_t *state, float range)
{
    return (xorshift_unit(state) * 2.0f - 1.0f) * range;
}

embeddip_status_t cv_particle_init(CvParticleState *state,
                                   uint16_t particle_count,
                                   float *particle_buffer,
                                   Rectangle roi)
{
    if (state == NULL || particle_buffer == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (particle_count == 0u) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (roi.width <= 0 || roi.height <= 0) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    state->particle_buffer = particle_buffer;
    state->particle_count = particle_count;
    state->box_width = roi.width;
    state->box_height = roi.height;
    state->rng_state = 0x9E3779B9u; /* fixed seed: deterministic tracking, no HW RNG dependency */
    state->hist_nbins = 0u; /* legacy gradient weighting unless cv_particle_init_hist overrides */

    float center_x = (float)roi.x + (float)roi.width / 2.0f;
    float center_y = (float)roi.y + (float)roi.height / 2.0f;
    for (uint16_t i = 0u; i < particle_count; ++i) {
        particle_buffer[i * 2u] = center_x + xorshift_range(&state->rng_state, 5.0f);
        particle_buffer[i * 2u + 1u] = center_y + xorshift_range(&state->rng_state, 5.0f);
    }

    state->initialized = true;
    return EMBEDDIP_OK;
}

embeddip_status_t cv_particle_init_hist(CvParticleState *state,
                                        uint16_t particle_count,
                                        float *particle_buffer,
                                        const ImageView *frame,
                                        Rectangle roi)
{
    if (frame == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (particle_count > CV_PARTICLE_MAX_COUNT) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    embeddip_status_t status = cv_particle_init(state, particle_count, particle_buffer, roi);
    if (status != EMBEDDIP_OK) {
        return status;
    }

    status = cv_hist_build(frame, roi, state->template_hist, &state->hist_nbins);
    if (status != EMBEDDIP_OK) {
        state->initialized = false; /* leave the caller with a clean, not half-set-up, state */
        state->hist_nbins = 0u;
        return status;
    }

    return EMBEDDIP_OK;
}

/* Build a non-owning Image view over an ImageView's buffer, matching fields
 * one-to-one (both are plain structs; no ownership transfer). */
static void particle_image_from_view(const ImageView *view, Image *out)
{
    out->width = view->width;
    out->height = view->height;
    out->pixels = view->pixels;
    out->chals = NULL;
    out->size = view->width * view->height;
    out->format = view->format;
    out->depth = view->depth;
    out->log = IMAGE_DATA_PIXELS;
    out->is_chals = false;
}

/* Null-safe teardown for a filter-populated Image's channel storage:
 * gaussianGradients/gradientMagnitude can leave chals allocated (struct
 * only, or struct + ch[0]) even on OOM failure paths, so every field must
 * be checked before freeing. */
static void particle_free_image_chals(Image *img)
{
    if (img->chals == NULL) {
        return;
    }
    if (img->chals->ch[0] != NULL) {
        memory_free(img->chals->ch[0]);
    }
    memory_free(img->chals);
    img->chals = NULL;
}

/* Histogram-likelihood update (SIR-PF, ref [1]): weight each particle by
 * Bhattacharyya similarity between its candidate box's histogram and the
 * template histogram, take the weighted centroid, then SIR-resample
 * particle_buffer. Ported from ../object-trackers/F429_Tracker/Core/Src/
 * Bhattacharya.c's cumulative-weight + uniform-draw resample loop, using
 * the module's xorshift32 RNG instead of HAL RNG.
 *
 * No heap: the candidate histogram and the weight/resample scratch below
 * are stack arrays bounded by CV_PARTICLE_MAX_COUNT (validated at
 * cv_particle_init_hist), independent of the caller's particle_buffer. */
static embeddip_status_t
cv_particle_update_hist(CvParticleState *state, const ImageView *frame, Rectangle *out_box)
{
    uint16_t count = state->particle_count;
    float weights[CV_PARTICLE_MAX_COUNT];
    float sum_w = 0.0f;

    for (uint16_t i = 0u; i < count; ++i) {
        float px = state->particle_buffer[i * 2u];
        float py = state->particle_buffer[i * 2u + 1u];

        Rectangle cand_roi = cv_clamp_centered_box((int32_t)px,
                                                   (int32_t)py,
                                                   state->box_width,
                                                   state->box_height,
                                                   frame->width,
                                                   frame->height);

        float cand_hist[CV_HIST_MAX_BINS];
        uint32_t cand_nbins = 0u;
        float weight = 0.0f;
        if (cv_hist_build(frame, cand_roi, cand_hist, &cand_nbins) == EMBEDDIP_OK) {
            weight =
                cv_hist_bhattacharyya(cand_hist, state->template_hist, state->hist_nbins) + 1e-6f;
        }
        weights[i] = weight;
        sum_w += weight;
    }

    if (sum_w <= 0.0f) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    float centroid_x = 0.0f;
    float centroid_y = 0.0f;
    for (uint16_t i = 0u; i < count; ++i) {
        weights[i] /= sum_w; /* normalize */
        centroid_x += weights[i] * state->particle_buffer[i * 2u];
        centroid_y += weights[i] * state->particle_buffer[i * 2u + 1u];
    }

    out_box->x = (int32_t)(centroid_x - (float)state->box_width / 2.0f);
    out_box->y = (int32_t)(centroid_y - (float)state->box_height / 2.0f);
    out_box->width = state->box_width;
    out_box->height = state->box_height;

    /* SIR resample: cumulative weights + uniform draw. */
    for (uint16_t i = 1u; i < count; ++i) {
        weights[i] += weights[i - 1u];
    }
    weights[count - 1u] = 1.0f; /* guard float round-off leaving 1.0 unreachable */

    float resampled[CV_PARTICLE_MAX_COUNT * 2u];
    for (uint16_t i = 0u; i < count; ++i) {
        float draw = xorshift_unit(&state->rng_state);
        uint16_t j = 0u;
        while (j < count - 1u && draw > weights[j]) {
            ++j;
        }
        resampled[i * 2u] = state->particle_buffer[j * 2u];
        resampled[i * 2u + 1u] = state->particle_buffer[j * 2u + 1u];
    }
    memcpy(state->particle_buffer, resampled, (size_t)count * 2u * sizeof(float));

    return EMBEDDIP_OK;
}

embeddip_status_t
cv_particle_update(CvParticleState *state, const ImageView *frame, Rectangle *out_box)
{
    if (state == NULL || frame == NULL || out_box == NULL || frame->pixels == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!state->initialized) {
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }
    bool hist_mode = state->hist_nbins > 0u;
    bool fmt_ok =
        hist_mode ? cv_format_is_gray_or_rgb565(frame->format) : cv_format_is_gray(frame->format);
    if (!fmt_ok) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    /* Diffuse particles (random walk). The histogram path needs a wider
     * search radius than the legacy path since it has no motion model:
     * resampling can only follow the target if diffusion occasionally
     * places particles over its new position. */
    float diffuse_range = hist_mode ? 10.0f : 3.0f;
    for (uint16_t i = 0u; i < state->particle_count; ++i) {
        state->particle_buffer[i * 2u] += xorshift_range(&state->rng_state, diffuse_range);
        state->particle_buffer[i * 2u + 1u] += xorshift_range(&state->rng_state, diffuse_range);
    }

    if (hist_mode) {
        return cv_particle_update_hist(state, frame, out_box);
    }

    /* Weight by local gradient magnitude (gradient peaks stand in for corner
     * strength; embedDIP has no Harris-response primitive). */
    Image gray_image;
    particle_image_from_view(frame, &gray_image);

    /* gaussianGradients/gradientMagnitude ignore ->pixels on their output
     * Images entirely: they memory_alloc a fresh chals->ch[0] buffer and
     * write results there (see imgproc/filter.c). So outputs must start
     * with chals == NULL and the actual results are read back from
     * ->chals->ch[0], not ->pixels. Each call's allocation is freed below
     * once the magnitude values have been consumed, to avoid a per-frame
     * leak. */
    Image gx_img = {.width = frame->width,
                    .height = frame->height,
                    .pixels = NULL,
                    .chals = NULL,
                    .size = frame->width * frame->height,
                    .format = IMAGE_FORMAT_GRAYSCALE,
                    .depth = IMAGE_DEPTH_F32,
                    .log = IMAGE_DATA_PIXELS,
                    .is_chals = false};
    Image gy_img = gx_img;
    Image mag_img = gx_img;

    embeddip_status_t status = gaussianGradients(&gray_image, &gx_img, &gy_img, 1.0f);
    if (status != EMBEDDIP_OK) {
        /* OOM paths in gaussianGradients can leave the chals struct
         * allocated (with ch[0] still NULL) even on failure; free
         * whatever got allocated before returning. */
        particle_free_image_chals(&gx_img);
        particle_free_image_chals(&gy_img);
        return status;
    }
    status = gradientMagnitude(&gx_img, &gy_img, &mag_img);
    if (status != EMBEDDIP_OK) {
        particle_free_image_chals(&gx_img);
        particle_free_image_chals(&gy_img);
        particle_free_image_chals(&mag_img);
        return status;
    }

    float sum_x = 0.0f;
    float sum_y = 0.0f;
    float sum_w = 0.0f;
    const float *mag = (const float *)mag_img.chals->ch[0];

    for (uint16_t i = 0u; i < state->particle_count; ++i) {
        float px = state->particle_buffer[i * 2u];
        float py = state->particle_buffer[i * 2u + 1u];
        int32_t ix = (int32_t)px;
        int32_t iy = (int32_t)py;
        if (ix < 0 || iy < 0 || (uint32_t)ix >= frame->width || (uint32_t)iy >= frame->height) {
            continue;
        }
        float weight = mag[(size_t)iy * frame->width + (size_t)ix] + 1e-3f;
        sum_x += px * weight;
        sum_y += py * weight;
        sum_w += weight;
    }

    particle_free_image_chals(&gx_img);
    particle_free_image_chals(&gy_img);
    particle_free_image_chals(&mag_img);

    if (sum_w <= 0.0f) {
        return EMBEDDIP_ERROR_UNKNOWN;
    }

    float centroid_x = sum_x / sum_w;
    float centroid_y = sum_y / sum_w;
    out_box->x = (int32_t)(centroid_x - (float)state->box_width / 2.0f);
    out_box->y = (int32_t)(centroid_y - (float)state->box_height / 2.0f);
    out_box->width = state->box_width;
    out_box->height = state->box_height;
    return EMBEDDIP_OK;
}

void cv_particle_free(CvParticleState *state)
{
    /* particle_buffer is caller-owned (no malloc happens in this module);
     * just drop the state's reference so a stale state can't be reused
     * without re-initializing. */
    if (state == NULL) {
        return;
    }
    state->particle_buffer = NULL;
    state->particle_count = 0u;
    state->initialized = false;
}

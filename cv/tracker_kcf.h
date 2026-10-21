// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_TRACKER_KCF_H
#define EMBEDDIP_CV_TRACKER_KCF_H

#include <stdbool.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Fixed patch edge length in pixels. Must be a power of 2 to satisfy
 * imgproc/fft.c's isValidFFTSize check. */
#define CV_KCF_PATCH_SIZE 64u

/**
 * @brief KCF (Kernel Correlation Filter) tracker state.
 *
 * `template_spectrum` is a heap-allocated Image owned by this state, holding
 * the forward FFT of the learned template patch (real/imag interleaved via
 * imgproc/fft.c's Image.chals convention: ch[0]=real, ch[1]=imag).
 *
 * The remaining Image* fields are per-frame scratch buffers used by
 * cv_kcf_update. They are all fixed CV_KCF_PATCH_SIZE x CV_KCF_PATCH_SIZE and
 * are allocated exactly once (in cv_kcf_init) and reused every call to
 * cv_kcf_update, instead of being alloc/freed per frame — some board
 * allocators (e.g. a bump allocator with a no-op free) never reclaim freed
 * memory, so per-frame alloc/free would exhaust the heap after one frame.
 * Every producer of these buffers (kcf_extract_patch, fft(), ifft(),
 * kcf_conj_multiply) fully overwrites every element on each call, so no
 * per-frame clear is needed between reuses.
 *
 * - search_patch:     grayscale patch sampled from the current search roi.
 * - search_spectrum:  forward FFT of search_patch.
 * - corr_spectrum:    search_spectrum * conj(template_spectrum).
 * - corr_time:        inverse FFT of corr_spectrum (peak is searched here).
 * - adapt_patch:      grayscale patch sampled at the newly tracked roi, used
 *                      only for online template adaptation (learn_rate > 0).
 * - adapt_spectrum:   forward FFT of adapt_patch, blended into
 *                      template_spectrum.
 *
 * All of the above are allocated in cv_kcf_init and released by
 * cv_kcf_free.
 */
typedef struct {
    Image *template_spectrum;
    Image *search_patch;
    Image *search_spectrum;
    Image *corr_spectrum;
    Image *corr_time;
    Image *adapt_patch;
    Image *adapt_spectrum;
    int32_t box_width;
    int32_t box_height;
    int32_t center_x;
    int32_t center_y;
    bool initialized;
    /** Online template adaptation rate in [0,1]; 0 disables adaptation
     * (template stays fixed at the init spectrum, matching the original
     * behavior). Set to 0.075f by cv_kcf_init. */
    float learn_rate;
} CvKcfState;

/**
 * @brief Initialize the tracker: extract and store a template patch's spectrum.
 *
 * @param[out] state Tracker state to initialize.
 * @param[in] src Grayscale (or mask) image view to sample the template from.
 * @param[in] roi Initial bounding box; the patch is resampled/cropped to
 *            CV_KCF_PATCH_SIZE x CV_KCF_PATCH_SIZE around its center.
 *
 * Also preallocates every scratch Image cv_kcf_update will reuse per frame
 * (see CvKcfState); on any allocation failure, everything allocated so far
 * is released before returning.
 *
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state or src is
 *         NULL, EMBEDDIP_ERROR_INVALID_FORMAT if src is not grayscale/mask,
 *         EMBEDDIP_ERROR_INVALID_SIZE if roi is out of bounds,
 *         EMBEDDIP_ERROR_OUT_OF_MEMORY if the spectrum Image or any scratch
 *         Image cannot be allocated.
 */
embeddip_status_t cv_kcf_init(CvKcfState *state, const ImageView *src, Rectangle roi);

/**
 * @brief Locate the tracked object in a new frame via correlation-filter response.
 *
 * @param[in,out] state Tracker state (template updated in place after match).
 * @param[in] frame Grayscale (or mask) image view to search.
 * @param[out] out_box New bounding box (same size as init roi).
 * @return EMBEDDIP_OK on success, EMBEDDIP_ERROR_NULL_PTR if state, frame, or
 *         out_box is NULL, EMBEDDIP_ERROR_NOT_INITIALIZED if cv_kcf_init was
 *         not called first, EMBEDDIP_ERROR_INVALID_FORMAT if frame is not
 *         grayscale/mask. Allocates no memory: all scratch Images were
 *         preallocated by cv_kcf_init and are reused every call.
 */
embeddip_status_t cv_kcf_update(CvKcfState *state, const ImageView *frame, Rectangle *out_box);

/**
 * @brief Release all heap Images owned by state (template_spectrum plus the
 * per-frame scratch buffers preallocated by cv_kcf_init).
 *
 * Safe to call on a zero-initialized, partially-initialized, or
 * already-freed state (each Image* is only deleted, and set back to NULL,
 * if non-NULL). Must be called before the state goes out of scope to avoid
 * leaking the owned Images' chals/pixels buffers.
 *
 * @param[in,out] state Tracker state to release.
 * @return EMBEDDIP_OK always (EMBEDDIP_ERROR_NULL_PTR if state is NULL).
 */
embeddip_status_t cv_kcf_free(CvKcfState *state);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_TRACKER_KCF_H */

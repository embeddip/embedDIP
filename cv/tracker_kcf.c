// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/tracker_kcf.h"

#include "board/common.h"
#include "imgproc/fft.h"

#include <stdint.h>
#include <string.h>

#include "cv/image_gray.h"

/* Crop and nearest-neighbor resample a src region to
 * CV_KCF_PATCH_SIZE x CV_KCF_PATCH_SIZE, writing into a heap Image's
 * pixels buffer (fft() reads src->pixels as uint8_t regardless of
 * ImageFormat). */
static void kcf_extract_patch(const ImageView *src, Rectangle roi, uint8_t *out_pixels)
{
    for (uint32_t py = 0u; py < CV_KCF_PATCH_SIZE; ++py) {
        int32_t sy = roi.y + (int32_t)((int64_t)py * roi.height / CV_KCF_PATCH_SIZE);
        const uint8_t *row = src->pixels + (size_t)sy * src->row_stride_bytes;
        for (uint32_t px = 0u; px < CV_KCF_PATCH_SIZE; ++px) {
            int32_t sx = roi.x + (int32_t)((int64_t)px * roi.width / CV_KCF_PATCH_SIZE);
            out_pixels[py * CV_KCF_PATCH_SIZE + px] = row[sx];
        }
    }
}

/* Build a heap Image holding a CV_KCF_PATCH_SIZE^2 grayscale patch sampled
 * from src's roi. Caller must deleteImage() the result. */
static embeddip_status_t
kcf_make_patch_image(const ImageView *src, Rectangle roi, Image **out_patch)
{
    Image *patch = NULL;
    embeddip_status_t status = createImageWH(
        (int)CV_KCF_PATCH_SIZE, (int)CV_KCF_PATCH_SIZE, IMAGE_FORMAT_GRAYSCALE, &patch);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    kcf_extract_patch(src, roi, (uint8_t *)patch->pixels);
    *out_patch = patch;
    return EMBEDDIP_OK;
}

/* Multiply search-patch spectrum by the conjugate of the template spectrum,
 * writing an already-chals-allocated complex spectrum Image (corr->chals
 * must be pre-allocated via createChalsComplex(corr, 2) by the caller).
 *
 * fft() writes interleaved complex data (re,im,re,im,...) into ch[0] and
 * then copies the whole interleaved buffer into ch[1] too, so ch[0] and
 * ch[1] are identical 2*num_bins-float interleaved buffers on both inputs
 * (imgproc/fft.c:50-77). ifft() (when src->log == IMAGE_DATA_COMPLEX) reads
 * its input as an interleaved buffer from ch[1] (imgproc/fft.c:99), so the
 * output here must be written as interleaved complex into both ch[0] and
 * ch[1] to match that shape. */
static void kcf_conj_multiply(const Image *search_spec, const Image *template_spec, Image *corr)
{
    uint32_t num_bins = search_spec->width * search_spec->height;
    const float *a = search_spec->chals->ch[0];
    const float *b = template_spec->chals->ch[0];
    float *out0 = corr->chals->ch[0];
    float *out1 = corr->chals->ch[1];

    for (uint32_t i = 0u; i < num_bins; ++i) {
        float a_re = a[2u * i];
        float a_im = a[2u * i + 1u];
        float b_re = b[2u * i];
        float b_im = b[2u * i + 1u];
        /* (a) * conj(b) = (a_re*b_re + a_im*b_im) + j(a_im*b_re - a_re*b_im) */
        float re = a_re * b_re + a_im * b_im;
        float im = a_im * b_re - a_re * b_im;
        out0[2u * i] = re;
        out0[2u * i + 1u] = im;
        out1[2u * i] = re;
        out1[2u * i + 1u] = im;
    }
    /* Zero the DC bin (i=0): both patches carry a large constant background
     * level (mostly-zero pixels with a small foreground block), so the DC
     * term's conjugate product is orders of magnitude larger than every AC
     * bin. At float32 precision this swamps the AC terms entirely, so the
     * inverse transform collapses to a near-constant image and the peak
     * search becomes meaningless. Dropping DC (a standard trick for
     * correlation-based trackers, equivalent to correlating mean-subtracted
     * patches) lets the actual shape/position information dominate. */
    out0[0] = 0.0f;
    out0[1] = 0.0f;
    out1[0] = 0.0f;
    out1[1] = 0.0f;
    corr->log = IMAGE_DATA_COMPLEX;
}

/* Find the index of the largest real correlation value, then convert its
 * circular position into a signed (dx,dy) pixel offset in patch space
 * (values in [0, N/2) map to positive offsets, [N/2, N) wrap to negative). */
static void kcf_find_peak_offset(const Image *corr_time, int32_t *out_dx, int32_t *out_dy)
{
    uint32_t n = corr_time->width;
    const float *data = corr_time->chals->ch[0];
    uint32_t best_idx = 0u;
    float best_val = data[0];
    for (uint32_t i = 1u; i < n * n; ++i) {
        if (data[i] > best_val) {
            best_val = data[i];
            best_idx = i;
        }
    }
    int32_t x = (int32_t)(best_idx % n);
    int32_t y = (int32_t)(best_idx / n);
    int32_t half = (int32_t)n / 2;
    *out_dx = (x >= half) ? (x - (int32_t)n) : x;
    *out_dy = (y >= half) ? (y - (int32_t)n) : y;
}

embeddip_status_t cv_kcf_init(CvKcfState *state, const ImageView *src, Rectangle roi)
{
    if (state == NULL || src == NULL || src->pixels == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!cv_format_is_gray(src->format)) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }
    if (roi.width <= 0 || roi.height <= 0 || roi.x < 0 || roi.y < 0 ||
        (uint32_t)(roi.x + roi.width) > src->width ||
        (uint32_t)(roi.y + roi.height) > src->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    memset(state, 0, sizeof(*state));

    Image *patch = NULL;
    embeddip_status_t status = kcf_make_patch_image(src, roi, &patch);
    if (status != EMBEDDIP_OK) {
        return status;
    }

    Image *spectrum = NULL;
    status = createImageWH(
        (int)CV_KCF_PATCH_SIZE, (int)CV_KCF_PATCH_SIZE, IMAGE_FORMAT_GRAYSCALE, &spectrum);
    if (status != EMBEDDIP_OK) {
        deleteImage(patch);
        return status;
    }

    status = fft(patch, spectrum);
    deleteImage(patch);
    if (status != EMBEDDIP_OK) {
        deleteImage(spectrum);
        return status;
    }
    state->template_spectrum = spectrum;

    /* Preallocate every per-frame scratch buffer cv_kcf_update will reuse
     * (see CvKcfState doc comment). On any failure below, release whatever
     * was already allocated (including template_spectrum above) and bail. */
    status = createImageWH((int)CV_KCF_PATCH_SIZE,
                           (int)CV_KCF_PATCH_SIZE,
                           IMAGE_FORMAT_GRAYSCALE,
                           &state->search_patch);
    if (status == EMBEDDIP_OK) {
        status = createImageWH((int)CV_KCF_PATCH_SIZE,
                               (int)CV_KCF_PATCH_SIZE,
                               IMAGE_FORMAT_GRAYSCALE,
                               &state->search_spectrum);
    }
    if (status == EMBEDDIP_OK) {
        status = createImageWH((int)CV_KCF_PATCH_SIZE,
                               (int)CV_KCF_PATCH_SIZE,
                               IMAGE_FORMAT_GRAYSCALE,
                               &state->corr_spectrum);
    }
    if (status == EMBEDDIP_OK) {
        status = createChalsComplex(state->corr_spectrum, 2u);
    }
    if (status == EMBEDDIP_OK) {
        status = createImageWH((int)CV_KCF_PATCH_SIZE,
                               (int)CV_KCF_PATCH_SIZE,
                               IMAGE_FORMAT_GRAYSCALE,
                               &state->corr_time);
    }
    if (status == EMBEDDIP_OK) {
        status = createImageWH((int)CV_KCF_PATCH_SIZE,
                               (int)CV_KCF_PATCH_SIZE,
                               IMAGE_FORMAT_GRAYSCALE,
                               &state->adapt_patch);
    }
    if (status == EMBEDDIP_OK) {
        status = createImageWH((int)CV_KCF_PATCH_SIZE,
                               (int)CV_KCF_PATCH_SIZE,
                               IMAGE_FORMAT_GRAYSCALE,
                               &state->adapt_spectrum);
    }
    if (status != EMBEDDIP_OK) {
        cv_kcf_free(state);
        return status;
    }

    state->box_width = roi.width;
    state->box_height = roi.height;
    state->center_x = roi.x + roi.width / 2;
    state->center_y = roi.y + roi.height / 2;
    state->initialized = true;
    state->learn_rate = 0.075f;
    return EMBEDDIP_OK;
}

embeddip_status_t cv_kcf_update(CvKcfState *state, const ImageView *frame, Rectangle *out_box)
{
    if (state == NULL || frame == NULL || out_box == NULL || frame->pixels == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!state->initialized) {
        return EMBEDDIP_ERROR_NOT_INITIALIZED;
    }
    if (!cv_format_is_gray(frame->format)) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    Rectangle search_roi = cv_clamp_centered_box(state->center_x,
                                                 state->center_y,
                                                 state->box_width,
                                                 state->box_height,
                                                 frame->width,
                                                 frame->height);

    /* All buffers below are preallocated once by cv_kcf_init and reused
     * every frame (see CvKcfState doc comment) — no create/delete here, so
     * this function performs zero heap traffic per call. Each producer
     * (kcf_extract_patch, fft, ifft, kcf_conj_multiply) fully overwrites
     * every element of its output on every call, so no per-frame clear is
     * needed before reuse. */
    kcf_extract_patch(frame, search_roi, (uint8_t *)state->search_patch->pixels);

    embeddip_status_t status = fft(state->search_patch, state->search_spectrum);
    if (status != EMBEDDIP_OK) {
        return status;
    }

    kcf_conj_multiply(state->search_spectrum, state->template_spectrum, state->corr_spectrum);

    status = ifft(state->corr_spectrum, state->corr_time);
    if (status != EMBEDDIP_OK) {
        return status;
    }

    int32_t peak_dx = 0;
    int32_t peak_dy = 0;
    kcf_find_peak_offset(state->corr_time, &peak_dx, &peak_dy);

    int32_t offset_x = peak_dx * search_roi.width / (int32_t)CV_KCF_PATCH_SIZE;
    int32_t offset_y = peak_dy * search_roi.height / (int32_t)CV_KCF_PATCH_SIZE;

    state->center_x = search_roi.x + search_roi.width / 2 + offset_x;
    state->center_y = search_roi.y + search_roi.height / 2 + offset_y;

    if (state->learn_rate > 0.0f) {
        Rectangle new_roi = cv_clamp_centered_box(state->center_x,
                                                  state->center_y,
                                                  state->box_width,
                                                  state->box_height,
                                                  frame->width,
                                                  frame->height);

        kcf_extract_patch(frame, new_roi, (uint8_t *)state->adapt_patch->pixels);
        status = fft(state->adapt_patch, state->adapt_spectrum);
        if (status == EMBEDDIP_OK) {
            float eta = state->learn_rate;
            uint32_t num_floats = 2u * state->adapt_spectrum->width * state->adapt_spectrum->height;
            /* Only ch[0] is ever read (kcf_conj_multiply above); ch[1] is a
             * redundant copy fft() makes (see its comment), so blending it
             * too would just be wasted work. */
            float *tmpl = state->template_spectrum->chals->ch[0];
            const float *fresh = state->adapt_spectrum->chals->ch[0];
            for (uint32_t i = 0u; i < num_floats; ++i) {
                tmpl[i] = (1.0f - eta) * tmpl[i] + eta * fresh[i];
            }
        }
    }

    out_box->x = state->center_x - state->box_width / 2;
    out_box->y = state->center_y - state->box_height / 2;
    out_box->width = state->box_width;
    out_box->height = state->box_height;
    return EMBEDDIP_OK;
}

embeddip_status_t cv_kcf_free(CvKcfState *state)
{
    if (state == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    Image **owned[] = {
        &state->template_spectrum,
        &state->search_patch,
        &state->search_spectrum,
        &state->corr_spectrum,
        &state->corr_time,
        &state->adapt_patch,
        &state->adapt_spectrum,
    };
    for (size_t i = 0u; i < sizeof(owned) / sizeof(owned[0]); ++i) {
        if (*owned[i] != NULL) {
            deleteImage(*owned[i]);
            *owned[i] = NULL;
        }
    }
    state->initialized = false;
    return EMBEDDIP_OK;
}

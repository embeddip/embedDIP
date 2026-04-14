// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "misc.h"

#include "board/common.h"
#include "core/memory_manager.h"

#include <float.h>
#include <math.h>
#include <string.h>

/**
 * @brief Helper function to normalize a float value to uint8_t range [0, 255].
 *
 * @param val Value to normalize
 * @param min Minimum value in the range
 * @param max Maximum value in the range
 * @return Normalized uint8_t value
 */
static inline uint8_t normalize_to_u8(uint8_t val, uint8_t min, uint8_t max)
{
    if (max - min < 1e-5f)  // avoid divide by zero
        return 0;
    float norm = ((float)val - (float)min) / ((float)max - (float)min);
    if (norm < 0.0f)
        norm = 0.0f;
    if (norm > 1.0f)
        norm = 1.0f;
    return (uint8_t)(norm * 255.0f);
}

/**
 * @brief Performs pixel-wise addition of two float-valued grayscale images.
 *
 * @param src1 Pointer to the first input image (float channel expected)
 * @param src2 Pointer to the second input image (float channel expected)
 * @param dst Pointer to the output image (float channel)
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t add(const Image *src1, const Image *src2, Image *dst)
{
    // NOTE: Channel operations work on width*height pixels, NOT width*height*depth
    int totalPixels = src1->width * src1->height;

    if (isChalsEmpty(dst)) {
        embeddip_status_t status = createChals(dst, dst->depth);
        if (status != EMBEDDIP_OK) {
            return status;  // Memory allocation failed
        }
        dst->is_chals = 1;
    }

    // Safety check: ensure channel was actually allocated
    if (dst->chals == NULL || dst->chals->ch[0] == NULL) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // Both in pixels buffer
    if (src1->log == IMAGE_DATA_PIXELS && src2->log == IMAGE_DATA_PIXELS) {
        uint8_t *src1_data = src1->pixels;
        uint8_t *src2_data = src2->pixels;
        float *dst_data = dst->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i) {
            dst_data[i] = src1_data[i] + src2_data[i];
        }
    }
    // src1 in pixels, src2 in channels
    else if (src1->log == IMAGE_DATA_PIXELS && src2->log != IMAGE_DATA_PIXELS) {
        if (isChalsEmpty(src2)) {
            embeddip_status_t status = createChals((Image *)src2, src2->depth);
            if (status != EMBEDDIP_OK) {
                return status;
            }
        }

        int ch2_idx = 0;
        if (src2->log >= IMAGE_DATA_CH0 && src2->log <= IMAGE_DATA_CH5) {
            ch2_idx = src2->log - IMAGE_DATA_CH0;
        }

        // Safety check
        if (src2->chals == NULL || src2->chals->ch[ch2_idx] == NULL) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }

        uint8_t *src1_data = src1->pixels;
        float *src2_data = src2->chals->ch[ch2_idx];
        float *dst_data = dst->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i) {
            dst_data[i] = src1_data[i] + src2_data[i];
        }
    }
    // src1 in channels, src2 in pixels
    else if (src1->log != IMAGE_DATA_PIXELS && src2->log == IMAGE_DATA_PIXELS) {
        if (isChalsEmpty(src1)) {
            embeddip_status_t status = createChals((Image *)src1, src1->depth);
            if (status != EMBEDDIP_OK) {
                return status;
            }
        }

        int ch1_idx = 0;
        if (src1->log >= IMAGE_DATA_CH0 && src1->log <= IMAGE_DATA_CH5) {
            ch1_idx = src1->log - IMAGE_DATA_CH0;
        }

        // Safety check
        if (src1->chals == NULL || src1->chals->ch[ch1_idx] == NULL) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }

        float *src1_data = src1->chals->ch[ch1_idx];
        uint8_t *src2_data = src2->pixels;
        float *dst_data = dst->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i) {
            dst_data[i] = src1_data[i] + src2_data[i];
        }
    }
    // Both in channels
    else {
        if (isChalsEmpty(src1)) {
            embeddip_status_t status = createChals((Image *)src1, src1->depth);
            if (status != EMBEDDIP_OK) {
                return status;
            }
        }
        if (isChalsEmpty(src2)) {
            embeddip_status_t status = createChals((Image *)src2, src2->depth);
            if (status != EMBEDDIP_OK) {
                return status;
            }
        }

        int ch1_idx = 0;
        if (src1->log >= IMAGE_DATA_CH0 && src1->log <= IMAGE_DATA_CH5) {
            ch1_idx = src1->log - IMAGE_DATA_CH0;
        }

        int ch2_idx = 0;
        if (src2->log >= IMAGE_DATA_CH0 && src2->log <= IMAGE_DATA_CH5) {
            ch2_idx = src2->log - IMAGE_DATA_CH0;
        }

        // Safety checks
        if (src1->chals == NULL || src1->chals->ch[ch1_idx] == NULL) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
        if (src2->chals == NULL || src2->chals->ch[ch2_idx] == NULL) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }

        float *src1_data = src1->chals->ch[ch1_idx];
        float *src2_data = src2->chals->ch[ch2_idx];
        float *dst_data = dst->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i) {
            dst_data[i] = src1_data[i] + src2_data[i];
        }
    }

    dst->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/**
 * @brief Normalizes pixel values to the range [0, 255].
 *
 * @param img Pointer to the image to normalize (modified in-place)
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t normalize(Image *img)
{
    uint8_t *data = (uint8_t *)img->pixels;

    uint8_t min = 255, max = 0;
    for (uint32_t i = 0; i < img->size; i++) {
        uint8_t v = (uint8_t)data[i];
        if (v < min)
            min = v;
        if (v > max)
            max = v;
    }
    for (uint32_t i = 0; i < img->size; i++) {
        data[i] = normalize_to_u8(data[i], min, max);
    }
    return EMBEDDIP_OK;
}

/**
 * @brief Converts image channel data back to pixel buffer with normalization.
 *
 * @param img Pointer to the image to convert (modified in-place)
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t convertTo(Image *img)
{
    uint8_t *data = (uint8_t *)img->pixels;

    switch (img->format) {
    case IMAGE_FORMAT_MASK:
        // Masks are already binary (0/255) or trimap (0/1/2), no conversion needed
        // Just ensure log state is set to PIXELS
        img->log = IMAGE_DATA_PIXELS;
        return EMBEDDIP_OK;

    case IMAGE_FORMAT_GRAYSCALE: {
        if (img->log == IMAGE_DATA_CH0 || img->log == IMAGE_DATA_MAGNITUDE ||
            img->log == IMAGE_DATA_PHASE) {
            float min = FLT_MAX, max = -FLT_MAX;

            // Step 1: Find min and max
            for (uint32_t i = 0; i < img->size; i++) {
                float v = img->chals->ch[0][i];
                if (v < min)
                    min = v;
                if (v > max)
                    max = v;
            }

            // Step 2: Normalize to [0, 255] and clamp
            if (max != min) {
                for (uint32_t i = 0; i < img->size; i++) {
                    float norm = (img->chals->ch[0][i] - min) / (max - min);
                    data[i] = (uint8_t)(norm * 255.0f + 0.5f);  // +0.5 for rounding
                }
            } else {
                // All values are the same, map to 0 or 255
                memset(data, 0, img->size);  // Or use 255
            }
        } else if (img->log == IMAGE_DATA_PIXELS) {
            uint8_t min = 255, max = 0;
            for (uint32_t i = 0; i < img->size; i++) {
                uint8_t v = (uint8_t)data[i];
                if (v < min)
                    min = v;
                if (v > max)
                    max = v;
            }
            for (uint32_t i = 0; i < img->size; i++) {
                data[i] = normalize_to_u8(data[i], min, max);
            }
        }

        break;
    }

    case IMAGE_FORMAT_RGB888:
    case IMAGE_FORMAT_YUV:
    case IMAGE_FORMAT_HSI: {
        int ch_r = 1, ch_g = 2, ch_b = 3;
        float min_r = FLT_MAX, max_r = -FLT_MAX;
        float min_g = FLT_MAX, max_g = -FLT_MAX;
        float min_b = FLT_MAX, max_b = -FLT_MAX;

        for (uint32_t i = 0; i < img->size; i++) {
            if (img->chals->ch[ch_r][i] < min_r)
                min_r = img->chals->ch[ch_r][i];
            if (img->chals->ch[ch_r][i] > max_r)
                max_r = img->chals->ch[ch_r][i];
            if (img->chals->ch[ch_g][i] < min_g)
                min_g = img->chals->ch[ch_g][i];
            if (img->chals->ch[ch_g][i] > max_g)
                max_g = img->chals->ch[ch_g][i];
            if (img->chals->ch[ch_b][i] < min_b)
                min_b = img->chals->ch[ch_b][i];
            if (img->chals->ch[ch_b][i] > max_b)
                max_b = img->chals->ch[ch_b][i];
        }

        for (uint32_t i = 0; i < img->size; i++) {
            data[i * 3 + 0] = normalize_to_u8(img->chals->ch[ch_b][i], min_b, max_b);
            data[i * 3 + 1] = normalize_to_u8(img->chals->ch[ch_g][i], min_g, max_g);
            data[i * 3 + 2] = normalize_to_u8(img->chals->ch[ch_r][i], min_r, max_r);
        }
        break;
    }

    case IMAGE_FORMAT_RGB565: {
        float min_r = FLT_MAX, max_r = -FLT_MAX;
        float min_g = FLT_MAX, max_g = -FLT_MAX;
        float min_b = FLT_MAX, max_b = -FLT_MAX;

        for (uint32_t i = 0; i < img->size; i++) {
            if (img->chals->ch[1][i] < min_r)
                min_r = img->chals->ch[1][i];
            if (img->chals->ch[1][i] > max_r)
                max_r = img->chals->ch[1][i];
            if (img->chals->ch[2][i] < min_g)
                min_g = img->chals->ch[2][i];
            if (img->chals->ch[2][i] > max_g)
                max_g = img->chals->ch[2][i];
            if (img->chals->ch[3][i] < min_b)
                min_b = img->chals->ch[3][i];
            if (img->chals->ch[3][i] > max_b)
                max_b = img->chals->ch[3][i];
        }

        for (uint32_t i = 0; i < img->size; i++) {
            uint8_t r = normalize_to_u8(img->chals->ch[1][i], min_r, max_r);
            uint8_t g = normalize_to_u8(img->chals->ch[2][i], min_g, max_g);
            uint8_t b = normalize_to_u8(img->chals->ch[3][i], min_b, max_b);

            uint16_t rgb565 = (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
            data[i * 2 + 0] = (uint8_t)(rgb565 & 0xFF);
            data[i * 2 + 1] = (uint8_t)((rgb565 >> 8) & 0xFF);
        }
        break;
    }

    default:
        break;
    }

    img->log = IMAGE_DATA_PIXELS;
    return EMBEDDIP_OK;
}

/**
 * @brief Computes the color distance of each pixel in an RGB
 * image to a given reference color.
 *
 * @param[in] inImg Pointer to the input RGB image (3 channels, interleaved as RGBRGB...).
 * @param[out] outImg Pointer to the output grayscale image (1 channel, same width and height as
 * input).
 * @param[in] R_ref Reference Red channel value (0–255).
 * @param[in] G_ref Reference Green channel value (0–255).
 * @param[in] B_ref Reference Blue channel value (0–255).
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t
dist(const Image *inImg, Image *outImg, uint8_t R_ref, uint8_t G_ref, uint8_t B_ref)
{
    int totalPixels = inImg->width * inImg->height;

    if (isChalsEmpty(outImg)) {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (inImg->log == IMAGE_DATA_PIXELS) {
        // Raw byte access
        uint8_t *inData = (uint8_t *)inImg->pixels;

        for (int i = 0; i < totalPixels; ++i) {
            int idx = i * 3;
            uint8_t R = inData[idx];
            uint8_t G = inData[idx + 1];
            uint8_t B = inData[idx + 2];

            float d = sqrtf((R - R_ref) * (R - R_ref) + (G - G_ref) * (G - G_ref) +
                            (B - B_ref) * (B - B_ref));

            outImg->chals->ch[0][i] = d;
        }
    } else {
        // Channel access (float-based) - RGB is in ch[1], ch[2], ch[3]
        if (isChalsEmpty(inImg)) {
            createChals((Image *)inImg, inImg->depth);
        }

        float *R_ch = inImg->chals->ch[1];
        float *G_ch = inImg->chals->ch[2];
        float *B_ch = inImg->chals->ch[3];

        for (int i = 0; i < totalPixels; ++i) {
            float d = sqrtf((R_ch[i] - R_ref) * (R_ch[i] - R_ref) +
                            (G_ch[i] - G_ref) * (G_ch[i] - G_ref) +
                            (B_ch[i] - B_ref) * (B_ch[i] - B_ref));

            outImg->chals->ch[0][i] = d;
        }
    }

    outImg->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/**
 * @brief Bitwise AND operation on binary masks
 *
 * @param img1 First input binary mask
 * @param img2 Second input binary mask
 * @param outImg Output binary mask
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t _and_(const Image *img1, const Image *img2, Image *outImg)
{
    // Input validation
    if (!img1 || !img2 || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!img1->pixels || !img2->pixels || !outImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (img1->format != outImg->format) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (img2->format != IMAGE_FORMAT_GRAYSCALE && img2->format != IMAGE_FORMAT_MASK) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    if (img1->width != img2->width || img1->height != img2->height ||
        img1->width != outImg->width || img1->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    const uint8_t *ps = (const uint8_t *)img1->pixels;
    const uint8_t *pm = (const uint8_t *)img2->pixels;
    uint8_t *po = (uint8_t *)outImg->pixels;

    int channels = (img1->format == IMAGE_FORMAT_GRAYSCALE) ? 1 : 3;

    for (uint32_t i = 0; i < img1->size; ++i) {
        if (pm[i]) {
            // copy original pixel
            for (int c = 0; c < channels; ++c) {
                po[i * channels + c] = ps[i * channels + c];
            }
        } else {
            // zero out pixel
            for (int c = 0; c < channels; ++c) {
                po[i * channels + c] = 0;
            }
        }
    }

    return EMBEDDIP_OK;
}

/**
 * @brief Element-wise bitwise OR operation on binary masks.
 *
 * @param[in]  img1     First input binary mask (grayscale).
 * @param[in]  img2     Second input binary mask (grayscale).
 * @param[out] outImg   Output binary mask (grayscale).
 */
embeddip_status_t _or(const Image *src1, const Image *src2, Image *dst)
{
    if (!src1 || !src2 || !dst)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (src1->format != IMAGE_FORMAT_GRAYSCALE || src2->format != IMAGE_FORMAT_GRAYSCALE ||
        dst->format != IMAGE_FORMAT_GRAYSCALE)
        return EMBEDDIP_ERROR_INVALID_FORMAT;

    if (src1->size != src2->size || src2->size != dst->size)
        return EMBEDDIP_ERROR_INVALID_SIZE;

    const uint8_t *src1_data = (const uint8_t *)src1->pixels;
    const uint8_t *src2_data = (const uint8_t *)src2->pixels;
    uint8_t *dst_data = (uint8_t *)dst->pixels;

    for (uint32_t i = 0; i < src1->size; ++i) {
        dst_data[i] = src1_data[i] | src2_data[i];
    }

    return EMBEDDIP_OK;
}

/**
 * @brief Bitwise XOR operation on binary masks
 *
 * @param src1 First input binary mask
 * @param src2 Second input binary mask
 * @param dst Output binary mask
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t _xor(const Image *src1, const Image *src2, Image *dst)
{
    if (!src1 || !src2 || !dst)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (src1->width != src2->width || src1->height != src2->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;

    if (src1->log != IMAGE_DATA_PIXELS || src2->log != IMAGE_DATA_PIXELS)
        return EMBEDDIP_ERROR_INVALID_FORMAT;

    int size = src1->width * src1->height;

    if (dst->pixels == NULL) {
        dst->pixels = memory_alloc(size * sizeof(uint8_t));
        dst->log = IMAGE_DATA_PIXELS;
    }

    const uint8_t *src1_data = src1->pixels;
    const uint8_t *src2_data = src2->pixels;
    uint8_t *dst_data = dst->pixels;

    for (int i = 0; i < size; ++i)
        dst_data[i] = src1_data[i] ^ src2_data[i];

    return EMBEDDIP_OK;
}

/**
 * @brief Bitwise NOT operation on binary mask
 *
 * @param src Input binary mask
 * @param dst Output binary mask
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t _not(const Image *src, Image *dst)
{
    if (!src || !dst)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (src->log != IMAGE_DATA_PIXELS)
        return EMBEDDIP_ERROR_INVALID_FORMAT;

    int size = src->width * src->height;

    if (dst->pixels == NULL) {
        dst->pixels = memory_alloc(size * sizeof(uint8_t));
        dst->log = IMAGE_DATA_PIXELS;
    }

    const uint8_t *src_data = src->pixels;
    uint8_t *dst_data = dst->pixels;

    for (int i = 0; i < size; ++i)
        dst_data[i] = ~src_data[i];

    return EMBEDDIP_OK;
}

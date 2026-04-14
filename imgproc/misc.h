// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_MISC_H
#define EMBEDDIP_MISC_H

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Performs pixel-wise addition of two float-valued grayscale images.
 *
 * @param src1 Pointer to the first input image (float channel expected)
 * @param src2 Pointer to the second input image (float channel expected)
 * @param dst Pointer to the output image (float channel)
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t add(const Image *src1, const Image *src2, Image *dst);

/**
 * @brief Normalizes pixel values to the range [0, 255].
 *
 * @param img Pointer to the image to normalize (modified in-place)
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t normalize(Image *img);

/**
 * @brief Converts image channel data back to pixel buffer with normalization.
 *
 * @param img Pointer to the image to convert (modified in-place)
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t convertTo(Image *img);

/**
 * @brief Computes the color distance of each pixel in an RGB image to a given reference color.
 *
 * @param inImg Pointer to the input RGB image (3 channels, interleaved as RGBRGB...)
 * @param outImg Pointer to the output grayscale image (1 channel, same width and height as input)
 * @param R_ref Reference Red channel value (0–255)
 * @param G_ref Reference Green channel value (0–255)
 * @param B_ref Reference Blue channel value (0–255)
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t
dist(const Image *inImg, Image *outImg, uint8_t R_ref, uint8_t G_ref, uint8_t B_ref);

/**
 * @brief Bitwise AND operation on binary masks
 *
 * @param img1 First input binary mask
 * @param img2 Second input binary mask
 * @param outImg Output binary mask
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t _and_(const Image *img1, const Image *img2, Image *outImg);

/**
 * @brief Element-wise bitwise OR operation on binary masks.
 *
 * @param[in]  img1     First input binary mask (grayscale).
 * @param[in]  img2     Second input binary mask (grayscale).
 * @param[out] outImg   Output binary mask (grayscale).
 */
embeddip_status_t _or(const Image *src1, const Image *src2, Image *dst);

/**
 * @brief Bitwise XOR operation on binary masks
 *
 * @param src1 First input binary mask
 * @param src2 Second input binary mask
 * @param dst Output binary mask
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t _xor(const Image *src1, const Image *src2, Image *dst);

/**
 * @brief Bitwise NOT operation on binary mask
 *
 * @param src Input binary mask
 * @param dst Output binary mask
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t _not(const Image *src, Image *dst);

#ifdef __cplusplus
}
#endif

#endif  // EMBEDDIP_MISC_H

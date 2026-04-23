// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "morph.h"

#include "board/common.h"
#include "core/memory_manager.h"

#include <string.h>

embeddip_status_t getStructuringElement(Kernel *kernel, MorphShape shape, uint8_t size)
{
    // Validate inputs
    if (!kernel)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (size == 0 || size % 2 == 0)
        return EMBEDDIP_ERROR_INVALID_ARG;  // Must be odd
    if (shape < MORPH_RECT || shape > MORPH_ELLIPSE)
        return EMBEDDIP_ERROR_INVALID_ARG;

    kernel->size = size;
    kernel->anchor = size / 2;
    kernel->data = (uint8_t *)memory_alloc(size * size);
    if (!kernel->data)
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    memset(kernel->data, 0, size * size);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int idx = y * size + x;
            switch (shape) {
            case MORPH_RECT:
                kernel->data[idx] = 1;
                break;
            case MORPH_CROSS:
                if (x == size / 2 || y == size / 2)
                    kernel->data[idx] = 1;
                break;
            case MORPH_ELLIPSE: {
                float center = (float)size / 2.0f;
                float dx = (float)x - center;
                float dy = (float)y - center;
                float r = center;
                if ((dx * dx + dy * dy) <= r * r)
                    kernel->data[idx] = 1;
                break;
            }
            }
        }
    }
    return EMBEDDIP_OK;
}

/**
 * @brief Performs morphological erosion on a binary image.
 *
 * @param[in]  src         Pointer to the source binary image.
 * @param[out] dst         Pointer to the destination image to store the eroded result.
 * @param[in]  kernel      Pointer to the structuring element (Kernel) used for erosion.
 * @param[in]  iterations  Number of times erosion is applied.
 */
embeddip_status_t erode(const Image *src, Image *dst, const Kernel *kernel, uint8_t iterations)
{
    // Validate inputs
    if (!src || !dst || !kernel)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (!src->pixels || !dst->pixels || !kernel->data)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (src->format != IMAGE_FORMAT_GRAYSCALE || dst->format != IMAGE_FORMAT_GRAYSCALE)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (src->width != dst->width || src->height != dst->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (iterations == 0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    int w = src->width, h = src->height;
    int kSize = kernel->size;
    int anchor = kernel->anchor;

    uint8_t *ping = (uint8_t *)memory_alloc(src->size);
    if (!ping)
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    uint8_t *pong = (uint8_t *)memory_alloc(src->size);
    if (!pong) {
        memory_free(ping);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // First iteration must start from source image content.
    memcpy(ping, src->pixels, src->size);

    for (uint8_t it = 0; it < iterations; ++it) {
        uint8_t *in = ping;
        uint8_t *out = (it == iterations - 1) ? (uint8_t *)dst->pixels : pong;

        // Process entire image including borders
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int match = 1;
                for (int ky = 0; ky < kSize && match; ++ky) {
                    for (int kx = 0; kx < kSize; ++kx) {
                        if (!kernel->data[ky * kSize + kx])
                            continue;
                        int ix = x + kx - anchor;
                        int iy = y + ky - anchor;
                        // Boundary check: treat out-of-bounds as 0 (background)
                        if (ix < 0 || ix >= w || iy < 0 || iy >= h || in[iy * w + ix] == 0) {
                            match = 0;
                            break;
                        }
                    }
                }
                out[y * w + x] = match ? 255 : 0;
            }
        }

        // Swap ping/pong for next iteration
        if (it < iterations - 1) {
            uint8_t *tmp = ping;
            ping = pong;
            pong = tmp;
        }
    }

    memory_free(ping);
    memory_free(pong);
    return EMBEDDIP_OK;
}

/**
 * @brief Performs morphological dilation on a binary image.
 *
 * @param[in]  src         Pointer to the source binary image.
 * @param[out] dst         Pointer to the destination image to store the dilated result.
 * @param[in]  kernel      Pointer to the structuring element (Kernel) used for dilation.
 * @param[in]  iterations  Number of times dilation is applied.
 */
embeddip_status_t dilate(const Image *src, Image *dst, const Kernel *kernel, uint8_t iterations)
{
    // Validate inputs
    if (!src || !dst || !kernel)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (!src->pixels || !dst->pixels || !kernel->data)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (src->format != IMAGE_FORMAT_GRAYSCALE || dst->format != IMAGE_FORMAT_GRAYSCALE)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (src->width != dst->width || src->height != dst->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (iterations == 0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    int w = src->width, h = src->height;
    int kSize = kernel->size;
    int anchor = kernel->anchor;

    uint8_t *ping = (uint8_t *)memory_alloc(src->size);
    if (!ping)
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    uint8_t *pong = (uint8_t *)memory_alloc(src->size);
    if (!pong) {
        memory_free(ping);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // First iteration must start from source image content.
    memcpy(ping, src->pixels, src->size);

    for (uint8_t it = 0; it < iterations; ++it) {
        uint8_t *in = ping;
        uint8_t *out = (it == iterations - 1) ? (uint8_t *)dst->pixels : pong;

        // Process entire image including borders
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int match = 0;
                for (int ky = 0; ky < kSize && !match; ++ky) {
                    for (int kx = 0; kx < kSize; ++kx) {
                        if (!kernel->data[ky * kSize + kx])
                            continue;
                        int ix = x + kx - anchor;
                        int iy = y + ky - anchor;
                        // Boundary check: treat out-of-bounds as 0 (background)
                        if (ix >= 0 && ix < w && iy >= 0 && iy < h && in[iy * w + ix] != 0) {
                            match = 1;
                            break;
                        }
                    }
                }
                out[y * w + x] = match ? 255 : 0;
            }
        }

        // Swap ping/pong for next iteration
        if (it < iterations - 1) {
            uint8_t *tmp = ping;
            ping = pong;
            pong = tmp;
        }
    }

    memory_free(ping);
    memory_free(pong);
    return EMBEDDIP_OK;
}

/**
 * @brief Performs morphological opening on a binary image.
 *
 * @param[in]  inImg       Pointer to the input binary image.
 * @param[out] outImg      Pointer to the output image to store the result of opening.
 * @param[in]  kernel      Pointer to the structuring element (Kernel) used for the operation.
 * @param[in]  iterations  Number of erosion/dilation iterations.
 */
embeddip_status_t
opening(const Image *inImg, Image *outImg, const Kernel *kernel, uint8_t iterations)
{
    // Validate inputs
    if (!inImg || !outImg || !kernel)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (!inImg->pixels || !outImg->pixels || !kernel->data)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (inImg->format != IMAGE_FORMAT_GRAYSCALE || outImg->format != IMAGE_FORMAT_GRAYSCALE)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (inImg->width != outImg->width || inImg->height != outImg->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (iterations == 0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    // Create temporary image with same dimensions as input
    Image *temp = createImageWH_legacy(inImg->width, inImg->height, IMAGE_FORMAT_GRAYSCALE);
    if (!temp)
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    // Perform erosion then dilation
    embeddip_status_t status = erode(inImg, temp, kernel, iterations);
    if (status != EMBEDDIP_OK) {
        memory_free(temp->pixels);
        memory_free(temp);
        return status;
    }

    status = dilate(temp, outImg, kernel, iterations);

    // Free temporary image
    memory_free(temp->pixels);
    memory_free(temp);

    return status;
}

/**
 * @brief Performs morphological closing on a binary image.
 *
 * @param[in]  inImg       Pointer to the input binary image.
 * @param[out] outImg      Pointer to the output image to store the result of closing.
 * @param[in]  kernel      Pointer to the structuring element (Kernel) used for the operation.
 * @param[in]  iterations  Number of dilation/erosion iterations.
 */
embeddip_status_t
closing(const Image *inImg, Image *outImg, const Kernel *kernel, uint8_t iterations)
{
    // Validate inputs
    if (!inImg || !outImg || !kernel)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (!inImg->pixels || !outImg->pixels || !kernel->data)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (inImg->format != IMAGE_FORMAT_GRAYSCALE || outImg->format != IMAGE_FORMAT_GRAYSCALE)
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    if (inImg->width != outImg->width || inImg->height != outImg->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (iterations == 0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    // Create temporary image with same dimensions as input
    Image *temp = createImageWH_legacy(inImg->width, inImg->height, IMAGE_FORMAT_GRAYSCALE);
    if (!temp)
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    // Perform dilation then erosion
    embeddip_status_t status = dilate(inImg, temp, kernel, iterations);
    if (status != EMBEDDIP_OK) {
        memory_free(temp->pixels);
        memory_free(temp);
        return status;
    }

    status = erode(temp, outImg, kernel, iterations);

    // Free temporary image
    memory_free(temp->pixels);
    memory_free(temp);

    return status;
}

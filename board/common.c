// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "core/error.h"
#include "core/image.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <board/common.h>
#include <core/memory_manager.h>

embeddip_status_t createImage(ImageResolution resolution, ImageFormat format, Image **out_image)
{
    if (!out_image) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if ((int)resolution < 0 || (int)resolution >= IMAGE_RES_COUNT ||
        resolution == IMAGE_RES_INVALID || resolution == IMAGE_RES_CUSTOM) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    // Allocate Image structure
    Image *image = (Image *)memory_alloc(sizeof(Image));
    if (image == NULL) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // Determine the resolution (width and height) based on the provided size argument
    image->width = RES_WIDTH_LOOKUP[resolution];
    image->height = RES_HEIGHT_LOOKUP[resolution];
    image->size = (uint32_t)((size_t)image->width * (size_t)image->height);
    image->log = IMAGE_DATA_PIXELS;
    image->format = format;

    // Determine depth and bytes per pixel
    uint32_t bytes_per_pixel;
    switch (format) {
    case IMAGE_FORMAT_GRAYSCALE:
        image->depth = IMAGE_DEPTH_U8;
        bytes_per_pixel = IMAGE_DEPTH_U8;
        break;
    case IMAGE_FORMAT_MASK:
        image->depth = IMAGE_DEPTH_U8;
        bytes_per_pixel = IMAGE_DEPTH_U8;
        break;
    case IMAGE_FORMAT_RGB565:
        image->depth = IMAGE_DEPTH_U16;
        bytes_per_pixel = IMAGE_DEPTH_U16;
        break;
    case IMAGE_FORMAT_YUV:
        image->depth = IMAGE_DEPTH_U24;
        bytes_per_pixel = IMAGE_DEPTH_U24;
        break;
    case IMAGE_FORMAT_RGB888:
        image->depth = IMAGE_DEPTH_U24;
        bytes_per_pixel = IMAGE_DEPTH_U24;
        break;
    case IMAGE_FORMAT_HSI:
        image->depth = IMAGE_DEPTH_U24;
        bytes_per_pixel = IMAGE_DEPTH_U24;
        break;
    default:
        memory_free(image);
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    // Allocate pixel buffer
    image->pixels = (uint8_t *)memory_alloc(image->size * bytes_per_pixel);
    if (image->pixels == NULL) {
        // Rollback if allocation fails
        memory_free(image);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    image->is_chals = 0x00;
    image->chals = NULL;

    *out_image = image;
    return EMBEDDIP_OK;
}

embeddip_status_t createImageWH(int width, int height, ImageFormat format, Image **out_image)
{
    if (!out_image) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (width <= 0 || height <= 0) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    // Allocate Image structure
    Image *image = (Image *)memory_alloc(sizeof(Image));
    if (image == NULL) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    size_t pixel_count = (size_t)width * (size_t)height;
    if (pixel_count > UINT32_MAX) {
        memory_free(image);
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    // Determine the resolution (width and height) based on the provided size argument
    image->width = width;
    image->height = height;
    image->size = (uint32_t)pixel_count;
    image->format = format;
    image->log = IMAGE_DATA_PIXELS;

    // Determine depth and bytes per pixel
    uint32_t bytes_per_pixel;
    switch (format) {
    case IMAGE_FORMAT_GRAYSCALE:
        image->depth = IMAGE_DEPTH_U8;
        bytes_per_pixel = IMAGE_DEPTH_U8;
        break;
    case IMAGE_FORMAT_MASK:
        image->depth = IMAGE_DEPTH_U8;
        bytes_per_pixel = IMAGE_DEPTH_U8;
        break;
    case IMAGE_FORMAT_RGB565:
        image->depth = IMAGE_DEPTH_U16;
        bytes_per_pixel = IMAGE_DEPTH_U16;
        break;
    case IMAGE_FORMAT_YUV:
        image->depth = IMAGE_DEPTH_U24;
        bytes_per_pixel = IMAGE_DEPTH_U24;
        break;
    case IMAGE_FORMAT_RGB888:
        image->depth = IMAGE_DEPTH_U24;
        bytes_per_pixel = IMAGE_DEPTH_U24;
        break;
    case IMAGE_FORMAT_HSI:
        image->depth = IMAGE_DEPTH_U24;
        bytes_per_pixel = IMAGE_DEPTH_U24;
        break;
    default:
        memory_free(image);
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    // Allocate pixel buffer
    image->pixels = (uint8_t *)memory_alloc(image->size * bytes_per_pixel);
    if (image->pixels == NULL) {
        // Rollback if allocation fails
        memory_free(image);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    image->is_chals = 0x00;
    image->chals = NULL;

    *out_image = image;
    return EMBEDDIP_OK;
}

void deleteImage(Image *image)
{
    if (image == NULL) {
        return;  // Nothing to free
    }

    // Free pixel buffer if allocated
    if (image->pixels != NULL) {
        memory_free(image->pixels);
        image->pixels = NULL;
    }

    // Free channel buffers if present
    if (image->is_chals && image->chals != NULL) {
        for (uint8_t i = 0; i < 6; i++) {
            if (image->chals->ch[i] != NULL) {
                memory_free(image->chals->ch[i]);
                image->chals->ch[i] = NULL;
            }
        }
        memory_free(image->chals);
        image->chals = NULL;
    }

    // Finally, free the Image structure
    memory_free(image);
}

bool isChalsEmpty(const Image *inImg)
{
    if (inImg == NULL) {
        return true;  // Treat NULL input as "empty"
    }
    return (inImg->is_chals == 0);
}

embeddip_status_t createChals(Image *inImg, uint8_t numChals)
{
    if (inImg == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (numChals == 0 || numChals > 6) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    // RGB-like data uses ch[1], ch[2], ch[3], so requesting 3 channels
    // must allocate indices 0..3.
    uint8_t channels_to_alloc = (numChals == 3) ? 4 : numChals;
    bool created_chals_struct = false;
    uint8_t allocated_in_call[6] = {0};

    if (inImg->chals == NULL) {
        inImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        if (inImg->chals == NULL) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
        memset(inImg->chals, 0, sizeof(channels_t));
        created_chals_struct = true;
    }

    for (uint8_t i = 0; i < channels_to_alloc; i++) {
        if (inImg->chals->ch[i] == NULL) {
            // NOTE: Removed "* 2U" multiplier - was allocating double the required memory
            // Correct size: width × height × sizeof(float) for a single channel
            // For WQVGA (480×272): 522,240 bytes per channel (not 1,044,480!)
            size_t bufSize = (size_t)inImg->width * (size_t)inImg->height * sizeof(float);
            inImg->chals->ch[i] = (float *)memory_alloc(bufSize);
            if (inImg->chals->ch[i] == NULL) {
                // Roll back only channels allocated in this call.
                for (uint8_t j = 0; j < channels_to_alloc; j++) {
                    if (allocated_in_call[j] && inImg->chals->ch[j] != NULL) {
                        memory_free(inImg->chals->ch[j]);
                        inImg->chals->ch[j] = NULL;
                    }
                }
                if (created_chals_struct) {
                    memory_free(inImg->chals);
                    inImg->chals = NULL;
                }
                return EMBEDDIP_ERROR_OUT_OF_MEMORY;
            }
            allocated_in_call[i] = 1;
        }
    }

    inImg->is_chals = 1;
    return EMBEDDIP_OK;
}

/**
 * @brief Creates complex-valued channels for FFT operations.
 *
 * Allocates channels with 2× size to store interleaved complex data (real + imaginary).
 * Used by FFT operations that need width × height × 2 × sizeof(float) per channel.
 *
 * @param inImg     Target image.
 * @param numChals  Number of complex channels to create (1-3).
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t createChalsComplex(Image *inImg, uint8_t numChals)
{
    if (inImg == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (numChals == 0 || numChals > 6) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    bool created_chals_struct = false;
    uint8_t allocated_in_call[6] = {0};

    if (inImg->chals == NULL) {
        inImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        if (inImg->chals == NULL) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
        memset(inImg->chals, 0, sizeof(channels_t));
        created_chals_struct = true;
    }

    for (uint8_t i = 0; i < numChals; i++) {
        if (inImg->chals->ch[i] == NULL) {
            // Complex channel: width × height × 2 × sizeof(float)
            // The × 2 is for interleaved real and imaginary parts
            // For WQVGA (480×272): 1,044,480 bytes per complex channel
            size_t bufSize = (size_t)inImg->width * (size_t)inImg->height * 2U * sizeof(float);
            inImg->chals->ch[i] = (float *)memory_alloc(bufSize);
            if (inImg->chals->ch[i] == NULL) {
                // Roll back only channels allocated in this call.
                for (uint8_t j = 0; j < numChals; j++) {
                    if (allocated_in_call[j] && inImg->chals->ch[j] != NULL) {
                        memory_free(inImg->chals->ch[j]);
                        inImg->chals->ch[j] = NULL;
                    }
                }
                if (created_chals_struct) {
                    memory_free(inImg->chals);
                    inImg->chals = NULL;
                }
                return EMBEDDIP_ERROR_OUT_OF_MEMORY;
            }
            allocated_in_call[i] = 1;
        }
    }

    inImg->is_chals = 1;
    return EMBEDDIP_OK;
}

/* ============================================================================
 * Legacy/Deprecated Wrappers (for backward compatibility)
 * ========================================================================== */

Image *createImage_legacy(ImageResolution resolution, ImageFormat format)
{
    Image *img = NULL;
    embeddip_status_t status = createImage(resolution, format, &img);
    if (status != EMBEDDIP_OK) {
        return NULL;
    }
    return img;
}

Image *createImageWH_legacy(int width, int height, ImageFormat format)
{
    Image *img = NULL;
    embeddip_status_t status = createImageWH(width, height, format, &img);
    if (status != EMBEDDIP_OK) {
        return NULL;
    }
    return img;
}

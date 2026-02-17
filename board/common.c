#include "core/image.h"
#include "core/error.h"
#include <stdlib.h>
#include <string.h>
#include <board/common.h>
#include <core/memory_manager.h>

embeddip_status_t createImage(ImageResolution resolution, ImageFormat format, Image **out_image)
{
    if (!out_image) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    // Allocate Image structure
    Image *image = (Image *)memory_alloc(sizeof(Image));
    if (image == NULL) {
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // Determine the resolution (width and height) based on the provided size argument
    image->width = RES_WIDTH_LOOKUP[resolution];
    image->height = RES_HEIGHT_LOOKUP[resolution];
    image->size = image->width * image->height;
    image->log = IMAGE_DATA_PIXELS;
    image->format = format;

    // Determine depth and bytes per pixel
    uint32_t bytes_per_pixel;
    switch (format)
    {
    case IMAGE_FORMAT_GRAYSCALE:
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
    if (image->pixels == NULL)
    {
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

    // Determine the resolution (width and height) based on the provided size argument
    image->width = width;
    image->height = height;
    image->size = width * height;
    image->format = format;
    image->log = IMAGE_DATA_PIXELS;

    // Determine depth and bytes per pixel
    uint32_t bytes_per_pixel;
    switch (format)
    {
    case IMAGE_FORMAT_GRAYSCALE:
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
    if (image->pixels == NULL)
    {
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
    if (image == NULL)
    {
        return; // Nothing to free
    }

    // Free pixel buffer if allocated
    if (image->pixels != NULL)
    {
        memory_free(image->pixels);
        image->pixels = NULL;
    }

    // Free channel buffers if present
    if (image->is_chals && image->chals != NULL)
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            if (image->chals->ch[i] != NULL)
            {
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
    if (inImg == NULL)
    {
        return true; // Treat NULL input as "empty"
    }
    return (inImg->is_chals == 0);
}

embeddip_status_t createChals(Image *inImg, uint8_t numChals)
{
    if (inImg == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (numChals == 0 || numChals > 3) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    if (inImg->chals == NULL)
    {
        inImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        if (inImg->chals == NULL)
        {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
        memset(inImg->chals, 0, sizeof(channels_t));
    }

    for (uint8_t i = 0; i < numChals; i++)
    {
        if (inImg->chals->ch[i] == NULL)
        {
            size_t bufSize = (size_t)inImg->width * (size_t)inImg->height * sizeof(float) * 2U;
            inImg->chals->ch[i] = (float *)memory_alloc(bufSize);
            if (inImg->chals->ch[i] == NULL)
            {
                // Roll back allocations for already-created channels
                for (uint8_t j = 0; j < i; j++)
                {
                    if (inImg->chals->ch[j] != NULL)
                    {
                        memory_free(inImg->chals->ch[j]);
                        inImg->chals->ch[j] = NULL;
                    }
                }
                memory_free(inImg->chals);
                inImg->chals = NULL;
                return EMBEDDIP_ERROR_OUT_OF_MEMORY;
            }
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
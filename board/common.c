#include "core/image.h"
#include <stdlib.h>
#include <string.h>
#include <board/common.h>
#include <core/memory_manager.h>

Image *createImage(ImageResolution resolution, ImageFormat format)
{

    // Allocate Image structure
    Image *image = (Image *)memory_alloc(sizeof(Image));

    // Check if the image pointer is NULL, which would mean memory allocation failed
    if (image == NULL)
    {
        return NULL; // Failed to allocate memory
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
        return NULL;
        break;
    }

    // Allocate pixel buffer
    image->pixels = (uint8_t *)memory_alloc(image->size * bytes_per_pixel);
    if (image->pixels == NULL)
    {
        // Rollback if allocation fails
        memory_free(image);
        return NULL;
    }

    image->is_chals = 0x00;
    image->chals = NULL;

    return image;
}

Image *createImageWH(int width, int height, ImageFormat format)
{
    // Allocate Image structure
    Image *image = (Image *)memory_alloc(sizeof(Image));

    // Check if the image pointer is NULL, which would mean memory allocation failed
    if (image == NULL)
    {
        return NULL; // Failed to allocate memory
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
        return NULL;
        break;
    }

    // Allocate pixel buffer
    image->pixels = (uint8_t *)memory_alloc(image->size * bytes_per_pixel);
    if (image->pixels == NULL)
    {
        // Rollback if allocation fails
        memory_free(image);
        return NULL;
    }

    image->is_chals = 0x00;
    image->chals = NULL;

    return image;
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

bool createChals(Image *inImg, uint8_t numChals)
{
    if (inImg == NULL || numChals == 0 || numChals > 3)
    {
        return false; // Invalid arguments
    }

    if (inImg->chals == NULL)
    {
        inImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        if (inImg->chals == NULL)
        {
            return false; // Allocation failed
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
                return false;
            }
        }
    }

    inImg->is_chals = 1;
    return true;
}
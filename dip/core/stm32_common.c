#include "image.h"
#include <stdlib.h>
#include <string.h>
#include <common.h>
#include <memory_manager.h>

/**
 * @brief Creates an image with a specified size and format.
 *
 * @param size Size of the image.
 * @param format Format of the image (e.g., grayscale, RGB).
 * @return Pointer to the created Image.
 */
Image *createImage(ImageResolution resolution, ImageFormat format)
{

    // Declare a pointer to an Image structure and assign it to a memory address in SDRAM
    // SDRAM_BANK_ADDR and WRITE_READ_ADDR are predefined constants, likely pointing to the starting address of memory
    // allocatedSize keeps track of the current memory offset for storing new data
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
    // Set the image size and format based on the format argument

    image->format = format;

    // TODO -> need to adjust create image with alwyas 3 bytes per pixel
    // TODO -> need also delete image
    uint32_t bytes_per_pixel;
    switch (format)
    {
    case IMAGE_FORMAT_GRAYSCALE:
        image->depth = IMAGE_DEPTH_U8;
        bytes_per_pixel = BYTES_U8;
        break;
    case IMAGE_FORMAT_RGB565:
        image->depth = IMAGE_DEPTH_U16;
        bytes_per_pixel = BYTES_U16;
        break;
    default:
        image->depth = IMAGE_DEPTH_U24;
        bytes_per_pixel = BYTES_U24;
        break;
    }

    // Assign always 4 bytes per pixel. -> float usage.
    image->pixels = (uint8_t *)memory_alloc(image->height * image->width * BYTES_PER_PIXEL);
    image->is_chals = 0x00;
    image->chals = NULL;
    // Return the pointer to the newly created Image structure
    return image;
}

Image *createImageWH(int width, int height, ImageFormat format)
{
    Image *image = (Image *)memory_alloc(sizeof(Image));
    if (image == NULL)
        return NULL;

    image->width = width;
    image->height = height;
    image->size = width * height;
    image->format = format;

    uint32_t bytes_per_pixel;
    switch (format)
    {
    case IMAGE_FORMAT_GRAYSCALE:
        image->depth = IMAGE_DEPTH_U8;
        bytes_per_pixel = BYTES_U8;
        break;
    case IMAGE_FORMAT_RGB565:
        image->depth = IMAGE_DEPTH_U16;
        bytes_per_pixel = BYTES_U16;
        break;
    default:
        image->depth = IMAGE_DEPTH_U24;
        bytes_per_pixel = BYTES_U24;
        break;
    }

    image->pixels = (uint8_t *)memory_alloc(image->size * BYTES_PER_PIXEL); // always 4B (float)
    image->is_chals = 0;
    image->chals = NULL;

    return image;
}

void deleteImage(Image *image)
{
    if (!image)
        return;

    if (image->pixels)
    {
        memory_free(image->pixels);
    }

    if (image->is_chals && image->chals)
    {
        for (int i = 0; i < 3; i++)
        {
            if (image->chals->ch[i])
            {
                memory_free(image->chals->ch[i]);
            }
        }
        memory_free(image->chals);
    }

    memory_free(image);
}

bool isChalsEmpty(const Image *inImg)
{
    return !(inImg->is_chals);
}
void createChals(Image *inImg, uint8_t numChals)
{
    if (inImg->chals == NULL)
    {
        inImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
    }
    for (int i = 0; i < numChals; i++)
    {
        inImg->chals->ch[i] = (float *)memory_alloc(inImg->height * inImg->width * 4);
    }
}

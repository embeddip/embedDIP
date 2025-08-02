#include "core/image.h"
#include <stdlib.h>
#include <string.h>
#include <board/common.h>
#include <core/memory_manager.h>

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
    Image *image = (Image *)malloc(sizeof(Image));

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
    image->pixels = (uint8_t *)malloc(image->height * image->width * BYTES_PER_PIXEL);
    image->is_chals = 0x00;
    image->chals = NULL;
    // Return the pointer to the newly created Image structure
    return image;
}

/**
 * @brief Creates an Image object with given width, height, and format.
 *
 * This function dynamically allocates memory for a new `Image` structure and
 * its associated pixel buffer. The pixel buffer size depends on the selected
 * image format (Grayscale, RGB565, or RGB888).
 *
 * - Grayscale: 1 byte per pixel (U8)
 * - RGB565:    2 bytes per pixel (U16)
 * - RGB888:    3 bytes per pixel (U24)
 * - All data is internally allocated using `malloc()`.
 *
 * @param width     Width of the image in pixels.
 * @param height    Height of the image in pixels.
 * @param format    Desired image format (e.g., IMAGE_FORMAT_GRAYSCALE, IMAGE_FORMAT_RGB565, IMAGE_FORMAT_RGB888).
 *
 * @return Pointer to the allocated Image object, or NULL if memory allocation fails.
 *
 * @note The caller is responsible for freeing the image using `freeImage()` or an equivalent function.
 */
Image *createImageWH(int width, int height, ImageFormat format)
{
    Image *image = (Image *)malloc(sizeof(Image));
    if (image == NULL)
        return NULL;

    image->width = width;
    image->height = height;
    image->size = width * height;
    image->format = format;
    image->log = IMAGE_DATA_PIXELS;

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

    image->pixels = (uint8_t *)malloc(image->size * BYTES_PER_PIXEL); // always 4B (float)
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
        inImg->chals = (channels_t *)malloc(sizeof(channels_t));
    }
    for (int i = 0; i < numChals; i++)
    {
        inImg->chals->ch[i] = (float *)malloc(inImg->height * inImg->width * sizeof(float) * 2);
    }
}

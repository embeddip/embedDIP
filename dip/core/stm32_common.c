#include "image.h"
#include <stdlib.h>
#include <string.h>
#include <common.h>
#include <memory_manager.h>

uint32_t allocatedSize = 0;

/**
 * @brief Creates an image with a specified size and format.
 *
 * @param size Size of the image.
 * @param format Format of the image (e.g., grayscale, RGB).
 * @return Pointer to the created Image.
 */
Image *createImage(image_resolution_t resolution, ImageFormat format)
{

    // Declare a pointer to an Image structure and assign it to a memory address in SDRAM
    // SDRAM_BANK_ADDR and WRITE_READ_ADDR are predefined constants, likely pointing to the starting address of memory
    // allocatedSize keeps track of the current memory offset for storing new data
    Image *image = (Image *)memory_alloc(sizeof(Image));

    // Update the allocated size to account for the new Image structure
    allocatedSize += sizeof(Image);

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
    image->chals->ch[0] = NULL;
    image->chals->ch[1] = NULL;
    image->chals->ch[2] = NULL;
    image->chals->ch[3] = NULL;
    // Return the pointer to the newly created Image structure
    return image;
}

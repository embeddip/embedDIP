#include "image.h"
#include <stdlib.h>
#include <string.h>

uint32_t allocatedSize = 0;

/**
 * @brief Creates an image with a specified size and format.
 *
 * @param size Size of the image.
 * @param format Format of the image (e.g., grayscale, RGB).
 * @return Pointer to the created Image.
 */
Image *createImage(uint8_t size, uint8_t format)
{

    // Declare a pointer to an Image structure and assign it to a memory address in SDRAM
    // SDRAM_BANK_ADDR and WRITE_READ_ADDR are predefined constants, likely pointing to the starting address of memory
    // allocatedSize keeps track of the current memory offset for storing new data
    Image *image = SDRAM_BANK_ADDR + 0 + allocatedSize;

    // Update the allocated size to account for the new Image structure
    allocatedSize += sizeof(Image);

    // Check if the image pointer is NULL, which would mean memory allocation failed
    if (image == NULL)
    {
        return NULL; // Failed to allocate memory
    }

    // Determine the resolution (width and height) based on the provided size argument
    switch (size)
    {
    case IMAGE_RES_QQVGA:
    {
        image->width = IMAGE_RES_QQVGA_Width;   // Set QQVGA width (160 pixels_u8)
        image->height = IMAGE_RES_QQVGA_Height; // Set QQVGA height (120 pixels_u8)
        break;
    }
    case IMAGE_RES_QVGA:
    {
        image->width = IMAGE_RES_QVGA_Width;   // Set QVGA width (320 pixels_u8)
        image->height = IMAGE_RES_QVGA_Height; // Set QVGA height (240 pixels_u8)
        break;
    }
    case IMAGE_RES_WQVGA:
    {
        image->width = IMAGE_RES_WQVGA_Width;   // Set custom resolution width (480 pixels_u8)
        image->height = IMAGE_RES_WQVGA_Height; // Set custom resolution height (272 pixels_u8)
        break;
    }
    case IMAGE_RES_VGA:
    {
        image->width = IMAGE_RES_VGA_Width;   // Set VGA width (640 pixels_u8)
        image->height = IMAGE_RES_VGA_Height; // Set VGA height (480 pixels_u8)
        break;
    }
    default:
    {
        // Invalid resolution, return NULL to indicate an error
        return NULL;
    }
    }

    printf("%d \n", image->width);
    // Set the image size and format based on the format argument
    switch (format)
    {
    case IMAGE_FORMAT_RGB565:
    {
        image->size = image->width * image->height * 2; // 2 bytes per pixel for RGB565 format
        image->format = IMAGE_FORMAT_RGB565;            // Assign the RGB565 format constant
        break;
    }
    case IMAGE_FORMAT_GRAYSCALE:
    {
        image->size = image->width * image->height; // 1 byte per pixel for grayscale
        image->format = IMAGE_FORMAT_GRAYSCALE;     // Assign the Grayscale format constant
        break;
    }
    default:
    {
        // Invalid format, return NULL to indicate an error
        return NULL;
    }
    }

    // Assign the pointer to the pixel data, allocating memory for it after the Image structure
    image->pixels_u8 = SDRAM_BANK_ADDR + 0 + allocatedSize;

    // Update allocatedSize to account for the memory needed to store the image pixel data
    allocatedSize += image->size;

    // Return the pointer to the newly created Image structure
    return image;
}

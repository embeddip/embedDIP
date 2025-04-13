#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "assert.h"
#define MAX_INTENSITY 255

/**
 * @brief Image resolution definitions.
 */
typedef enum
{
    IMAGE_RES_QQVGA = 0, // 160x120
    IMAGE_RES_QVGA = 1,  // 320x240
    IMAGE_RES_WQVGA = 2, // 480x272
    IMAGE_RES_VGA = 3    // 640x480
} image_resolution_t;

/**
 * @brief Image dimensions for various resolutions.
 */
#define IMAGE_RES_VGA_Width 640
#define IMAGE_RES_VGA_Height 480
#define IMAGE_RES_QVGA_Width 320
#define IMAGE_RES_QVGA_Height 240
#define IMAGE_RES_QQVGA_Width 160
#define IMAGE_RES_QQVGA_Height 120
#define IMAGE_RES_WQVGA_Width 480
#define IMAGE_RES_WQVGA_Height 272

static const uint16_t RES_WIDTH_LOOKUP[] = {
    IMAGE_RES_QQVGA_Width, // QQVGA
    IMAGE_RES_QVGA_Width,  // QVGA
    IMAGE_RES_WQVGA_Width, // WQVGA
    IMAGE_RES_VGA_Width    // VGA
};

static const uint16_t RES_HEIGHT_LOOKUP[] = {
    IMAGE_RES_QQVGA_Height, // QQVGA
    IMAGE_RES_QVGA_Height,  // QVGA
    IMAGE_RES_WQVGA_Height, // WQVGA
    IMAGE_RES_VGA_Height    // VGA
};

// TODO TAIŞANACAK

/**
 * @brief SDRAM base address.
 */

/**
 * @brief Base address for read/write operations.
 */
#define WRITE_READ_ADDR ((uint32_t)0x100000)

typedef enum
{
    IMAGE_FORMAT_GRAYSCALE = 0, // 1 channel
    IMAGE_FORMAT_RGB888,        // 3 channels, unpacked
    IMAGE_FORMAT_RGB565,        // packed format, 2 bytes
    IMAGE_FORMAT_YUV,           // 3 channels
    IMAGE_FORMAT_HSV            // 3 channels (typically float)
} ImageFormat;

typedef enum
{
    IMAGE_DEPTH_U8 = 1,
    IMAGE_DEPTH_U16 = 2,
    IMAGE_DEPTH_U24 = 3,
    IMAGE_DEPTH_F32 = 4,
} ImageDepth;

typedef struct
{
    union
    {
        float *ch[4]; // Access via index: ch[0] = r, ch[1] = g, etc.

        struct
        {
            float *l; // ch[0] (used for grayscale or luminance)
            float *r; // ch[1]
            float *g; // ch[2]
            float *b; // ch[3]
        };
    };
} channels_t;

/**
 *
 * @brief Structure to represent an image with both raw and high-precision data.
 */
typedef struct
{
    uint32_t width;
    uint32_t height;
    void *pixels;
    channels_t *chals; // single pointer for pixel data
    uint8_t is_chals;
    uint32_t size;      // number of elements = width * height * channels
    ImageFormat format; // GRAYSCALE or RGB
    ImageDepth depth;   // U8 or F32

    float *fourier_x;
    float *fourier_y;
} Image;

void convertTo(Image *inImg); //TODO

#endif // IMAGE_H

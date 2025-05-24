#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "assert.h"

/**
 * @def MAX_INTENSITY
 * @brief Maximum intensity value for 8-bit image formats.
 */
#define MAX_INTENSITY 255

/**
 * @enum image_resolution_t
 * @brief Predefined image resolutions with associated indices.
 */
typedef enum
{
    IMAGE_RES_QQVGA = 0, /**< 160x120 resolution */
    IMAGE_RES_QVGA = 1,  /**< 320x240 resolution */
    IMAGE_RES_WQVGA = 2, /**< 480x272 resolution */
    IMAGE_RES_VGA = 3    /**< 640x480 resolution */
} ImageResolution;

/** @name Resolution dimensions
 *  @brief Image width and height definitions for each resolution.
 *  @{
 */
#define IMAGE_RES_QQVGA_Width 160
#define IMAGE_RES_QQVGA_Height 120
#define IMAGE_RES_QVGA_Width 320
#define IMAGE_RES_QVGA_Height 240
#define IMAGE_RES_WQVGA_Width 480
#define IMAGE_RES_WQVGA_Height 272
#define IMAGE_RES_VGA_Width 640
#define IMAGE_RES_VGA_Height 480
/** @} */

/**
 * @brief Lookup table for image widths by resolution index.
 */
static const uint16_t RES_WIDTH_LOOKUP[] = {
    IMAGE_RES_QQVGA_Width, /**< QQVGA width */
    IMAGE_RES_QVGA_Width,  /**< QVGA width */
    IMAGE_RES_WQVGA_Width, /**< WQVGA width */
    IMAGE_RES_VGA_Width    /**< VGA width */
};

/**
 * @brief Lookup table for image heights by resolution index.
 */
static const uint16_t RES_HEIGHT_LOOKUP[] = {
    IMAGE_RES_QQVGA_Height, /**< QQVGA height */
    IMAGE_RES_QVGA_Height,  /**< QVGA height */
    IMAGE_RES_WQVGA_Height, /**< WQVGA height */
    IMAGE_RES_VGA_Height    /**< VGA height */
};

/**
 * @def WRITE_READ_ADDR
 * @brief SDRAM base address for read/write operations.
 */
#define WRITE_READ_ADDR ((uint32_t)0x100000)

/**
 * @enum ImageFormat
 * @brief Enum representing different image color formats.
 */
typedef enum
{
    IMAGE_FORMAT_GRAYSCALE = 0, /**< 1 channel grayscale */
    IMAGE_FORMAT_RGB888,        /**< RGB format, 3 channels, 8-bit each */
    IMAGE_FORMAT_RGB565,        /**< Packed RGB format, 16-bit */
    IMAGE_FORMAT_YUV,           /**< YUV format, 3 channels */
    IMAGE_FORMAT_HSV            /**< HSV format, usually stored as float */
} ImageFormat;

/**
 * @enum ImageDepth
 * @brief Enum representing the depth (bit-precision) of image data.
 */
typedef enum
{
    IMAGE_DEPTH_U8 = 1,  /**< Unsigned 8-bit depth */
    IMAGE_DEPTH_U16 = 2, /**< Unsigned 16-bit depth */
    IMAGE_DEPTH_U24 = 3, /**< Unsigned 24-bit depth (packed RGB888) */
    IMAGE_DEPTH_F32 = 4  /**< 32-bit floating-point depth */
} ImageDepth;

/**
 * @struct channels_t
 * @brief Structure to hold floating-point representations of image channels.
 *
 * Allows flexible access to color components or single-channel images.
 */
typedef struct
{
    union
    {
        float *ch[6]; /**< Array-style access: ch[0] = l/r, ch[1] = g, etc. */

        struct
        {
            float *l;         /**< Luminance or grayscale channel (same as ch[0]) */
            float *r;         /**< Red channel (same as ch[1]) */
            float *g;         /**< Green channel (same as ch[2]) */
            float *b;         /**< Blue channel (same as ch[3]) */
            float *fourier_x; /**< Optional horizontal FFT data */
            float *fourier_y; /**< Optional vertical FFT data */
        };
    };
} channels_t;

/**
 * @struct Image
 * @brief Represents an image with both raw pixel data and channel-wise float representation.
 *
 * Includes resolution, format, depth, and optional Fourier components.
 */
typedef struct
{
    uint32_t width;     /**< Image width in pixels */
    uint32_t height;    /**< Image height in pixels */
    void *pixels;       /**< Raw pixel data */
    channels_t *chals;  /**< Optional high-precision float channels */
    bool is_chals;      /**< Flag indicating if chals is valid */
    uint32_t size;      /**< Total number of elements = width * height * channels */
    ImageFormat format; /**< Color format of the image */
    ImageDepth depth;   /**< Pixel depth (bit precision or float) */

    float *fourier_x; /**< Optional horizontal FFT data */
    float *fourier_y; /**< Optional vertical FFT data */
} Image;

void resize(Image *inImg, Image *outImg, int size);

void dist(const Image *inImg, Image *outImg, uint8_t R_ref, uint8_t G_ref, uint8_t B_ref);

void add(const Image *img1, const Image *img2, Image *outImg);

void normalize(Image *inImg);

/**
 * @brief Converts raw pixel data to high-precision floating-point channels.
 *
 * This function allocates and fills `chals` from raw `pixels` depending on format and depth.
 *
 * @param inImg Pointer to the image whose pixels will be converted.
 */
void convertTo(Image *inImg); // TODO: Implement

#endif // IMAGE_H

#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

#define MAX_INTENSITY 255

/**
 * @brief Image resolution definitions.
 */
#define IMAGE_RES_QQVGA 0
#define IMAGE_RES_QVGA 1
#define IMAGE_RES_WQVGA 2
#define IMAGE_RES_VGA 3

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

// TODO TAIŞANACAK

/**
 * @brief SDRAM base address.
 */
#define SDRAM_BANK_ADDR ((uint32_t)0xC0000000)

/**
 * @brief Base address for read/write operations.
 */
#define WRITE_READ_ADDR ((uint32_t)0x100000)

typedef enum
{
    IMAGE_FORMAT_RGB565 = 1,
    IMAGE_FORMAT_YUV = 2,
    IMAGE_FORMAT_GRAYSCALE = 3,
    IMAGE_FORMAT_FLOAT32 = 100 // For floating-point image representation
} ImageFormat;

/**
 * @brief Structure to represent an image with both raw and high-precision data.
 */
typedef struct
{
    uint32_t width;     ///< Image width in pixels
    uint32_t height;    ///< Image height in pixels
    uint8_t *pixels_u8; ///< Raw pixel data (8-bit per channel)
    float *pixels_f32;  ///< High-precision floating-point pixel data
    uint32_t size;      ///< Total size in bytes (depends on format & channels)
    ImageFormat format; ///< Image format (GRAYSCALE, RGB, FLOAT32, etc.)

    float *fourier_x; ///< Fourier transform result (X axis)
    float *fourier_y; ///< Fourier transform result (Y axis)

} Image;

#endif // IMAGE_H

#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "assert.h"
#include <string.h>
#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @def MAX_INTENSITY
 * @brief Maximum intensity value for 8-bit image formats.
 */
#define MAX_INTENSITY 255

    /**
     * @enum image_resolution_t
     * @brief Predefined image resolutions with associated indices.
     */
    /**
     * @enum ImageResolution
     * @brief Predefined image resolutions with associated indices.
     */
    typedef enum
    {
        IMAGE_RES_96X96 = 0, /**< 96x96 resolution */
        IMAGE_RES_QQVGA,     /**< 160x120 resolution */
        IMAGE_RES_QCIF,      /**< 176x144 resolution */
        IMAGE_RES_HQVGA,     /**< 240x176 resolution */
        IMAGE_RES_240X240,   /**< 240x240 resolution */
        IMAGE_RES_QVGA,      /**< 320x240 resolution */
        IMAGE_RES_CIF,       /**< 352x288 resolution */
        IMAGE_RES_HVGA,      /**< 480x320 resolution */
        IMAGE_RES_VGA,       /**< 640x480 resolution */
        IMAGE_RES_SVGA,      /**< 800x600 resolution */
        IMAGE_RES_XGA,       /**< 1024x768 resolution */
        IMAGE_RES_HD,        /**< 1280x720 resolution */
        IMAGE_RES_SXGA,      /**< 1280x1024 resolution */
        IMAGE_RES_UXGA,      /**< 1600x1200 resolution */
        IMAGE_RES_FHD,       /**< 1920x1080 resolution */
        IMAGE_RES_P_HD,      /**< 720x1280 resolution (portrait HD) */
        IMAGE_RES_P_3MP,     /**< 864x1536 resolution (portrait 3MP) */
        IMAGE_RES_QXGA,      /**< 2048x1536 resolution */
        IMAGE_RES_QHD,       /**< 2560x1440 resolution */
        IMAGE_RES_WQXGA,     /**< 2560x1600 resolution */
        IMAGE_RES_P_FHD,     /**< 1080x1920 resolution (portrait FHD) */
        IMAGE_RES_QSXGA,     /**< 2560x1920 resolution */
        IMAGE_RES_INVALID,
        IMAGE_RES_CUSTOM, /**< User-defined resolution */
        IMAGE_RES_WQVGA   /**< 480x272 resolution (custom addition) */
    } ImageResolution;

/** @name Resolution dimensions
 *  @brief Image width and height definitions for each resolution.
 *  @{
 */
#define IMAGE_RES_96X96_Width 96
#define IMAGE_RES_96X96_Height 96
#define IMAGE_RES_QQVGA_Width 160
#define IMAGE_RES_QQVGA_Height 120
#define IMAGE_RES_QCIF_Width 176
#define IMAGE_RES_QCIF_Height 144
#define IMAGE_RES_HQVGA_Width 240
#define IMAGE_RES_HQVGA_Height 176
#define IMAGE_RES_240X240_Width 240
#define IMAGE_RES_240X240_Height 240
#define IMAGE_RES_QVGA_Width 320
#define IMAGE_RES_QVGA_Height 240
#define IMAGE_RES_CIF_Width 352
#define IMAGE_RES_CIF_Height 288
#define IMAGE_RES_HVGA_Width 480
#define IMAGE_RES_HVGA_Height 320
#define IMAGE_RES_WQVGA_Width 480
#define IMAGE_RES_WQVGA_Height 272
#define IMAGE_RES_VGA_Width 640
#define IMAGE_RES_VGA_Height 480
#define IMAGE_RES_SVGA_Width 800
#define IMAGE_RES_SVGA_Height 600
#define IMAGE_RES_XGA_Width 1024
#define IMAGE_RES_XGA_Height 768
#define IMAGE_RES_HD_Width 1280
#define IMAGE_RES_HD_Height 720
#define IMAGE_RES_SXGA_Width 1280
#define IMAGE_RES_SXGA_Height 1024
#define IMAGE_RES_UXGA_Width 1600
#define IMAGE_RES_UXGA_Height 1200
#define IMAGE_RES_FHD_Width 1920
#define IMAGE_RES_FHD_Height 1080
#define IMAGE_RES_P_HD_Width 720
#define IMAGE_RES_P_HD_Height 1280
#define IMAGE_RES_P_3MP_Width 864
#define IMAGE_RES_P_3MP_Height 1536
#define IMAGE_RES_QXGA_Width 2048
#define IMAGE_RES_QXGA_Height 1536
#define IMAGE_RES_QHD_Width 2560
#define IMAGE_RES_QHD_Height 1440
#define IMAGE_RES_WQXGA_Width 2560
#define IMAGE_RES_WQXGA_Height 1600
#define IMAGE_RES_P_FHD_Width 1080
#define IMAGE_RES_P_FHD_Height 1920
#define IMAGE_RES_QSXGA_Width 2560
#define IMAGE_RES_QSXGA_Height 1920

#define IMAGE_RES_INVALID_Width 0xFF
#define IMAGE_RES_INVALID_Height 0xFF

#define IMAGE_RES_CUSTOM_Width 0xEF
#define IMAGE_RES_CUSTOM_Height 0xEF

    /** @} */

    /**
     * @brief Lookup table for image widths by resolution index.
     */
    static const uint16_t RES_WIDTH_LOOKUP[] = {
        IMAGE_RES_96X96_Width,
        IMAGE_RES_QQVGA_Width,
        IMAGE_RES_QCIF_Width,
        IMAGE_RES_HQVGA_Width,
        IMAGE_RES_240X240_Width,
        IMAGE_RES_QVGA_Width,
        IMAGE_RES_CIF_Width,
        IMAGE_RES_HVGA_Width,
        IMAGE_RES_VGA_Width,
        IMAGE_RES_SVGA_Width,
        IMAGE_RES_XGA_Width,
        IMAGE_RES_HD_Width,
        IMAGE_RES_SXGA_Width,
        IMAGE_RES_UXGA_Width,
        IMAGE_RES_FHD_Width,
        IMAGE_RES_P_HD_Width,
        IMAGE_RES_P_3MP_Width,
        IMAGE_RES_QXGA_Width,
        IMAGE_RES_QHD_Width,
        IMAGE_RES_WQXGA_Width,
        IMAGE_RES_P_FHD_Width,
        IMAGE_RES_QSXGA_Width,
        IMAGE_RES_INVALID_Width,
        IMAGE_RES_CUSTOM_Width,
        IMAGE_RES_WQVGA_Width,
    };

    static const uint16_t RES_HEIGHT_LOOKUP[] = {
        IMAGE_RES_96X96_Height,
        IMAGE_RES_QQVGA_Height,
        IMAGE_RES_QCIF_Height,
        IMAGE_RES_HQVGA_Height,
        IMAGE_RES_240X240_Height,
        IMAGE_RES_QVGA_Height,
        IMAGE_RES_CIF_Height,
        IMAGE_RES_HVGA_Height,
        IMAGE_RES_VGA_Height,
        IMAGE_RES_SVGA_Height,
        IMAGE_RES_XGA_Height,
        IMAGE_RES_HD_Height,
        IMAGE_RES_SXGA_Height,
        IMAGE_RES_UXGA_Height,
        IMAGE_RES_FHD_Height,
        IMAGE_RES_P_HD_Height,
        IMAGE_RES_P_3MP_Height,
        IMAGE_RES_QXGA_Height,
        IMAGE_RES_QHD_Height,
        IMAGE_RES_WQXGA_Height,
        IMAGE_RES_P_FHD_Height,
        IMAGE_RES_QSXGA_Height,
        IMAGE_RES_INVALID_Height,
        IMAGE_RES_CUSTOM_Height,
        IMAGE_RES_WQVGA_Height};

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
        IMAGE_FORMAT_HSI            /**< HSI format, usually stored as float */
    } ImageFormat;

    typedef struct
    {
        int x;
        int y;
        int width;
        int height;
    } Rect;

    typedef struct
    {
        float mean[3]; // RGB
        float var[3];  // RGB diagonal covariance
        int count;
    } GMMStats;

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

    typedef enum
    {
        IMAGE_DATA_INVALID = 0, /**< No valid data yet */
        IMAGE_DATA_PIXELS,      /**< Raw pixel data in 'pixels' is the most recent */
        IMAGE_DATA_CH0,         /**< chals->ch[0] is the most recent (e.g., grayscale or real) */
        IMAGE_DATA_CH1,         /**< chals->ch[1] is the most recent (e.g., FFT transposed) */
        IMAGE_DATA_CH2,         /**< chals->ch[2] is the most recent (e.g., green channel) */
        IMAGE_DATA_CH3,         /**< chals->ch[3] is the most recent (e.g., blue channel) */
        IMAGE_DATA_CH4,         /**< chals->ch[4] is the most recent (optional use) */
        IMAGE_DATA_CH5,         /**< chals->ch[5] is the most recent (optional use) */
        IMAGE_DATA_COMPLEX,     /**< Complex data stored across ch[0] and ch[1] */
        IMAGE_DATA_MAGNITUDE,   /**< Magnitude spectrum stored in ch[0] */
        IMAGE_DATA_PHASE        /**< Phase spectrum stored in ch[0] */
    } ImageDataState;

    /**
     * @struct channels_t
     * @brief Structure to hold floating-point representations of image channels.
     *
     * Allows flexible access to color components or single-channel images.
     */
    typedef struct
    {

#ifdef __cplusplus

        float *ch[6]; /**< Array-style access: ch[0] = l/r, ch[1] = g, etc. */

        float *&l() { return ch[0]; }  /**< Luminance or grayscale channel (same as ch[0]) */
        float *&r() { return ch[1]; }  /**< Red channel (same as ch[1]) */
        float *&g() { return ch[2]; }  /**< Green channel (same as ch[2]) */
        float *&b() { return ch[3]; }  /**< Blue channel (same as ch[3]) */
        float *&fx() { return ch[4]; } /**< Optional horizontal FFT data */
        float *&fy() { return ch[5]; } /**< Optional vertical FFT data */
#else
    union
    {
        float *ch[6]; /**< Array-style access: ch[0] = l/r, ch[1] = g, etc. */
        struct test
        {
            float *l;  /**< Luminance or grayscale channel (same as ch[0]) */
            float *r;  /**< Red channel (same as ch[1]) */
            float *g;  /**< Green channel (same as ch[2]) */
            float *b;  /**< Blue channel (same as ch[3]) */
            float *fx; /**< Optional horizontal FFT data */
            float *fy; /**< Optional vertical FFT data */
        } test;
    };
#endif

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
        ImageDataState log; /**< Last valid or most recently updated image data */
    } Image;

#ifdef __cplusplus
}
#endif

#endif // IMAGE_H

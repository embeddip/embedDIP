/* ========================================================================== */
/*  File: image.h                                                             */
/*  Brief: Image data structures, resolutions, formats, and channel handling  */
/*  SPDX-License-Identifier: BSD-3-Clause                                     */
/* ========================================================================== */
#ifndef IMAGE_H
#define IMAGE_H

/**
 * @file image.h
 * @brief Core image type definitions for the EmbedDIP library.
 *
 * This header defines:
 * - Predefined resolutions and lookup tables
 * - Image formats and pixel depths
 * - Structures for rectangular regions, GMM statistics, and channel storage
 * - The main ::Image container
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/**
 * @defgroup embedDIP_image Image Structures & Formats
 * @ingroup embedDIP_c_api
 * @brief Data structures and constants for image representation in EmbedDIP.
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Maximum intensity value for 8-bit formats. */
#ifndef UINT8_MAX
#define UINT8_MAX 255
#endif

    /* -------------------------------------------------------------------------- */
    /* Resolution enumeration                                                     */
    /* -------------------------------------------------------------------------- */
    /**
     * @enum ImageResolution
     * @brief Predefined image resolutions with associated indices.
     */
    typedef enum
    {
        IMAGE_RES_96X96 = 0, /**< 96×96 */
        IMAGE_RES_QQVGA,     /**< 160×120 */
        IMAGE_RES_QCIF,      /**< 176×144 */
        IMAGE_RES_HQVGA,     /**< 240×176 */
        IMAGE_RES_240X240,   /**< 240×240 */
        IMAGE_RES_QVGA,      /**< 320×240 */
        IMAGE_RES_CIF,       /**< 352×288 */
        IMAGE_RES_HVGA,      /**< 480×320 */
        IMAGE_RES_VGA,       /**< 640×480 */
        IMAGE_RES_SVGA,      /**< 800×600 */
        IMAGE_RES_XGA,       /**< 1024×768 */
        IMAGE_RES_HD,        /**< 1280×720 */
        IMAGE_RES_SXGA,      /**< 1280×1024 */
        IMAGE_RES_UXGA,      /**< 1600×1200 */
        IMAGE_RES_FHD,       /**< 1920×1080 */
        IMAGE_RES_P_HD,      /**< 720×1280 (portrait HD) */
        IMAGE_RES_P_3MP,     /**< 864×1536 (portrait 3MP) */
        IMAGE_RES_QXGA,      /**< 2048×1536 */
        IMAGE_RES_QHD,       /**< 2560×1440 */
        IMAGE_RES_WQXGA,     /**< 2560×1600 */
        IMAGE_RES_P_FHD,     /**< 1080×1920 (portrait FHD) */
        IMAGE_RES_QSXGA,     /**< 2560×1920 */
        IMAGE_RES_INVALID,   /**< Invalid resolution */
        IMAGE_RES_CUSTOM,    /**< User-defined dimensions */
        IMAGE_RES_WQVGA      /**< 480×272 */
    } ImageResolution;

/* -------------------------------------------------------------------------- */
/* Resolution dimension macros                                                */
/* -------------------------------------------------------------------------- */
/** @name Resolution dimensions
 *  @brief Width/height macros for each ::ImageResolution.
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

    /* -------------------------------------------------------------------------- */
    /* Lookup tables                                                              */
    /* -------------------------------------------------------------------------- */
    /** @brief Lookup table for image widths by resolution index. */
    static const uint16_t RES_WIDTH_LOOKUP[] = {
        IMAGE_RES_96X96_Width, IMAGE_RES_QQVGA_Width, IMAGE_RES_QCIF_Width, IMAGE_RES_HQVGA_Width,
        IMAGE_RES_240X240_Width, IMAGE_RES_QVGA_Width, IMAGE_RES_CIF_Width, IMAGE_RES_HVGA_Width,
        IMAGE_RES_VGA_Width, IMAGE_RES_SVGA_Width, IMAGE_RES_XGA_Width, IMAGE_RES_HD_Width,
        IMAGE_RES_SXGA_Width, IMAGE_RES_UXGA_Width, IMAGE_RES_FHD_Width, IMAGE_RES_P_HD_Width,
        IMAGE_RES_P_3MP_Width, IMAGE_RES_QXGA_Width, IMAGE_RES_QHD_Width, IMAGE_RES_WQXGA_Width,
        IMAGE_RES_P_FHD_Width, IMAGE_RES_QSXGA_Width, IMAGE_RES_INVALID_Width,
        IMAGE_RES_CUSTOM_Width, IMAGE_RES_WQVGA_Width};

    /** @brief Lookup table for image heights by resolution index. */
    static const uint16_t RES_HEIGHT_LOOKUP[] = {
        IMAGE_RES_96X96_Height, IMAGE_RES_QQVGA_Height, IMAGE_RES_QCIF_Height, IMAGE_RES_HQVGA_Height,
        IMAGE_RES_240X240_Height, IMAGE_RES_QVGA_Height, IMAGE_RES_CIF_Height, IMAGE_RES_HVGA_Height,
        IMAGE_RES_VGA_Height, IMAGE_RES_SVGA_Height, IMAGE_RES_XGA_Height, IMAGE_RES_HD_Height,
        IMAGE_RES_SXGA_Height, IMAGE_RES_UXGA_Height, IMAGE_RES_FHD_Height, IMAGE_RES_P_HD_Height,
        IMAGE_RES_P_3MP_Height, IMAGE_RES_QXGA_Height, IMAGE_RES_QHD_Height, IMAGE_RES_WQXGA_Height,
        IMAGE_RES_P_FHD_Height, IMAGE_RES_QSXGA_Height, IMAGE_RES_INVALID_Height,
        IMAGE_RES_CUSTOM_Height, IMAGE_RES_WQVGA_Height};

    /* -------------------------------------------------------------------------- */
    /* Image format, depth, and metadata                                          */
    /* -------------------------------------------------------------------------- */
    /**
     * @enum ImageFormat
     * @brief Supported color formats for images.
     */
    typedef enum
    {
        IMAGE_FORMAT_GRAYSCALE = 0, /**< 1 channel grayscale */
        IMAGE_FORMAT_RGB888,        /**< 3×8-bit RGB */
        IMAGE_FORMAT_RGB565,        /**< Packed 16-bit RGB */
        IMAGE_FORMAT_YUV,           /**< YUV (3 channels) */
        IMAGE_FORMAT_HSI            /**< HSI color space, usually float */
    } ImageFormat;

    /**
     * @enum ImageDepth
     * @brief Bit-depth or precision of pixel data.
     */
    typedef enum
    {
        IMAGE_DEPTH_U8 = 1,  /**< Unsigned 8-bit */
        IMAGE_DEPTH_U16 = 2, /**< Unsigned 16-bit */
        IMAGE_DEPTH_U24 = 3, /**< Packed RGB888 */
        IMAGE_DEPTH_F32 = 4  /**< 32-bit float */
    } ImageDepth;

    /**
     * @enum ImageDataState
     * @brief Indicator of the most recent or valid data location within ::Image.
     */
    typedef enum
    {
        IMAGE_DATA_INVALID = 0, /**< No valid data yet */
        IMAGE_DATA_PIXELS,      /**< Raw pixel data is most recent */
        IMAGE_DATA_CH0,         /**< ch[0] most recent */
        IMAGE_DATA_CH1,         /**< ch[1] most recent */
        IMAGE_DATA_CH2,         /**< ch[2] most recent */
        IMAGE_DATA_CH3,         /**< ch[3] most recent */
        IMAGE_DATA_CH4,         /**< ch[4] most recent */
        IMAGE_DATA_CH5,         /**< ch[5] most recent */
        IMAGE_DATA_COMPLEX,     /**< Complex data in ch[0] and ch[1] */
        IMAGE_DATA_MAGNITUDE,   /**< Magnitude spectrum in ch[0] */
        IMAGE_DATA_PHASE        /**< Phase spectrum in ch[0] */
    } ImageDataState;

    /* -------------------------------------------------------------------------- */
    /* Helper structs                                                             */
    /* -------------------------------------------------------------------------- */
    /**
     * @struct Rectangle
     * @brief Axis-aligned rectangle.
     */
    typedef struct
    {
        int x;      /**< X coordinate (top-left) */
        int y;      /**< Y coordinate (top-left) */
        int width;  /**< Width in pixels */
        int height; /**< Height in pixels */
    } Rectangle;

    /**
     * @struct GMMStats
     * @brief Simple Gaussian mixture model statistics (per channel).
     */
    typedef struct
    {
        float mean[3]; /**< Per-channel mean (RGB) */
        float var[3];  /**< Per-channel variance (diagonal covariance) */
        int count;     /**< Number of samples accumulated */
    } GMMStats;

    /**
     * @struct channels_t
     * @brief Floating-point image channel storage (up to 6 channels).
     */
    typedef struct
    {
#ifdef __cplusplus
        float *ch[6]; /**< Channel array: ch[0] = l, ch[1] = r, etc. */

        float *&l() { return ch[0]; }  /**< Luminance/grayscale */
        float *&r() { return ch[1]; }  /**< Red channel */
        float *&g() { return ch[2]; }  /**< Green channel */
        float *&b() { return ch[3]; }  /**< Blue channel */
        float *&fx() { return ch[4]; } /**< Optional horizontal FFT data */
        float *&fy() { return ch[5]; } /**< Optional vertical FFT data */
#else
    union
    {
        float *ch[6];
        struct
        {
            float *l;
            float *r;
            float *g;
            float *b;
            float *fx;
            float *fy;
        };
    };
#endif
    } channels_t;

    /* -------------------------------------------------------------------------- */
    /* Main image container                                                       */
    /* -------------------------------------------------------------------------- */
    /**
     * @struct Image
     * @brief Represents an image with both raw pixel data and optional float channels.
     */
    typedef struct
    {
        uint32_t width;     /**< Image width in pixels */
        uint32_t height;    /**< Image height in pixels */
        void *pixels;       /**< Pointer to raw pixel data */
        channels_t *chals;  /**< Pointer to channel data (optional) */
        bool is_chals;      /**< True if chals is valid/allocated */
        uint32_t size;      /**< Total number of elements (width × height × channels) */
        ImageFormat format; /**< Image color format */
        ImageDepth depth;   /**< Pixel depth */
        ImageDataState log; /**< Most recent/valid data location */
    } Image;

#ifdef __cplusplus
}
#endif

/** @} */ /* end of embedDIP_image */

#endif /* IMAGE_H */

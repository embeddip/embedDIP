// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef IMAGE_H
#define IMAGE_H

#include <stdbool.h>
#include <stdint.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    #define EMBEDDIP_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
#elif defined(__cplusplus) && (__cplusplus >= 201103L)
    #define EMBEDDIP_STATIC_ASSERT(cond, msg) static_assert((cond), msg)
#else
    #define EMBEDDIP_STATIC_ASSERT(cond, msg) /* no static assert */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum intensity value for 8-bit formats (fallback for non-C99
 * libs). */
#ifndef UINT8_MAX
    #define UINT8_MAX 255u
#endif

/* -------------------------------------------------------------------------- */
/* Resolution enumeration                                                     */
/* -------------------------------------------------------------------------- */
/**
 * @enum ImageResolution
 * @brief Predefined image resolutions with associated indices.
 */
typedef enum {
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

/** @brief Total number of entries in the resolution LUTs. */
#define IMAGE_RES_COUNT ((int)IMAGE_RES_WQVGA + 1)

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

#define IMAGE_RES_INVALID_Width 0xFFu
#define IMAGE_RES_INVALID_Height 0xFFu
#define IMAGE_RES_CUSTOM_Width 0xEFu
#define IMAGE_RES_CUSTOM_Height 0xEFu
/** @} */

/* -------------------------------------------------------------------------- */
/* Lookup tables (header-local, const, cache friendly)                        */
/* -------------------------------------------------------------------------- */

/** @brief Lookup table for image widths by resolution index. */
static const uint16_t RES_WIDTH_LOOKUP[IMAGE_RES_COUNT] = {
    IMAGE_RES_96X96_Width,   IMAGE_RES_QQVGA_Width, IMAGE_RES_QCIF_Width,    IMAGE_RES_HQVGA_Width,
    IMAGE_RES_240X240_Width, IMAGE_RES_QVGA_Width,  IMAGE_RES_CIF_Width,     IMAGE_RES_HVGA_Width,
    IMAGE_RES_VGA_Width,     IMAGE_RES_SVGA_Width,  IMAGE_RES_XGA_Width,     IMAGE_RES_HD_Width,
    IMAGE_RES_SXGA_Width,    IMAGE_RES_UXGA_Width,  IMAGE_RES_FHD_Width,     IMAGE_RES_P_HD_Width,
    IMAGE_RES_P_3MP_Width,   IMAGE_RES_QXGA_Width,  IMAGE_RES_QHD_Width,     IMAGE_RES_WQXGA_Width,
    IMAGE_RES_P_FHD_Width,   IMAGE_RES_QSXGA_Width, IMAGE_RES_INVALID_Width, IMAGE_RES_CUSTOM_Width,
    IMAGE_RES_WQVGA_Width};

/** @brief Lookup table for image heights by resolution index. */
static const uint16_t RES_HEIGHT_LOOKUP[IMAGE_RES_COUNT] = {
    IMAGE_RES_96X96_Height, IMAGE_RES_QQVGA_Height,   IMAGE_RES_QCIF_Height,
    IMAGE_RES_HQVGA_Height, IMAGE_RES_240X240_Height, IMAGE_RES_QVGA_Height,
    IMAGE_RES_CIF_Height,   IMAGE_RES_HVGA_Height,    IMAGE_RES_VGA_Height,
    IMAGE_RES_SVGA_Height,  IMAGE_RES_XGA_Height,     IMAGE_RES_HD_Height,
    IMAGE_RES_SXGA_Height,  IMAGE_RES_UXGA_Height,    IMAGE_RES_FHD_Height,
    IMAGE_RES_P_HD_Height,  IMAGE_RES_P_3MP_Height,   IMAGE_RES_QXGA_Height,
    IMAGE_RES_QHD_Height,   IMAGE_RES_WQXGA_Height,   IMAGE_RES_P_FHD_Height,
    IMAGE_RES_QSXGA_Height, IMAGE_RES_INVALID_Height, IMAGE_RES_CUSTOM_Height,
    IMAGE_RES_WQVGA_Height};

EMBEDDIP_STATIC_ASSERT((sizeof(RES_WIDTH_LOOKUP) / sizeof(RES_WIDTH_LOOKUP[0])) == IMAGE_RES_COUNT,
                       "RES_WIDTH_LOOKUP size mismatch with IMAGE_RES_COUNT");

EMBEDDIP_STATIC_ASSERT((sizeof(RES_HEIGHT_LOOKUP) / sizeof(RES_HEIGHT_LOOKUP[0])) ==
                           IMAGE_RES_COUNT,
                       "RES_HEIGHT_LOOKUP size mismatch with IMAGE_RES_COUNT");

/**
 * @brief Get width of a predefined resolution.
 * @warning For IMAGE_RES_CUSTOM the return value is a sentinel, not the real
 * width.
 */
static inline uint16_t image_res_width(ImageResolution res)
{
    return RES_WIDTH_LOOKUP[(int)res];
}

/**
 * @brief Get height of a predefined resolution.
 * @warning For IMAGE_RES_CUSTOM the return value is a sentinel, not the real
 * height.
 */
static inline uint16_t image_res_height(ImageResolution res)
{
    return RES_HEIGHT_LOOKUP[(int)res];
}

/* -------------------------------------------------------------------------- */
/* Image format, depth, and metadata                                          */
/* -------------------------------------------------------------------------- */
/**
 * @enum ImageFormat
 * @brief Supported color formats for images.
 */
typedef enum {
    IMAGE_FORMAT_GRAYSCALE = 0, /**< 1 channel grayscale */
    IMAGE_FORMAT_RGB888,        /**< 3×8-bit RGB */
    IMAGE_FORMAT_RGB565,        /**< Packed 16-bit RGB */
    IMAGE_FORMAT_YUV,           /**< YUV (3 channels) */
    IMAGE_FORMAT_HSI,           /**< HSI color space, usually float */
    IMAGE_FORMAT_MASK           /**< 1 channel mask (trimap 0/1/2 or binary 0/255) */
} ImageFormat;

/**
 * @enum ImageDepth
 * @brief Bit-depth or precision of pixel data.
 */
typedef enum {
    IMAGE_DEPTH_U8 = 1,  /**< Unsigned 8-bit  */
    IMAGE_DEPTH_U16 = 2, /**< Unsigned 16-bit */
    IMAGE_DEPTH_U24 = 3, /**< Packed RGB888   */
    IMAGE_DEPTH_F32 = 4  /**< 32-bit float    */
} ImageDepth;

/**
 * @enum ImageDataState
 * @brief Indicator of the most recent or valid data location within ::Image.
 */
typedef enum {
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
typedef struct {
    int32_t x;      /**< X coordinate (top-left) */
    int32_t y;      /**< Y coordinate (top-left) */
    int32_t width;  /**< Width in pixels  */
    int32_t height; /**< Height in pixels */
} Rectangle;

/**
 * @brief Integer pixel coordinate.
 */
typedef struct {
    int x; /**< X coordinate */
    int y; /**< Y coordinate */
} Point;

/**
 * @brief Structuring element used in morphological operations.
 */
typedef struct {
    uint8_t *data;  /**< kernel[i * size + j], row-major */
    uint8_t size;   /**< kernel is square (size x size) */
    uint8_t anchor; /**< usually size / 2 */
} Kernel;

/**
 * @brief Supported structuring-element shapes for morphology.
 */
typedef enum {
    MORPH_RECT = 0,   /**< Rectangular structuring element */
    MORPH_CROSS = 1,  /**< Cross-shaped structuring element */
    MORPH_ELLIPSE = 2 /**< Elliptical structuring element */
} MorphShape;

/**
 * @struct channels_t
 * @brief Floating-point image channel storage (up to 6 channels).
 */
typedef struct {
#ifdef __cplusplus
    float *ch[6]; /**< Channel array: ch[0] = l, ch[1] = r, etc. */

    float *&l()
    {
        return ch[0];
    } /**< Luminance/grayscale */
    float *&r()
    {
        return ch[1];
    } /**< Red channel         */
    float *&g()
    {
        return ch[2];
    } /**< Green channel       */
    float *&b()
    {
        return ch[3];
    } /**< Blue channel        */
    float *&fx()
    {
        return ch[4];
    } /**< Optional horizontal FFT data */
    float *&fy()
    {
        return ch[5];
    } /**< Optional vertical FFT data   */
#else
    union {
        float *ch[6];
        struct {
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
 * @brief Represents an image with both raw pixel data and optional float
 * channels.
 */
typedef struct {
    uint32_t width;     /**< Image width in pixels  */
    uint32_t height;    /**< Image height in pixels */
    void *pixels;       /**< Pointer to raw pixel data */
    channels_t *chals;  /**< Pointer to channel data (optional) */
    uint32_t size;      /**< Total elements (width × height × channels) */
    ImageFormat format; /**< Image color format */
    ImageDepth depth;   /**< Pixel depth/precision */
    ImageDataState log; /**< Most recent/valid data location */
    bool is_chals;      /**< True if chals is valid/allocated */
} Image;

/* Small helpers for format/depth
 * ------------------------------------------------ */

/**
 * @brief Get number of logical channels for a given format.
 * @note RGB565 returns 3 (conceptual) channels.
 */
static inline uint8_t image_num_channels(ImageFormat fmt)
{
    switch (fmt) {
    case IMAGE_FORMAT_GRAYSCALE:
        return 1u;
    case IMAGE_FORMAT_RGB888:
        return 3u;
    case IMAGE_FORMAT_RGB565:
        return 3u;
    case IMAGE_FORMAT_YUV:
        return 3u;
    case IMAGE_FORMAT_HSI:
        return 3u;
    case IMAGE_FORMAT_MASK:
        return 1u;
    default:
        return 0u;
    }
}

/**
 * @brief Get pixel size in bytes for a given format/depth combination.
 *
 * This is primarily useful for buffer allocations and sanity checks.
 */
static inline uint8_t image_pixel_size_bytes(ImageFormat fmt, ImageDepth depth)
{
    (void)fmt; /* fmt kept for future extensions, currently depth drives size */

    switch (depth) {
    case IMAGE_DEPTH_U8:
        return 1u;
    case IMAGE_DEPTH_U16:
        return 2u;
    case IMAGE_DEPTH_U24:
        return 3u;
    case IMAGE_DEPTH_F32:
        return 4u;
    default:
        return 0u;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_H */

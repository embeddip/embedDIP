// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "color.h"

#include "core/error.h"

#include <math.h>
#include <string.h>

// Static conversion functions

// ----------- Inline Math Helpers (to avoid linking issues) ------------
static inline float inline_fmodf(float x, float y)
{
    if (y == 0.0f)
        return 0.0f;
    return x - ((int)(x / y)) * y;
}

static inline float inline_fabsf(float x)
{
    return (x < 0.0f) ? -x : x;
}

static inline float inline_fminf(float a, float b)
{
    return (a < b) ? a : b;
}

static inline float inline_fmaxf(float a, float b)
{
    return (a > b) ? a : b;
}

static inline float inline_roundf(float x)
{
    return (x >= 0.0f) ? (float)(int)(x + 0.5f) : (float)(int)(x - 0.5f);
}

// ----------- Helper Macros ------------
#define RGB888_TO_RGB565(r, g, b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))
#define RGB565_TO_RGB888(pixel, r, g, b)                                                           \
    do {                                                                                           \
        r = ((pixel >> 8) & 0xF8) | ((pixel >> 13) & 0x07);                                        \
        g = ((pixel >> 3) & 0xFC) | ((pixel >> 9) & 0x03);                                         \
        b = ((pixel << 3) & 0xF8) | ((pixel >> 2) & 0x07);                                         \
    } while (0)

// Validation macros to replace assert()
#define CHECK_NULL(ptr)                                                                            \
    do {                                                                                           \
        if (!(ptr))                                                                                \
            return EMBEDDIP_ERROR_NULL_PTR;                                                        \
    } while (0)

#define CHECK_FORMAT(img, expected_fmt)                                                            \
    do {                                                                                           \
        if ((img)->format != (expected_fmt))                                                       \
            return EMBEDDIP_ERROR_INVALID_FORMAT;                                                  \
    } while (0)

#define CHECK_DEPTH(img, expected_depth)                                                           \
    do {                                                                                           \
        if ((img)->depth != (expected_depth))                                                      \
            return EMBEDDIP_ERROR_INVALID_DEPTH;                                                   \
    } while (0)

// ----------- RGB888 → GRAYSCALE ------------
static embeddip_status_t rgb888_to_grayscale(const Image *inImg, Image *outImg)
{
    CHECK_NULL(inImg);
    CHECK_NULL(outImg);
    CHECK_FORMAT(inImg, IMAGE_FORMAT_RGB888);
    CHECK_DEPTH(inImg, IMAGE_DEPTH_U24);
    CHECK_FORMAT(outImg, IMAGE_FORMAT_GRAYSCALE);
    CHECK_DEPTH(outImg, IMAGE_DEPTH_U8);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    uint32_t pixel_count = inImg->width * inImg->height;
    for (uint32_t i = 0; i < pixel_count; ++i) {
        uint8_t r = in[i * 3];
        uint8_t g = in[i * 3 + 1];
        uint8_t b = in[i * 3 + 2];
        out[i] = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
    }

    return EMBEDDIP_OK;
}

// ----------- GRAYSCALE → RGB888 ------------
static embeddip_status_t grayscale_to_rgb(const Image *inImg, Image *outImg)
{
    CHECK_NULL(inImg);
    CHECK_NULL(outImg);
    CHECK_FORMAT(inImg, IMAGE_FORMAT_GRAYSCALE);
    CHECK_DEPTH(inImg, IMAGE_DEPTH_U8);
    CHECK_FORMAT(outImg, IMAGE_FORMAT_RGB888);
    CHECK_DEPTH(outImg, IMAGE_DEPTH_U24);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i) {
        uint8_t gray = in[i];
        out[i * 3] = gray;
        out[i * 3 + 1] = gray;
        out[i * 3 + 2] = gray;
    }

    return EMBEDDIP_OK;
}

// ----------- RGB888 → RGB565 ------------
static embeddip_status_t rgb888_to_rgb565(const Image *inImg, Image *outImg)
{
    CHECK_NULL(inImg);
    CHECK_NULL(outImg);
    CHECK_FORMAT(inImg, IMAGE_FORMAT_RGB888);
    CHECK_DEPTH(inImg, IMAGE_DEPTH_U24);
    CHECK_FORMAT(outImg, IMAGE_FORMAT_RGB565);
    CHECK_DEPTH(outImg, IMAGE_DEPTH_U16);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint16_t *out = (uint16_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i) {
        uint8_t r = in[i * 3];
        uint8_t g = in[i * 3 + 1];
        uint8_t b = in[i * 3 + 2];
        out[i] = RGB888_TO_RGB565(r, g, b);
    }

    return EMBEDDIP_OK;
}

// ----------- RGB565 → RGB888 ------------
static embeddip_status_t convert_rgb565_to_rgb888(const Image *inImg, Image *outImg)
{
    CHECK_NULL(inImg);
    CHECK_NULL(outImg);
    CHECK_FORMAT(inImg, IMAGE_FORMAT_RGB565);
    CHECK_DEPTH(inImg, IMAGE_DEPTH_U16);
    CHECK_FORMAT(outImg, IMAGE_FORMAT_RGB888);
    CHECK_DEPTH(outImg, IMAGE_DEPTH_U24);

    const uint16_t *in = (const uint16_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i) {
        uint8_t r, g, b;
        RGB565_TO_RGB888(in[i], r, g, b);
        out[i * 3] = r;
        out[i * 3 + 1] = g;
        out[i * 3 + 2] = b;
    }

    return EMBEDDIP_OK;
}

// ----------- RGB888 → YUV ------------
static embeddip_status_t rgb888_to_yuv(const Image *inImg, Image *outImg)
{
    CHECK_NULL(inImg);
    CHECK_NULL(outImg);
    CHECK_FORMAT(inImg, IMAGE_FORMAT_RGB888);
    CHECK_FORMAT(outImg, IMAGE_FORMAT_YUV);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

#define CLIP(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))

    for (uint32_t i = 0; i < inImg->size; ++i) {
        uint8_t r = in[i * 3];
        uint8_t g = in[i * 3 + 1];
        uint8_t b = in[i * 3 + 2];

        int y = (int)(inline_roundf(0.299f * r + 0.587f * g + 0.114f * b));
        int u = (int)(inline_roundf(-0.14713f * r - 0.28886f * g + 0.436f * b)) + 128;
        int v = (int)(inline_roundf(0.615f * r - 0.51499f * g - 0.10001f * b)) + 128;

        out[i * 3] = (uint8_t)CLIP(y);
        out[i * 3 + 1] = (uint8_t)CLIP(u);
        out[i * 3 + 2] = (uint8_t)CLIP(v);
    }

    return EMBEDDIP_OK;
}

// ----------- YUV → RGB888 ------------
static embeddip_status_t yuv_to_rgb888(const Image *inImg, Image *outImg)
{
    CHECK_NULL(inImg);
    CHECK_NULL(outImg);
    CHECK_FORMAT(inImg, IMAGE_FORMAT_YUV);
    CHECK_FORMAT(outImg, IMAGE_FORMAT_RGB888);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i) {
        int y = in[i * 3];
        int u = in[i * 3 + 1] - 128;
        int v = in[i * 3 + 2] - 128;

        int r = (int)(y + 1.403 * v);
        int g = (int)(y - 0.344 * u - 0.714 * v);
        int b = (int)(y + 1.770 * u);

        out[i * 3] = (uint8_t)inline_fminf(inline_fmaxf(r, 0), 255);
        out[i * 3 + 1] = (uint8_t)inline_fminf(inline_fmaxf(g, 0), 255);
        out[i * 3 + 2] = (uint8_t)inline_fminf(inline_fmaxf(b, 0), 255);
    }

    return EMBEDDIP_OK;
}

// ----------- RGB888 → HSI ------------
static embeddip_status_t rgb888_to_HSI(const Image *inImg, Image *outImg)
{
    CHECK_NULL(inImg);
    CHECK_NULL(outImg);
    CHECK_FORMAT(inImg, IMAGE_FORMAT_RGB888);
    CHECK_FORMAT(outImg, IMAGE_FORMAT_HSI);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i) {
        float r = in[i * 3] / 255.0f;
        float g = in[i * 3 + 1] / 255.0f;
        float b = in[i * 3 + 2] / 255.0f;

        float max = inline_fmaxf(inline_fmaxf(r, g), b);
        float min = inline_fminf(inline_fminf(r, g), b);
        float delta = max - min;

        float h = 0.0f, s, v = max;

        if (delta != 0) {
            if (max == r)
                h = 60 * inline_fmodf((g - b) / delta, 6.0f);
            else if (max == g)
                h = 60 * ((b - r) / delta + 2);
            else
                h = 60 * ((r - g) / delta + 4);
            if (h < 0)
                h += 360.0f;
        }

        s = (max == 0) ? 0 : (delta / max);

        out[i * 3] = (uint8_t)(h / 360.0f * 255.0f);
        out[i * 3 + 1] = (uint8_t)(s * 255.0f);
        out[i * 3 + 2] = (uint8_t)(v * 255.0f);
    }

    return EMBEDDIP_OK;
}

// ----------- HSI → RGB888 ------------
static embeddip_status_t HSI_to_rgb888(const Image *inImg, Image *outImg)
{
    CHECK_NULL(inImg);
    CHECK_NULL(outImg);
    CHECK_FORMAT(inImg, IMAGE_FORMAT_HSI);
    CHECK_FORMAT(outImg, IMAGE_FORMAT_RGB888);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i) {
        float h = in[i * 3] / 255.0f * 360.0f;
        float s = in[i * 3 + 1] / 255.0f;
        float v = in[i * 3 + 2] / 255.0f;

        float c = v * s;
        float x = c * (1 - inline_fabsf(inline_fmodf(h / 60.0f, 2) - 1));
        float m = v - c;

        float r_, g_, b_;
        if (h < 60) {
            r_ = c;
            g_ = x;
            b_ = 0;
        } else if (h < 120) {
            r_ = x;
            g_ = c;
            b_ = 0;
        } else if (h < 180) {
            r_ = 0;
            g_ = c;
            b_ = x;
        } else if (h < 240) {
            r_ = 0;
            g_ = x;
            b_ = c;
        } else if (h < 300) {
            r_ = x;
            g_ = 0;
            b_ = c;
        } else {
            r_ = c;
            g_ = 0;
            b_ = x;
        }

        out[i * 3] = (uint8_t)((r_ + m) * 255.0f);
        out[i * 3 + 1] = (uint8_t)((g_ + m) * 255.0f);
        out[i * 3 + 2] = (uint8_t)((b_ + m) * 255.0f);
    }

    return EMBEDDIP_OK;
}

// ----------- RGB565 → HSI ------------
static embeddip_status_t rgb565_to_HSI(const Image *inImg, Image *outImg)
{
    CHECK_NULL(inImg);
    CHECK_NULL(outImg);
    CHECK_FORMAT(inImg, IMAGE_FORMAT_RGB565);
    CHECK_FORMAT(outImg, IMAGE_FORMAT_HSI);
    CHECK_DEPTH(inImg, IMAGE_DEPTH_U8);
    CHECK_DEPTH(outImg, IMAGE_DEPTH_U8);

    const uint16_t *in = (const uint16_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i) {
        uint8_t r, g, b;
        RGB565_TO_RGB888(in[i], r, g, b);

        float rf = r / 255.0f;
        float gf = g / 255.0f;
        float bf = b / 255.0f;

        float max = inline_fmaxf(inline_fmaxf(rf, gf), bf);
        float min = inline_fminf(inline_fminf(rf, gf), bf);
        float delta = max - min;

        float h = 0.0f, s = 0.0f, v = max;

        if (delta > 0.0001f) {
            if (max == rf)
                h = 60.0f * inline_fmodf(((gf - bf) / delta), 6.0f);
            else if (max == gf)
                h = 60.0f * (((bf - rf) / delta) + 2.0f);
            else  // max == bf
                h = 60.0f * (((rf - gf) / delta) + 4.0f);

            if (h < 0.0f)
                h += 360.0f;

            s = delta / max;
        }

        out[i * 3] = (uint8_t)(h / 360.0f * 255.0f);
        out[i * 3 + 1] = (uint8_t)(s * 255.0f);
        out[i * 3 + 2] = (uint8_t)(v * 255.0f);
    }

    return EMBEDDIP_OK;
}

// ----------- HSI → RGB565 ------------
static embeddip_status_t HSI_to_rgb565(const Image *inImg, Image *outImg)
{
    CHECK_NULL(inImg);
    CHECK_NULL(outImg);
    CHECK_FORMAT(inImg, IMAGE_FORMAT_HSI);
    CHECK_FORMAT(outImg, IMAGE_FORMAT_RGB565);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint16_t *out = (uint16_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i) {
        float h = in[i * 3] / 255.0f * 360.0f;
        float s = in[i * 3 + 1] / 255.0f;
        float v = in[i * 3 + 2] / 255.0f;

        float c = v * s;
        float x = c * (1 - inline_fabsf(inline_fmodf(h / 60.0f, 2) - 1));
        float m = v - c;

        float r_, g_, b_;
        if (h < 60) {
            r_ = c;
            g_ = x;
            b_ = 0;
        } else if (h < 120) {
            r_ = x;
            g_ = c;
            b_ = 0;
        } else if (h < 180) {
            r_ = 0;
            g_ = c;
            b_ = x;
        } else if (h < 240) {
            r_ = 0;
            g_ = x;
            b_ = c;
        } else if (h < 300) {
            r_ = x;
            g_ = 0;
            b_ = c;
        } else {
            r_ = c;
            g_ = 0;
            b_ = x;
        }

        uint8_t r = (uint8_t)((r_ + m) * 255.0f);
        uint8_t g = (uint8_t)((g_ + m) * 255.0f);
        uint8_t b = (uint8_t)((b_ + m) * 255.0f);

        out[i] = RGB888_TO_RGB565(r, g, b);
    }

    return EMBEDDIP_OK;
}

embeddip_status_t cvtColor(const Image *src, Image *dst, ColorConversionCode code)
{
    /* Validate input parameters */
    if (!src || !dst) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (!src->pixels || !dst->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    switch (code) {
    // ------------------- RGB888 TO -------------------
    case CVT_RGB888_TO_GRAYSCALE:
        return rgb888_to_grayscale(src, dst);

    case CVT_RGB888_TO_RGB565:
        return rgb888_to_rgb565(src, dst);

    case CVT_RGB888_TO_YUV:
        return rgb888_to_yuv(src, dst);

    case CVT_RGB888_TO_HSI:
        return rgb888_to_HSI(src, dst);

    // ------------------- RGB565 TO -------------------
    case CVT_RGB565_TO_RGB888:
        return convert_rgb565_to_rgb888(src, dst);

    case CVT_RGB565_TO_GRAYSCALE: {
        Image *tmp = (Image *)createImageWH_legacy(src->width, src->height, IMAGE_FORMAT_RGB888);
        if (!tmp)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        embeddip_status_t status = convert_rgb565_to_rgb888(src, tmp);
        if (status != EMBEDDIP_OK) {
            deleteImage(tmp);
            return status;
        }
        status = rgb888_to_grayscale(tmp, dst);
        deleteImage(tmp);
        return status;
    }

    case CVT_RGB565_TO_YUV: {
        Image *tmp = (Image *)createImageWH_legacy(src->width, src->height, IMAGE_FORMAT_RGB888);
        if (!tmp)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        embeddip_status_t status = convert_rgb565_to_rgb888(src, tmp);
        if (status != EMBEDDIP_OK) {
            deleteImage(tmp);
            return status;
        }
        status = rgb888_to_yuv(tmp, dst);
        deleteImage(tmp);
        return status;
    }

    case CVT_RGB565_TO_HSI:
        return rgb565_to_HSI(src, dst);

    // ------------------- GRAYSCALE TO -------------------
    case CVT_GRAYSCALE_TO_RGB888:
        return grayscale_to_rgb(src, dst);

    case CVT_GRAYSCALE_TO_RGB565: {
        Image *tmp = (Image *)createImageWH_legacy(src->width, src->height, IMAGE_FORMAT_RGB888);
        if (!tmp)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        embeddip_status_t status = grayscale_to_rgb(src, tmp);
        if (status != EMBEDDIP_OK) {
            deleteImage(tmp);
            return status;
        }
        status = rgb888_to_rgb565(tmp, dst);
        deleteImage(tmp);
        return status;
    }

    case CVT_GRAYSCALE_TO_YUV: {
        Image *tmp = (Image *)createImageWH_legacy(src->width, src->height, IMAGE_FORMAT_RGB888);
        if (!tmp)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        embeddip_status_t status = grayscale_to_rgb(src, tmp);
        if (status != EMBEDDIP_OK) {
            deleteImage(tmp);
            return status;
        }
        status = rgb888_to_yuv(tmp, dst);
        deleteImage(tmp);
        return status;
    }

    case CVT_GRAYSCALE_TO_HSI: {
        Image *tmp = (Image *)createImageWH_legacy(src->width, src->height, IMAGE_FORMAT_RGB888);
        if (!tmp)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        embeddip_status_t status = grayscale_to_rgb(src, tmp);
        if (status != EMBEDDIP_OK) {
            deleteImage(tmp);
            return status;
        }
        status = rgb888_to_HSI(tmp, dst);
        deleteImage(tmp);
        return status;
    }

    // ------------------- YUV TO -------------------
    case CVT_YUV_TO_RGB888:
        return yuv_to_rgb888(src, dst);

    case CVT_YUV_TO_RGB565: {
        Image *tmp = (Image *)createImageWH_legacy(src->width, src->height, IMAGE_FORMAT_RGB888);
        if (!tmp)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        embeddip_status_t status = yuv_to_rgb888(src, tmp);
        if (status != EMBEDDIP_OK) {
            deleteImage(tmp);
            return status;
        }
        status = rgb888_to_rgb565(tmp, dst);
        deleteImage(tmp);
        return status;
    }

    case CVT_YUV_TO_GRAYSCALE: {
        Image *tmp = (Image *)createImageWH_legacy(src->width, src->height, IMAGE_FORMAT_RGB888);
        if (!tmp)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        embeddip_status_t status = yuv_to_rgb888(src, tmp);
        if (status != EMBEDDIP_OK) {
            deleteImage(tmp);
            return status;
        }
        status = rgb888_to_grayscale(tmp, dst);
        deleteImage(tmp);
        return status;
    }

    case CVT_YUV_TO_HSI: {
        Image *tmp = (Image *)createImageWH_legacy(src->width, src->height, IMAGE_FORMAT_RGB888);
        if (!tmp)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        embeddip_status_t status = yuv_to_rgb888(src, tmp);
        if (status != EMBEDDIP_OK) {
            deleteImage(tmp);
            return status;
        }
        status = rgb888_to_HSI(tmp, dst);
        deleteImage(tmp);
        return status;
    }

    // ------------------- HSI TO -------------------
    case CVT_HSI_TO_RGB888:
        return HSI_to_rgb888(src, dst);

    case CVT_HSI_TO_RGB565:
        return HSI_to_rgb565(src, dst);

    case CVT_HSI_TO_GRAYSCALE: {
        Image *tmp = (Image *)createImageWH_legacy(src->width, src->height, IMAGE_FORMAT_RGB888);
        if (!tmp)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        embeddip_status_t status = HSI_to_rgb888(src, tmp);
        if (status != EMBEDDIP_OK) {
            deleteImage(tmp);
            return status;
        }
        status = rgb888_to_grayscale(tmp, dst);
        deleteImage(tmp);
        return status;
    }

    case CVT_HSI_TO_YUV: {
        Image *tmp = (Image *)createImageWH_legacy(src->width, src->height, IMAGE_FORMAT_RGB888);
        if (!tmp)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        embeddip_status_t status = HSI_to_rgb888(src, tmp);
        if (status != EMBEDDIP_OK) {
            deleteImage(tmp);
            return status;
        }
        status = rgb888_to_yuv(tmp, dst);
        deleteImage(tmp);
        return status;
    }

    // ------------------- COPY -------------------
    case CVT_COPY:
        if (src->format != dst->format) {
            return EMBEDDIP_ERROR_INVALID_FORMAT;
        }
        if (src->width != dst->width || src->height != dst->height) {
            return EMBEDDIP_ERROR_INVALID_SIZE;
        }
        if ((size_t)dst->size * (size_t)dst->depth < (size_t)src->size * (size_t)src->depth) {
            return EMBEDDIP_ERROR_INVALID_SIZE;
        }
        memcpy(dst->pixels, src->pixels, (size_t)src->size * (size_t)src->depth);
        break;

    default:
        /* Unsupported conversion code */
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }

    return EMBEDDIP_OK;
}

/**
 * @brief Applies hue-based filtering to an HSI image and outputs a binary mask.
 *
 * @param[in]  input     Pointer to the input HSI image.
 * @param[out] output    Pointer to the output GRAYSCALE/MASK image.
 * @param[in]  minHue    Minimum hue value (0–255) for thresholding.
 * @param[in]  maxHue    Maximum hue value (0–255) for thresholding.
 */
embeddip_status_t hueThreshold(const Image *src, Image *dst, float min_hue, float max_hue)
{
    if (!src || !dst) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (src->format != IMAGE_FORMAT_HSI) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }
    if (dst->format != IMAGE_FORMAT_GRAYSCALE && dst->format != IMAGE_FORMAT_MASK) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }
    if (src->size != dst->size) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    const uint8_t *in = (const uint8_t *)src->pixels;
    uint8_t *out = (uint8_t *)dst->pixels;

    for (uint32_t i = 0; i < src->size; ++i) {
        uint8_t h = in[i * 3];  // Hue channel

        if (min_hue <= max_hue) {
            // Normal range (no wraparound)
            if (h >= min_hue && h <= max_hue) {
                out[i] = 255;
            } else {
                out[i] = 0;
            }
        } else  // Handle hue wraparound
        {
            if (h >= min_hue || h <= max_hue) {
                out[i] = 255;
            } else {
                out[i] = 0;
            }
        }
    }

    return EMBEDDIP_OK;
}

/**
 * @brief Band thresholding (OpenCV-style).
 *
 * @param[in]  src   Input image (3-channel).
 * @param[out] mask    Output binary mask (1-channel, same width/height).
 * @param[in]  lower   Lower bound array [3].
 * @param[in]  upper   Upper bound array [3].
 */
embeddip_status_t
inRange(const Image *src, Image *mask, const uint8_t lower[3], const uint8_t upper[3])
{
    // Null pointer checks
    if (!src || !mask || !lower || !upper) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (!src->pixels || !mask->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    // Format validation - src must be 3-channel
    if (src->format != IMAGE_FORMAT_RGB888 && src->format != IMAGE_FORMAT_HSI &&
        src->format != IMAGE_FORMAT_YUV) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    // Mask must be grayscale or mask format
    if (mask->format != IMAGE_FORMAT_GRAYSCALE && mask->format != IMAGE_FORMAT_MASK) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    // Dimension validation
    if (src->width != mask->width || src->height != mask->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    const uint8_t *in = (const uint8_t *)src->pixels;
    uint8_t *out = (uint8_t *)mask->pixels;

    for (uint32_t i = 0; i < src->size; ++i) {
        uint8_t c1 = in[i * 3 + 0];
        uint8_t c2 = in[i * 3 + 1];
        uint8_t c3 = in[i * 3 + 2];

        if (c1 >= lower[0] && c1 <= upper[0] && c2 >= lower[1] && c2 <= upper[1] &&
            c3 >= lower[2] && c3 <= upper[2]) {
            out[i] = 255;
        } else {
            out[i] = 0;
        }
    }

    return EMBEDDIP_OK;
}

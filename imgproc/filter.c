// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "filter.h"

#include "core/error.h"

#include <string.h> /* For memset, memcpy */

#include "float.h"

// Validation macros to replace assert()
#define CHECK_NULL_INT(ptr)                                                                        \
    do {                                                                                           \
        if (!(ptr))                                                                                \
            return EMBEDDIP_ERROR_NULL_PTR;                                                        \
    } while (0)

#define CHECK_FORMAT_INT(img, expected_fmt)                                                        \
    do {                                                                                           \
        if ((img)->format != (expected_fmt))                                                       \
            return EMBEDDIP_ERROR_INVALID_FORMAT;                                                  \
    } while (0)

#define CHECK_CONDITION_INT(cond)                                                                  \
    do {                                                                                           \
        if (!(cond))                                                                               \
            return EMBEDDIP_ERROR_INVALID_ARG;                                                     \
    } while (0)

// Internal helper — filters only one channel
uint8_t channel_mask[] = {
    0xFE,
    0xFD,
    0xFB,
    0xF7,
};

static inline int clamp_index(int idx, int limit)
{
    if (idx < 0)
        return 0;
    if (idx >= limit)
        return limit - 1;
    return idx;
}

// i am not sure about the kernel.
embeddip_status_t filter2D_single_channel(Image *src, Image *dst, int ch_idx, void *ctx)
{
    if (!src || !dst || !ctx) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    Filter2DContext *context = (Filter2DContext *)ctx;
    int size = context->size;
    int k = size / 2;
    float *kernel = context->kernel;

    int width = src->width;
    int height = src->height;
    int num_pixels = width * height;

    // Ensure output has channels allocated
    if (isChalsEmpty(dst)) {
        createChals(dst, dst->depth);
        dst->is_chals = 1;
    }

    // Ensure output channel is allocated
    if (dst->chals->ch[ch_idx] == NULL) {
        dst->chals->ch[ch_idx] = (float *)memory_alloc(num_pixels * sizeof(float));
        if (!dst->chals->ch[ch_idx]) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
    }

    float *outCh = dst->chals->ch[ch_idx];

    // Apply 2D convolution based on input data location
    if (src->log == IMAGE_DATA_PIXELS) {
        // Read directly from pixels buffer without allocating temp channel
        const uint8_t *raw = (const uint8_t *)src->pixels;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float sum = 0.0f;

                for (int fy = -k; fy <= k; ++fy) {
                    for (int fx = -k; fx <= k; ++fx) {
                        int iy = clamp_index(y + fy, height);
                        int ix = clamp_index(x + fx, width);

                        float val = 0.0f;
                        if (src->format == IMAGE_FORMAT_GRAYSCALE) {
                            val = (float)raw[iy * width + ix];
                        } else if (src->format == IMAGE_FORMAT_RGB888) {
                            val = (float)raw[(iy * width + ix) * 3 + ch_idx - 1];
                        }

                        sum += val * kernel[(fy + k) * size + (fx + k)];
                    }
                }

                outCh[y * width + x] = sum;
            }
        }
    } else {
        // Read from chals - use appropriate channel
        if (isChalsEmpty(src)) {
            createChals((Image *)src, src->depth);
        }

        // Map log state to channel index
        int in_ch_idx = ch_idx;
        if (src->log >= IMAGE_DATA_CH0 && src->log <= IMAGE_DATA_CH5) {
            in_ch_idx = src->log - IMAGE_DATA_CH0;
        } else if (src->log == IMAGE_DATA_MAGNITUDE || src->log == IMAGE_DATA_PHASE) {
            in_ch_idx = 0;
        }

        float *inCh = src->chals->ch[in_ch_idx];

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float sum = 0.0f;

                for (int fy = -k; fy <= k; ++fy) {
                    for (int fx = -k; fx <= k; ++fx) {
                        int iy = clamp_index(y + fy, height);
                        int ix = clamp_index(x + fx, width);

                        float val = inCh[iy * width + ix];
                        sum += val * kernel[(fy + k) * size + (fx + k)];
                    }
                }

                outCh[y * width + x] = sum;
            }
        }
    }

    return EMBEDDIP_OK;
}

/**
 * @brief Apply 2D convolution filter to an image
 *
 * @param[in]  inImg       Input image (must not be NULL)
 * @param[out] outImg      Output image (must be pre-allocated, same size as input)
 * @param[in]  kernel      Flattened kernel array in row-major order (kernelSize x kernelSize)
 * @param[in]  kernelSize  Size of the square kernel (must be odd: 3, 5, 7, etc.)
 * @return EMBEDDIP_OK on success, error code otherwise
 */
embeddip_status_t filter2D(const Image *src, Image *dst, const float *kernel, int kernel_size)
{
    // Input validation
    if (!src || !dst || !kernel) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    // Validate kernel size (must be odd and >= 1)
    if (kernel_size < 1 || (kernel_size % 2) == 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    // Validate image dimensions match
    if (src->width != dst->width || src->height != dst->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    // Validate image formats match
    if (src->format != dst->format) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    // Create Filter2DContext
    Filter2DContext context;
    context.size = kernel_size;
    context.kernel = (float *)kernel;  // Cast away const for internal use
    context.chal = 0;                  // Not used in this context

    // Dispatch filtering based on format
    embeddip_status_t status;
    if (src->format == IMAGE_FORMAT_GRAYSCALE) {
        // Filter single grayscale channel (ch_idx = 0)
        status = filter2D_single_channel((Image *)src, dst, 0, &context);
        if (status != EMBEDDIP_OK) {
            return status;
        }
        dst->log = IMAGE_DATA_CH0;
    } else if (src->format == IMAGE_FORMAT_RGB888) {
        // Filter each RGB channel independently
        status = filter2D_single_channel((Image *)src, dst, 1, &context);  // R channel
        if (status != EMBEDDIP_OK)
            return status;
        status = filter2D_single_channel((Image *)src, dst, 2, &context);  // G channel
        if (status != EMBEDDIP_OK)
            return status;
        status = filter2D_single_channel((Image *)src, dst, 3, &context);  // B channel
        if (status != EMBEDDIP_OK)
            return status;
        dst->log = IMAGE_DATA_CH1;  // Multi-channel RGB data processed
    } else {
        // Unsupported format
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }

    return EMBEDDIP_OK;
}

embeddip_status_t sepfilter2D(Image *src,
                              Image *dst,
                              int size_x,
                              float *kernel_x,
                              int size_y,
                              float *kernel_y,
                              float delta)
{
    if (!src || !dst || !kernel_x || !kernel_y) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (kernel_y != kernel_x || size_x != size_y) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    int half = size_x / 2;
    int width = src->width;
    int height = src->height;
    int num_pixels = width * height;

    // Ensure output has channels allocated
    if (isChalsEmpty(dst)) {
        createChals(dst, dst->depth);
        dst->is_chals = 1;
    }

    float *inCh = NULL;

    // Check where the valid input data is located based on log state
    if (src->log == IMAGE_DATA_PIXELS) {
        // Valid data is in pixels buffer - convert to float channel
        if (isChalsEmpty(src)) {
            createChals((Image *)src, src->depth);
        }

        if (src->chals->ch[0] == NULL) {
            src->chals->ch[0] = (float *)memory_alloc(num_pixels * sizeof(float));
        }

        inCh = src->chals->ch[0];
        const uint8_t *raw = (const uint8_t *)src->pixels;

        if (src->format == IMAGE_FORMAT_GRAYSCALE) {
            for (int i = 0; i < num_pixels; ++i)
                inCh[i] = (float)raw[i];
        } else if (src->format == IMAGE_FORMAT_RGB888) {
            for (int i = 0; i < num_pixels; ++i)
                inCh[i] = (float)raw[i * 3];
        }
    } else {
        // Valid data is in chals - use appropriate channel
        if (isChalsEmpty(src)) {
            createChals((Image *)src, src->depth);
        }

        // Map log state to channel index
        int ch_idx = 0;
        if (src->log >= IMAGE_DATA_CH0 && src->log <= IMAGE_DATA_CH5) {
            ch_idx = src->log - IMAGE_DATA_CH0;
        } else if (src->log == IMAGE_DATA_MAGNITUDE || src->log == IMAGE_DATA_PHASE) {
            ch_idx = 0;
        }

        inCh = src->chals->ch[ch_idx];
    }

    // Ensure output channel is allocated
    if (dst->chals->ch[0] == NULL) {
        dst->chals->ch[0] = (float *)memory_alloc(num_pixels * sizeof(float));
    }

    float *outCh = dst->chals->ch[0];

    // Temp buffer for intermediate results
    float *temp = (float *)memory_alloc(num_pixels * sizeof(float));

    // Vertical pass
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float sum = 0.0f;
            for (int ky = -half; ky <= half; ++ky) {
                int iy = clamp_index(y + ky, height);
                sum += inCh[iy * width + x] * kernel_y[ky + half];
            }
            temp[y * width + x] = sum;
        }
    }

    // Horizontal pass
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float sum = 0.0f;
            for (int kx = -half; kx <= half; ++kx) {
                int ix = clamp_index(x + kx, width);
                sum += temp[y * width + ix] * kernel_x[kx + half];
            }

            sum = (float)(sum * delta);
            outCh[y * width + x] = sum;
        }
    }

    memory_free(temp);

    dst->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/*
void wrapper(ImageOpFunc func, Image *src, Image *dst, void *context)
{
    assert(func && src && dst);
    assert(src->format == dst->format);

    // Ensure channels are allocated for input
    if (!src->is_chals)
    {
        src->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        src->is_chals = true;
        for (int i = 0; i < 4; ++i)
            src->chals->ch[i] = NULL;
    }

    // Ensure channels are allocated for output
    if (!dst->is_chals)
    {
        dst->chals = (channels_t *)memory_alloc(sizeof(channels_t)F);
        dst->is_chals = true;
        for (int i = 0; i < 4; ++i)
            dst->chals->ch[i] = NULL;
    }

    // Dispatch per format
    if (src->format == IMAGE_FORMAT_GRAYSCALE)
    {
        func(src, dst, 0, context); // l channel
    }
    else if (src->format == IMAGE_FORMAT_RGB888)
    {
        for (int ch = 1; ch <= 3; ++ch) // r=1, g=2, b=3
            func(src, dst, ch, context);
    }
    else
    {
        assert(false && "Unsupported format in wrapper");
    }
}
    */

/**
 * @brief Applies a min filter (non-linear) to the image using a square window.
 *
 * @param src Input image.
 * @param dst Output image after min filtering.
 * @param kernelSize Size of the square window (must be odd).
 */

embeddip_status_t minFilter(const Image *src, Image *dst, int kernel_size)
{
    if (!src || !dst) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (kernel_size < 1 || (kernel_size % 2) == 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    int kernelRadius = kernel_size / 2;

    // Ensure output has channels allocated
    if (isChalsEmpty(dst)) {
        createChals(dst, dst->depth);
        dst->is_chals = 1;
    }

    // Check where the valid input data is located based on log state
    if (src->log == IMAGE_DATA_PIXELS) {
        // Valid data is in pixels buffer - read as uint8_t
        uint8_t *pixels = (uint8_t *)src->pixels;

        for (uint32_t y = 0; y < src->height; ++y) {
            for (uint32_t x = 0; x < src->width; ++x) {
                uint8_t minPixelValue = UINT8_MAX;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky) {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx) {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < (int)src->width && offsetY >= 0 &&
                            offsetY < (int)src->height) {
                            uint8_t pixelValue = pixels[offsetY * src->width + offsetX];
                            if (pixelValue < minPixelValue) {
                                minPixelValue = pixelValue;
                            }
                        }
                    }
                }

                dst->chals->ch[0][y * src->width + x] = (float)minPixelValue;
            }
        }
    } else {
        // Valid data is in chals - determine which channel based on log
        if (isChalsEmpty(src)) {
            createChals((Image *)src, src->depth);
        }

        // Map log state to channel index
        int ch_idx = 0;
        if (src->log >= IMAGE_DATA_CH0 && src->log <= IMAGE_DATA_CH5) {
            ch_idx = src->log - IMAGE_DATA_CH0;
        } else if (src->log == IMAGE_DATA_MAGNITUDE || src->log == IMAGE_DATA_PHASE) {
            ch_idx = 0;
        }

        float *inCh = src->chals->ch[ch_idx];

        for (uint32_t y = 0; y < src->height; ++y) {
            for (uint32_t x = 0; x < src->width; ++x) {
                float minPixelValue = FLT_MAX;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky) {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx) {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < (int)src->width && offsetY >= 0 &&
                            offsetY < (int)src->height) {
                            float pixelValue = inCh[offsetY * src->width + offsetX];
                            if (pixelValue < minPixelValue) {
                                minPixelValue = pixelValue;
                            }
                        }
                    }
                }

                dst->chals->ch[0][y * src->width + x] = minPixelValue;
            }
        }
    }

    dst->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/**
 * @brief Applies a max filter (non-linear) to the image using a square window.
 *
 * @param src Input image.
 * @param dst Output image after max filtering.
 * @param kernel_size Size of the square window (must be odd).
 */

embeddip_status_t maxFilter(const Image *src, Image *dst, int kernel_size)
{
    if (!src || !dst) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (kernel_size < 1 || (kernel_size % 2) == 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    int kernelRadius = kernel_size / 2;

    // Ensure output has channels allocated
    if (isChalsEmpty(dst)) {
        createChals(dst, dst->depth);
        dst->is_chals = 1;
    }

    // Check where the valid input data is located based on log state
    if (src->log == IMAGE_DATA_PIXELS) {
        // Valid data is in pixels buffer - read as uint8_t
        uint8_t *pixels = (uint8_t *)src->pixels;

        for (uint32_t y = 0; y < src->height; ++y) {
            for (uint32_t x = 0; x < src->width; ++x) {
                uint8_t maxPixelValue = 0;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky) {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx) {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < (int)src->width && offsetY >= 0 &&
                            offsetY < (int)src->height) {
                            uint8_t pixelValue = pixels[offsetY * src->width + offsetX];
                            if (pixelValue > maxPixelValue) {
                                maxPixelValue = pixelValue;
                            }
                        }
                    }
                }

                dst->chals->ch[0][y * src->width + x] = (float)maxPixelValue;
            }
        }
    } else {
        // Valid data is in chals - determine which channel based on log
        if (isChalsEmpty(src)) {
            createChals((Image *)src, src->depth);
        }

        // Map log state to channel index
        int ch_idx = 0;
        if (src->log >= IMAGE_DATA_CH0 && src->log <= IMAGE_DATA_CH5) {
            ch_idx = src->log - IMAGE_DATA_CH0;
        } else if (src->log == IMAGE_DATA_MAGNITUDE || src->log == IMAGE_DATA_PHASE) {
            ch_idx = 0;
        }

        float *inCh = src->chals->ch[ch_idx];

        for (uint32_t y = 0; y < src->height; ++y) {
            for (uint32_t x = 0; x < src->width; ++x) {
                float maxPixelValue = -FLT_MAX;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky) {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx) {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < (int)src->width && offsetY >= 0 &&
                            offsetY < (int)src->height) {
                            float pixelValue = inCh[offsetY * src->width + offsetX];
                            if (pixelValue > maxPixelValue) {
                                maxPixelValue = pixelValue;
                            }
                        }
                    }
                }

                dst->chals->ch[0][y * src->width + x] = maxPixelValue;
            }
        }
    }

    dst->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

#include <stdlib.h>

/**
 * @brief Applies a median filter (non-linear) to the image using a square window.
 *
 * @param src Input image.
 * @param dst Output image after median filtering.
 * @param kernel_size Size of the square window (must be odd).
 */
embeddip_status_t medianFilter(const Image *src, Image *dst, int kernel_size)
{
    if (!src || !dst) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (kernel_size < 1 || (kernel_size % 2) == 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    int kernelRadius = kernel_size / 2;
    int windowArea = kernel_size * kernel_size;

    // Ensure output has channels allocated
    if (isChalsEmpty(dst)) {
        createChals(dst, dst->depth);
        dst->is_chals = 1;
    }

    // Temporary buffer to store window values
    float *window = (float *)memory_alloc(sizeof(float) * windowArea);

    // Check where the valid input data is located based on log state
    if (src->log == IMAGE_DATA_PIXELS) {
        // Valid data is in pixels buffer - read as uint8_t
        uint8_t *pixels = (uint8_t *)src->pixels;

        for (uint32_t y = 0; y < src->height; ++y) {
            for (uint32_t x = 0; x < src->width; ++x) {
                int count = 0;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky) {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx) {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < (int)src->width && offsetY >= 0 &&
                            offsetY < (int)src->height) {
                            window[count++] = (float)pixels[offsetY * src->width + offsetX];
                        }
                    }
                }

                // Sort the window and take the median
                for (int i = 0; i < count - 1; ++i) {
                    for (int j = i + 1; j < count; ++j) {
                        if (window[i] > window[j]) {
                            float tmp = window[i];
                            window[i] = window[j];
                            window[j] = tmp;
                        }
                    }
                }

                float median;
                if (count % 2 == 1)
                    median = window[count / 2];
                else
                    median = (window[count / 2 - 1] + window[count / 2]) / 2.0f;

                dst->chals->ch[0][y * src->width + x] = median;
            }
        }
    } else {
        // Valid data is in chals - determine which channel based on log
        if (isChalsEmpty(src)) {
            createChals((Image *)src, src->depth);
        }

        // Map log state to channel index
        int ch_idx = 0;
        if (src->log >= IMAGE_DATA_CH0 && src->log <= IMAGE_DATA_CH5) {
            ch_idx = src->log - IMAGE_DATA_CH0;
        } else if (src->log == IMAGE_DATA_MAGNITUDE || src->log == IMAGE_DATA_PHASE) {
            ch_idx = 0;
        }

        float *inCh = src->chals->ch[ch_idx];

        for (uint32_t y = 0; y < src->height; ++y) {
            for (uint32_t x = 0; x < src->width; ++x) {
                int count = 0;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky) {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx) {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < (int)src->width && offsetY >= 0 &&
                            offsetY < (int)src->height) {
                            window[count++] = inCh[offsetY * src->width + offsetX];
                        }
                    }
                }

                // Sort the window and take the median
                for (int i = 0; i < count - 1; ++i) {
                    for (int j = i + 1; j < count; ++j) {
                        if (window[i] > window[j]) {
                            float tmp = window[i];
                            window[i] = window[j];
                            window[j] = tmp;
                        }
                    }
                }

                float median;
                if (count % 2 == 1)
                    median = window[count / 2];
                else
                    median = (window[count / 2 - 1] + window[count / 2]) / 2.0f;

                dst->chals->ch[0][y * src->width + x] = median;
            }
        }
    }

    memory_free(window);

    dst->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

embeddip_status_t rgbSplit(const Image *src, Image *r_img, Image *g_img, Image *b_img)
/**
 * @brief Splits an RGB888 image into R, G, and B bands.
 *
 * @param[in]  src  Input image in RGB format (IMAGE_FORMAT_RGB888).
 * @param[out] r_img   Output red band (grayscale format).
 * @param[out] g_img   Output green band.
 * @param[out] b_img   Output blue band.
 */
{
    CHECK_NULL_INT(src);
    CHECK_NULL_INT(r_img);
    CHECK_NULL_INT(g_img);
    CHECK_NULL_INT(b_img);
    CHECK_FORMAT_INT(src, IMAGE_FORMAT_RGB888);
    CHECK_FORMAT_INT(r_img, IMAGE_FORMAT_GRAYSCALE);
    CHECK_FORMAT_INT(g_img, IMAGE_FORMAT_GRAYSCALE);
    CHECK_FORMAT_INT(b_img, IMAGE_FORMAT_GRAYSCALE);

    uint8_t *r = (uint8_t *)r_img->pixels;
    uint8_t *g = (uint8_t *)g_img->pixels;
    uint8_t *b = (uint8_t *)b_img->pixels;

    // Check where the valid input data is located based on log state
    if (src->log == IMAGE_DATA_PIXELS) {
        // Valid data is in pixels buffer - RGB888 interleaved
        const uint8_t *in = (const uint8_t *)src->pixels;

        for (uint32_t i = 0; i < src->size; ++i) {
            r[i] = in[i * 3 + 0];
            g[i] = in[i * 3 + 1];
            b[i] = in[i * 3 + 2];
        }
    } else {
        // Valid data is in chals - RGB in separate channels (ch[1]=R, ch[2]=G, ch[3]=B)
        if (isChalsEmpty(src)) {
            createChals((Image *)src, src->depth);
        }

        float *rCh = src->chals->ch[1];
        float *gCh = src->chals->ch[2];
        float *bCh = src->chals->ch[3];

        for (uint32_t i = 0; i < src->size; ++i) {
            r[i] = (uint8_t)(rCh[i] > 255.0f ? 255 : (rCh[i] < 0.0f ? 0 : rCh[i]));
            g[i] = (uint8_t)(gCh[i] > 255.0f ? 255 : (gCh[i] < 0.0f ? 0 : gCh[i]));
            b[i] = (uint8_t)(bCh[i] > 255.0f ? 255 : (bCh[i] < 0.0f ? 0 : bCh[i]));
        }
    }

    r_img->log = IMAGE_DATA_PIXELS;
    g_img->log = IMAGE_DATA_PIXELS;
    b_img->log = IMAGE_DATA_PIXELS;

    return EMBEDDIP_OK;
}

embeddip_status_t rgbMerge(const Image *r_img, const Image *g_img, const Image *b_img, Image *dst)
/**
 * @brief Merges three grayscale bands into a single RGB888 image.
 *
 * @param[in]  r_img    Red band.
 * @param[in]  g_img    Green band.
 * @param[in]  b_img    Blue band.
 * @param[out] dst  Output image in RGB format.
 */
{
    CHECK_NULL_INT(r_img);
    CHECK_NULL_INT(g_img);
    CHECK_NULL_INT(b_img);
    CHECK_NULL_INT(dst);
    CHECK_FORMAT_INT(dst, IMAGE_FORMAT_RGB888);
    CHECK_FORMAT_INT(r_img, IMAGE_FORMAT_GRAYSCALE);
    CHECK_FORMAT_INT(g_img, IMAGE_FORMAT_GRAYSCALE);
    CHECK_FORMAT_INT(b_img, IMAGE_FORMAT_GRAYSCALE);

    uint8_t *out = (uint8_t *)dst->pixels;

    // Check where the valid input data is located based on log state
    // All three inputs should have the same state (pixels or channels)
    if (r_img->log == IMAGE_DATA_PIXELS) {
        // Valid data is in pixels buffer
        const uint8_t *r = (const uint8_t *)r_img->pixels;
        const uint8_t *g = (const uint8_t *)g_img->pixels;
        const uint8_t *b = (const uint8_t *)b_img->pixels;

        for (uint32_t i = 0; i < dst->size; ++i) {
            out[i * 3 + 0] = r[i];
            out[i * 3 + 1] = g[i];
            out[i * 3 + 2] = b[i];
        }
    } else {
        // Valid data is in chals - grayscale in ch[0]
        if (isChalsEmpty(r_img)) {
            createChals((Image *)r_img, r_img->depth);
        }
        if (isChalsEmpty(g_img)) {
            createChals((Image *)g_img, g_img->depth);
        }
        if (isChalsEmpty(b_img)) {
            createChals((Image *)b_img, b_img->depth);
        }

        // Map log state to channel index for each input
        int r_ch_idx = 0;
        if (r_img->log >= IMAGE_DATA_CH0 && r_img->log <= IMAGE_DATA_CH5) {
            r_ch_idx = r_img->log - IMAGE_DATA_CH0;
        }

        int g_ch_idx = 0;
        if (g_img->log >= IMAGE_DATA_CH0 && g_img->log <= IMAGE_DATA_CH5) {
            g_ch_idx = g_img->log - IMAGE_DATA_CH0;
        }

        int b_ch_idx = 0;
        if (b_img->log >= IMAGE_DATA_CH0 && b_img->log <= IMAGE_DATA_CH5) {
            b_ch_idx = b_img->log - IMAGE_DATA_CH0;
        }

        float *rCh = r_img->chals->ch[r_ch_idx];
        float *gCh = g_img->chals->ch[g_ch_idx];
        float *bCh = b_img->chals->ch[b_ch_idx];

        for (uint32_t i = 0; i < dst->size; ++i) {
            out[i * 3 + 0] = (uint8_t)(rCh[i] > 255.0f ? 255 : (rCh[i] < 0.0f ? 0 : rCh[i]));
            out[i * 3 + 1] = (uint8_t)(gCh[i] > 255.0f ? 255 : (gCh[i] < 0.0f ? 0 : gCh[i]));
            out[i * 3 + 2] = (uint8_t)(bCh[i] > 255.0f ? 255 : (bCh[i] < 0.0f ? 0 : bCh[i]));
        }
    }

    dst->log = IMAGE_DATA_PIXELS;
    return EMBEDDIP_OK;
}

embeddip_status_t dogFilter(const Image *src, Image *dst, float sigma1, float sigma2)
{
    if (!src || !dst) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (src->format != IMAGE_FORMAT_GRAYSCALE) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    int width = src->width;
    int height = src->height;
    int size1 = (int)(6 * sigma1 + 1) | 1;  // force odd size
    int size2 = (int)(6 * sigma2 + 1) | 1;
    int half1 = size1 / 2;
    int half2 = size2 / 2;

    // Allocate Gaussian kernels
    float *kernel1 = (float *)memory_alloc(size1 * size1 * sizeof(float));
    float *kernel2 = (float *)memory_alloc(size2 * size2 * sizeof(float));

    float sum1 = 0.0f, sum2 = 0.0f;

    // Fill Gaussian kernel1
    for (int y = -half1; y <= half1; ++y) {
        for (int x = -half1; x <= half1; ++x) {
            float val = expf(-(x * x + y * y) / (2.0f * sigma1 * sigma1));
            kernel1[(y + half1) * size1 + (x + half1)] = val;
            sum1 += val;
        }
    }

    // Normalize kernel1
    for (int i = 0; i < size1 * size1; ++i)
        kernel1[i] /= sum1;

    // Fill Gaussian kernel2
    for (int y = -half2; y <= half2; ++y) {
        for (int x = -half2; x <= half2; ++x) {
            float val = expf(-(x * x + y * y) / (2.0f * sigma2 * sigma2));
            kernel2[(y + half2) * size2 + (x + half2)] = val;
            sum2 += val;
        }
    }

    // Normalize kernel2
    for (int i = 0; i < size2 * size2; ++i)
        kernel2[i] /= sum2;

    // Allocate intermediate float buffers
    float *src_buf = (float *)memory_alloc(width * height * sizeof(float));
    float *blur1 = (float *)memory_alloc(width * height * sizeof(float));
    float *blur2 = (float *)memory_alloc(width * height * sizeof(float));

    // Convert input pixels to float
    const uint8_t *raw = (const uint8_t *)src->pixels;
    for (int i = 0; i < width * height; ++i)
        src_buf[i] = (float)raw[i];

    // Convolve with kernel1 (sigma1)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float sum = 0.0f;
            for (int fy = -half1; fy <= half1; ++fy) {
                for (int fx = -half1; fx <= half1; ++fx) {
                    int iy = clamp_index(y + fy, height);
                    int ix = clamp_index(x + fx, width);
                    sum += src_buf[iy * width + ix] * kernel1[(fy + half1) * size1 + (fx + half1)];
                }
            }
            blur1[y * width + x] = sum;
        }
    }

    // Convolve with kernel2 (sigma2)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float sum = 0.0f;
            for (int fy = -half2; fy <= half2; ++fy) {
                for (int fx = -half2; fx <= half2; ++fx) {
                    int iy = clamp_index(y + fy, height);
                    int ix = clamp_index(x + fx, width);
                    sum += src_buf[iy * width + ix] * kernel2[(fy + half2) * size2 + (fx + half2)];
                }
            }
            blur2[y * width + x] = sum;
        }
    }

    // Allocate output float channel
    if (!dst->chals) {
        dst->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(dst->chals, 0, sizeof(channels_t));
    }
    dst->chals->ch[0] = (float *)memory_alloc(width * height * sizeof(float));
    dst->is_chals |= channel_mask[0];
    float *outCh = dst->chals->ch[0];

    // Compute DoG = |blur1 - blur2|
    for (int i = 0; i < width * height; ++i)
        outCh[i] = fabsf(blur1[i] - blur2[i]);

    // Free temporary resources
    memory_free(kernel1);
    memory_free(kernel2);
    memory_free(src_buf);
    memory_free(blur1);
    memory_free(blur2);

    dst->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

#ifndef M_PI
    #define M_PI 3.14159265358979323846264338327950288
#endif

/**
 * @brief Applies Laplacian of Gaussian (LoG) filtering to a grayscale image.
 *
 * @param[in]  src   Pointer to grayscale input image.
 * @param[out] dst  Pointer to output image (float convolution result).
 * @param[in]  sigma   Standard deviation of the Gaussian (controls smoothness).
 */
embeddip_status_t logFilter(const Image *src, Image *dst, float sigma)
{
    // Validate input pointers
    CHECK_NULL_INT(src);
    CHECK_NULL_INT(dst);
    CHECK_NULL_INT(src->pixels);

    // Validate format and dimensions
    CHECK_FORMAT_INT(src, IMAGE_FORMAT_GRAYSCALE);
    CHECK_CONDITION_INT(src->width > 0 && src->height > 0);
    CHECK_CONDITION_INT(dst->width == src->width && dst->height == src->height);
    CHECK_CONDITION_INT(sigma > 0.0f);

    int width = src->width;
    int height = src->height;
    int ksize = ((int)(6 * sigma + 1)) | 1;  // force odd
    int half = ksize / 2;
    float s2 = sigma * sigma;
    float s4 = s2 * s2;

    // Allocate LoG kernel
    float *kernel = (float *)memory_alloc(ksize * ksize * sizeof(float));
    if (!kernel)
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    float sum = 0.0f;
    for (int y = -half; y <= half; ++y) {
        for (int x = -half; x <= half; ++x) {
            float r2 = x * x + y * y;
            float norm = (r2 - 2 * s2) / (2 * M_PI * s4);
            float gauss = expf(-r2 / (2 * s2));
            float value = norm * gauss;
            kernel[(y + half) * ksize + (x + half)] = value;
            sum += value;
        }
    }

    // Normalize to zero sum
    float avg = sum / (ksize * ksize);
    for (int i = 0; i < ksize * ksize; ++i)
        kernel[i] -= avg;

    // Allocate float buffer and convert input
    float *src_buf = (float *)memory_alloc(width * height * sizeof(float));
    if (!src_buf) {
        memory_free(kernel);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    const uint8_t *raw = (const uint8_t *)src->pixels;
    for (int i = 0; i < width * height; ++i)
        src_buf[i] = (float)raw[i];

    // Allocate output float buffer
    if (!dst->chals) {
        dst->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        if (!dst->chals) {
            memory_free(kernel);
            memory_free(src_buf);
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
        memset(dst->chals, 0, sizeof(channels_t));
    }

    dst->chals->ch[0] = (float *)memory_alloc(width * height * sizeof(float));
    if (!dst->chals->ch[0]) {
        memory_free(kernel);
        memory_free(src_buf);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    dst->is_chals |= channel_mask[0];
    float *outCh = dst->chals->ch[0];

    // Convolution with LoG kernel
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float sum = 0.0f;
            for (int fy = -half; fy <= half; ++fy) {
                for (int fx = -half; fx <= half; ++fx) {
                    int iy = clamp_index(y + fy, height);
                    int ix = clamp_index(x + fx, width);
                    sum += src_buf[iy * width + ix] * kernel[(fy + half) * ksize + (fx + half)];
                }
            }
            outCh[y * width + x] = sum;
        }
    }

    memory_free(kernel);
    dst->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/**
 * @brief Non-maximum suppression on gradient magnitude using gradient phase.
 *
 * @param[in]  magImg   Gradient magnitude (float ch0).
 * @param[in]  phaseImg Gradient phase (float ch0, radians).
 * @param[out] dst   Suppressed output (float ch0).
 */
void nonMaximumSuppression(const Image *magImg, const Image *phaseImg, Image *dst)
{
    if (!magImg || !phaseImg || !dst)
        return;
    uint32_t w = magImg->width, h = magImg->height;
    if (w != phaseImg->width || h != phaseImg->height)
        return;
    if (!magImg->chals || !magImg->chals->ch[0] || !phaseImg->chals || !phaseImg->chals->ch[0])
        return;
    uint32_t N = w * h;

    const float *mag = magImg->chals->ch[0];
    const float *phase = phaseImg->chals->ch[0];

    if (!dst->chals) {
        dst->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        if (!dst->chals)
            return;
        memset(dst->chals, 0, sizeof(channels_t));
    }
    dst->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    if (!dst->chals->ch[0])
        return;
    dst->is_chals = 1;
    float *dst_data = dst->chals->ch[0];

    // Initialize all to zero (including borders)
    memset(dst_data, 0, (size_t)N * sizeof(float));

    // Iterate, skip borders
    for (uint32_t y = 1; y < h - 1; y++) {
        for (uint32_t x = 1; x < w - 1; x++) {
            uint32_t idx = y * w + x;
            float angle = phase[idx] * 180.0f / (float)M_PI;
            if (angle < 0)
                angle += 180.0f;

            float m = mag[idx];
            float m1 = 0, m2 = 0;

            if ((angle >= 0 && angle < 22.5) || (angle >= 157.5 && angle <= 180)) {
                m1 = mag[idx - 1];
                m2 = mag[idx + 1];  // left-right
            } else if (angle >= 22.5 && angle < 67.5) {
                m1 = mag[idx - w - 1];
                m2 = mag[idx + w + 1];  // diag ↘
            } else if (angle >= 67.5 && angle < 112.5) {
                m1 = mag[idx - w];
                m2 = mag[idx + w];  // up-down
            } else if (angle >= 112.5 && angle < 157.5) {
                m1 = mag[idx - w + 1];
                m2 = mag[idx + w - 1];  // diag ↙
            }

            dst_data[idx] = (m >= m1 && m >= m2) ? m : 0.0f;
        }
    }

    dst->log = IMAGE_DATA_CH0;
}

#define STRONG 255
#define WEAK 50

/**
 * @brief Apply double thresholding to classify strong/weak edges.
 * Writes to float ch0 (values: 0.0f, weakVal, strongVal).
 */
void doubleThreshold(const Image *src,
                     Image *dst,
                     float lowThresh,
                     float highThresh,
                     float weakVal,
                     float strongVal)
{
    if (!src || !dst)
        return;
    if (!src->chals || !src->chals->ch[0])
        return;
    uint32_t N = src->width * src->height;
    const float *src_data = src->chals->ch[0];
    if (lowThresh > highThresh) {
        float tmp = lowThresh;
        lowThresh = highThresh;
        highThresh = tmp;
    }

    if (!dst->chals) {
        dst->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        if (!dst->chals)
            return;
        memset(dst->chals, 0, sizeof(channels_t));
    }
    dst->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    if (!dst->chals->ch[0])
        return;
    dst->is_chals = 1;
    float *dst_data = dst->chals->ch[0];

    for (uint32_t i = 0; i < N; i++) {
        if (src_data[i] >= highThresh)
            dst_data[i] = strongVal;
        else if (src_data[i] >= lowThresh)
            dst_data[i] = weakVal;
        else
            dst_data[i] = 0.0f;
    }

    dst->log = IMAGE_DATA_CH0;
}

/**
 * @brief Edge tracking by hysteresis.
 * Promotes weak edges connected to strong edges.
 */
void hysteresis(const Image *src, Image *dst, float weakVal, float strongVal)
{
    if (!src || !dst)
        return;
    uint32_t w = src->width, h = src->height;
    uint32_t N = w * h;
    if (!src->chals || !src->chals->ch[0] || N == 0)
        return;
    const float *src_data = src->chals->ch[0];

    if (!dst->chals) {
        dst->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        if (!dst->chals)
            return;
        memset(dst->chals, 0, sizeof(channels_t));
    }
    dst->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    if (!dst->chals->ch[0])
        return;
    dst->is_chals = 1;
    float *dst_data = dst->chals->ch[0];
    memset(dst_data, 0, (size_t)N * sizeof(float));

    int *stack = (int *)memory_alloc((size_t)N * sizeof(int));
    if (!stack)
        return;
    uint32_t sp = 0;

    for (uint32_t i = 0; i < N; ++i) {
        if (src_data[i] == strongVal) {
            dst_data[i] = strongVal;
            stack[sp++] = (int)i;
        }
    }

    while (sp > 0) {
        int idx = stack[--sp];
        uint32_t x = (uint32_t)idx % w;
        uint32_t y = (uint32_t)idx / w;

        int y0 = (y > 0) ? (int)y - 1 : 0;
        int y1 = (y + 1 < h) ? (int)y + 1 : (int)h - 1;
        int x0 = (x > 0) ? (int)x - 1 : 0;
        int x1 = (x + 1 < w) ? (int)x + 1 : (int)w - 1;

        for (int ny = y0; ny <= y1; ++ny) {
            for (int nx = x0; nx <= x1; ++nx) {
                uint32_t nidx = (uint32_t)ny * w + (uint32_t)nx;
                if (dst_data[nidx] == strongVal)
                    continue;
                if (src_data[nidx] == weakVal) {
                    dst_data[nidx] = strongVal;
                    stack[sp++] = (int)nidx;
                }
            }
        }
    }

    memory_free(stack);
    dst->log = IMAGE_DATA_CH0;
}

static void make_gaussian_and_dgauss_1d(float sigma, float **G, float **dG, int *ksize)
{
    int k = ((int)(6.f * sigma + 1.f)) | 1;  // force odd
    int r = k >> 1;

    float *g = (float *)memory_alloc((size_t)k * sizeof(float));
    float *dg = (float *)memory_alloc((size_t)k * sizeof(float));

    float s2 = sigma * sigma;
    float norm = 1.f / (sqrtf(2.f * (float)M_PI) * sigma);

    float sum_g = 0.f, sum_dg = 0.f;

    for (int i = -r; i <= r; ++i) {
        float x = (float)i;
        float gx = norm * expf(-(x * x) / (2.f * s2));
        float dgx = -(x / s2) * gx;  // d/dx G(x) = -(x/sigma^2) G(x)
        g[i + r] = gx;
        dg[i + r] = dgx;
        sum_g += gx;
        sum_dg += dgx;
    }

    // normalize G to sum=1; remove tiny bias in dG so sum≈0 exactly
    for (int i = 0; i < k; ++i)
        g[i] /= (sum_g > 0.f ? sum_g : 1.f);
    float bias = sum_dg / (float)k;
    for (int i = 0; i < k; ++i)
        dg[i] -= bias;

    *G = g;
    *dG = dg;
    *ksize = k;
}

static void sep_conv_xy_f32(const float *src,
                            float *dst,
                            int width,
                            int height,
                            const float *kx,
                            int kx_sz,
                            const float *ky,
                            int ky_sz)
{
    int rx = kx_sz >> 1, ry = ky_sz >> 1;
    float *tmp = (float *)memory_alloc((size_t)width * height * sizeof(float));

    // X
    for (int y = 0; y < height; ++y) {
        const float *row = src + y * width;
        float *trow = tmp + y * width;
        for (int x = 0; x < width; ++x) {
            float acc = 0.f;
            for (int i = -rx; i <= rx; ++i) {
                int xx = clamp_index(x + i, width);
                acc += row[xx] * kx[i + rx];
            }
            trow[x] = acc;
        }
    }

    // Y
    for (int y = 0; y < height; ++y) {
        float *drow = dst + y * width;
        for (int x = 0; x < width; ++x) {
            float acc = 0.f;
            for (int j = -ry; j <= ry; ++j) {
                int yy = clamp_index(y + j, height);
                acc += tmp[yy * width + x] * ky[j + ry];
            }
            drow[x] = acc;
        }
    }

    memory_free(tmp);
}

/**
 * @brief Compute Gaussian-smoothed image gradients (Ix, Iy) using ∂G/∂x, ∂G/∂y.
 *
 * @param[in]  src    Pointer to grayscale input image.
 * @param[out] outIx  Pointer to output image for x-gradient (float ch0).
 * @param[out] outIy  Pointer to output image for y-gradient (float ch0).
 * @param[in]  sigma  Standard deviation of the Gaussian kernel.
 */
embeddip_status_t gaussianGradients(const Image *src, Image *outIx, Image *outIy, float sigma)
{
    // Validate input pointers
    CHECK_NULL_INT(src);
    CHECK_NULL_INT(outIx);
    CHECK_NULL_INT(outIy);
    CHECK_NULL_INT(src->pixels);

    // Validate format and dimensions
    CHECK_FORMAT_INT(src, IMAGE_FORMAT_GRAYSCALE);
    CHECK_CONDITION_INT(src->width > 0 && src->height > 0);
    CHECK_CONDITION_INT(outIx->width == src->width && outIx->height == src->height);
    CHECK_CONDITION_INT(outIy->width == src->width && outIy->height == src->height);
    CHECK_CONDITION_INT(sigma > 0.0f);

    int width = src->width, height = src->height;
    int N = width * height;

    // input to float buffer
    const uint8_t *raw = (const uint8_t *)src->pixels;
    float *src_buf = (float *)memory_alloc((size_t)N * sizeof(float));
    if (!src_buf)
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    for (int i = 0; i < N; ++i)
        src_buf[i] = (float)raw[i];

    // kernels
    float *G = NULL, *dG = NULL;
    int ksz = 0;
    make_gaussian_and_dgauss_1d(sigma, &G, &dG, &ksz);
    if (!G || !dG) {
        memory_free(src_buf);
        memory_free(G);
        memory_free(dG);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    // prepare outputs (float channels)
    if (!outIx->chals) {
        outIx->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        if (!outIx->chals) {
            memory_free(G);
            memory_free(dG);
            memory_free(src_buf);
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
        memset(outIx->chals, 0, sizeof(channels_t));
    }
    if (!outIy->chals) {
        outIy->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        if (!outIy->chals) {
            memory_free(G);
            memory_free(dG);
            memory_free(src_buf);
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
        memset(outIy->chals, 0, sizeof(channels_t));
    }

    outIx->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    if (!outIx->chals->ch[0]) {
        memory_free(G);
        memory_free(dG);
        memory_free(src_buf);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    outIy->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    if (!outIy->chals->ch[0]) {
        memory_free(outIx->chals->ch[0]);
        memory_free(G);
        memory_free(dG);
        memory_free(src_buf);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    outIx->is_chals = 1;
    outIy->is_chals = 1;
    float *ix = outIx->chals->ch[0];
    float *iy = outIy->chals->ch[0];

    // Ix = (I * dG_x) * G_y
    sep_conv_xy_f32(src_buf, ix, width, height, dG, ksz, G, ksz);
    // Iy = (I * G_x) * dG_y
    sep_conv_xy_f32(src_buf, iy, width, height, G, ksz, dG, ksz);

    memory_free(G);
    memory_free(dG);
    memory_free(src_buf);

    // optional: mark data origin/type if you track it (similar to dst->log = IMAGE_DATA_CH0)
    outIx->log = IMAGE_DATA_CH0;  // reuse your flag; or define IMAGE_DATA_DX if you prefer
    outIy->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/**
 * @brief Compute gradient magnitude sqrt(Ix^2 + Iy^2) from float ch0 of Ix, Iy.
 * Writes float magnitude to outMag->chals->ch[0].
 */
embeddip_status_t gradientMagnitude(const Image *IxImg, const Image *IyImg, Image *outMag)
{
    // Validate input pointers
    CHECK_NULL_INT(IxImg);
    CHECK_NULL_INT(IyImg);
    CHECK_NULL_INT(outMag);

    // Validate dimensions
    CHECK_CONDITION_INT(IxImg->width > 0 && IxImg->height > 0);
    CHECK_CONDITION_INT(IxImg->width == IyImg->width && IxImg->height == IyImg->height);
    CHECK_CONDITION_INT(outMag->width == IxImg->width && outMag->height == IxImg->height);

    uint32_t width = IxImg->width, height = IxImg->height;
    uint32_t N = width * height;

    // Validate channel data exists
    const float *ix = IxImg->chals ? IxImg->chals->ch[0] : NULL;
    const float *iy = IyImg->chals ? IyImg->chals->ch[0] : NULL;
    CHECK_NULL_INT(ix);
    CHECK_NULL_INT(iy);

    // Allocate output channel
    if (!outMag->chals) {
        outMag->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        if (!outMag->chals)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        memset(outMag->chals, 0, sizeof(channels_t));
    }

    outMag->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    if (!outMag->chals->ch[0])
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    outMag->is_chals = 1;
    float *mag = outMag->chals->ch[0];

    // Compute magnitude
    for (uint32_t i = 0; i < N; ++i) {
        float gx = ix[i], gy = iy[i];
        mag[i] = sqrtf(gx * gx + gy * gy);
    }

    outMag->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/**
 * @brief Compute gradient phase atan2(Iy, Ix) from float ch0 of Ix, Iy.
 * Writes float phase (in radians) to outPhase->chals->ch[0].
 * Range: [-π, π].
 */
embeddip_status_t gradientPhase(const Image *IxImg, const Image *IyImg, Image *outPhase)
{
    // Validate input pointers
    CHECK_NULL_INT(IxImg);
    CHECK_NULL_INT(IyImg);
    CHECK_NULL_INT(outPhase);

    // Validate dimensions (fix bug: was using IyImg->height for width)
    CHECK_CONDITION_INT(IxImg->width > 0 && IxImg->height > 0);
    CHECK_CONDITION_INT(IxImg->width == IyImg->width && IxImg->height == IyImg->height);
    CHECK_CONDITION_INT(outPhase->width == IxImg->width && outPhase->height == IxImg->height);

    uint32_t width = IxImg->width, height = IxImg->height;
    uint32_t N = width * height;

    // Validate channel data exists
    const float *ix = IxImg->chals ? IxImg->chals->ch[0] : NULL;
    const float *iy = IyImg->chals ? IyImg->chals->ch[0] : NULL;
    CHECK_NULL_INT(ix);
    CHECK_NULL_INT(iy);

    // Allocate output channel
    if (!outPhase->chals) {
        outPhase->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        if (!outPhase->chals)
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        memset(outPhase->chals, 0, sizeof(channels_t));
    }

    outPhase->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    if (!outPhase->chals->ch[0])
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    outPhase->is_chals = 1;
    float *phase = outPhase->chals->ch[0];

    // Compute phase
    for (uint32_t i = 0; i < N; ++i) {
        float gx = ix[i], gy = iy[i];
        phase[i] = atan2f(gy, gx);  // radians, [-π, π]
    }

    outPhase->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

embeddip_status_t Canny(const Image *src,
                        Image *dst,
                        double threshold1,
                        double threshold2,
                        int aperture_size,
                        bool l2_gradient)
{
    CHECK_NULL_INT(src);
    CHECK_NULL_INT(dst);

    // --- Step 1: Gaussian smoothing + gradients ---
    int k = (aperture_size < 3) ? 3 : aperture_size;
    if ((k & 1) == 0)
        ++k;
    if (k > 7)
        k = 7;
    float sigma = 0.3f * ((float)(k - 1) * 0.5f - 1.0f) + 0.8f;

    Image *Ix = createImageWH_legacy(src->width, src->height, src->format);
    Image *Iy = createImageWH_legacy(src->width, src->height, src->format);
    if (!Ix || !Iy) {
        deleteImage(Ix);
        deleteImage(Iy);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    embeddip_status_t st = gaussianGradients(src, Ix, Iy, sigma);
    if (st != EMBEDDIP_OK) {
        deleteImage(Ix);
        deleteImage(Iy);
        return st;
    }

    // --- Step 2: magnitude + phase ---
    Image *Mag = createImageWH_legacy(src->width, src->height, src->format);
    Image *Phase = createImageWH_legacy(src->width, src->height, src->format);
    if (!Mag || !Phase) {
        deleteImage(Ix);
        deleteImage(Iy);
        deleteImage(Mag);
        deleteImage(Phase);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }

    if (l2_gradient) {
        st = gradientMagnitude(Ix, Iy, Mag);
    } else {
        uint32_t N = src->size;
        if (!Mag->chals) {
            Mag->chals = (channels_t *)memory_alloc(sizeof(channels_t));
            if (!Mag->chals) {
                deleteImage(Ix);
                deleteImage(Iy);
                deleteImage(Mag);
                deleteImage(Phase);
                return EMBEDDIP_ERROR_OUT_OF_MEMORY;
            }
            memset(Mag->chals, 0, sizeof(channels_t));
        }
        Mag->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
        if (!Mag->chals->ch[0]) {
            deleteImage(Ix);
            deleteImage(Iy);
            deleteImage(Mag);
            deleteImage(Phase);
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
        const float *ix = Ix->chals ? Ix->chals->ch[0] : NULL;
        const float *iy = Iy->chals ? Iy->chals->ch[0] : NULL;
        if (!ix || !iy) {
            deleteImage(Ix);
            deleteImage(Iy);
            deleteImage(Mag);
            deleteImage(Phase);
            return EMBEDDIP_ERROR_INVALID_ARG;
        }
        for (uint32_t i = 0; i < N; ++i) {
            Mag->chals->ch[0][i] = fabsf(ix[i]) + fabsf(iy[i]);
        }
        Mag->is_chals = 1;
        Mag->log = IMAGE_DATA_CH0;
        st = EMBEDDIP_OK;
    }
    if (st != EMBEDDIP_OK) {
        deleteImage(Ix);
        deleteImage(Iy);
        deleteImage(Mag);
        deleteImage(Phase);
        return st;
    }
    st = gradientPhase(Ix, Iy, Phase);
    if (st != EMBEDDIP_OK) {
        deleteImage(Ix);
        deleteImage(Iy);
        deleteImage(Mag);
        deleteImage(Phase);
        return st;
    }

    float *data = Mag->chals->ch[0];

    float min = FLT_MAX, max = -FLT_MAX;

    // Step 1: Find min and max
    for (uint32_t i = 0; i < src->size; i++) {
        float v = Mag->chals->ch[0][i];
        if (v < min)
            min = v;
        if (v > max)
            max = v;
    }

    // Step 2: Normalize to [0, 255] and clamp
    if (max != min) {
        for (uint32_t i = 0; i < Mag->size; i++) {
            float norm = (Mag->chals->ch[0][i] - min) / (max - min);
            data[i] = (float)(norm * 255.0f + 0.5f);  // +0.5 for rounding
        }
    } else {
        // All values are the same, map to 0 or 255
        memset(data, 0, Mag->size * sizeof(float));  // Or use 255
    }

    // --- Step 3: NMS ---
    Image *Nms = createImageWH_legacy(src->width, src->height, src->format);
    if (!Nms) {
        deleteImage(Ix);
        deleteImage(Iy);
        deleteImage(Mag);
        deleteImage(Phase);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    nonMaximumSuppression(Mag, Phase, Nms);

    // --- Step 4: Double threshold ---
    Image *Dt = createImageWH_legacy(src->width, src->height, src->format);
    if (!Dt) {
        deleteImage(Ix);
        deleteImage(Iy);
        deleteImage(Mag);
        deleteImage(Phase);
        deleteImage(Nms);
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;
    }
    doubleThreshold(Nms, Dt, (float)threshold1, (float)threshold2, 50.0f, 255.0f);

    // --- Step 5: Hysteresis ---
    hysteresis(Dt, dst, 50.0f, 255.0f);

    // free temps
    deleteImage(Ix);
    deleteImage(Iy);
    deleteImage(Mag);
    deleteImage(Phase);
    deleteImage(Nms);
    deleteImage(Dt);

    return EMBEDDIP_OK;
}

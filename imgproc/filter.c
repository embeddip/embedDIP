#include "filter.h"
#include "core/error.h"
#include "float.h"
#include <string.h>  /* For memset, memcpy */

// Validation macros to replace assert()
#define CHECK_NULL_INT(ptr) \
    do { if (!(ptr)) return EMBEDDIP_ERROR_NULL_PTR; } while (0)

#define CHECK_FORMAT_INT(img, expected_fmt) \
    do { if ((img)->format != (expected_fmt)) return EMBEDDIP_ERROR_INVALID_FORMAT; } while (0)

#define CHECK_CONDITION_INT(cond) \
    do { if (!(cond)) return EMBEDDIP_ERROR_INVALID_ARG; } while (0)

// Internal helper — filters only one channel
uint8_t channel_mask[] = {
    0xFE,
    0xFD,
    0xFB,
    0xF7,
};

// i am not sure about the kernel.
int filter2D_single_channel(Image *inImg, Image *outImg, int ch_idx, void *ctx)
{
    if (!inImg || !outImg || !ctx) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    Filter2DContext *context = (Filter2DContext *)ctx;
    int size = context->size;
    int k = size / 2;
    float *kernel = context->kernel;

    int width = inImg->width;
    int height = inImg->height;

    float *inCh = NULL;

    if (inImg->chals == NULL)
    {
        inImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        inImg->chals->ch[0] = NULL;
        inImg->chals->ch[1] = NULL;
        inImg->chals->ch[2] = NULL;
        inImg->chals->ch[3] = NULL;
    }

    // Allocate float input if necessary
    if (inImg->is_chals & ~(channel_mask[ch_idx]))
    {
        inCh = inImg->chals->ch[ch_idx];
    }
    else
    {
        inImg->chals->ch[ch_idx] = (float *)memory_alloc(height * width * sizeof(float));
        inCh = inImg->chals->ch[ch_idx];
        inImg->is_chals = inImg->is_chals | ~(channel_mask[ch_idx]);

        if (!inCh) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
        const uint8_t *raw = (const uint8_t *)inImg->pixels;

        if (inImg->format == IMAGE_FORMAT_GRAYSCALE)
        {
            for (int i = 0; i < (int)inImg->size; ++i)
                inCh[i] = (float)raw[i];
        }
        else if (inImg->format == IMAGE_FORMAT_RGB888)
        {
            for (int i = 0; i < (int)inImg->size; ++i)
                inCh[i] = (float)raw[i * 3 + ch_idx - 1]; // ch[1]=R, ch[2]=G, ch[3]=B
        }
    }

    float *outCh = NULL;

    if (outImg->chals == NULL)
    {
        outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        outImg->chals->ch[0] = NULL;
        outImg->chals->ch[1] = NULL;
        outImg->chals->ch[2] = NULL;
        outImg->chals->ch[3] = NULL;
    }

    if (outImg->is_chals & ~(channel_mask[ch_idx]))
    {
        outCh = outImg->chals->ch[ch_idx];
    }
    else
    {
        outImg->chals->ch[ch_idx] = (float *)memory_alloc(height * width * sizeof(float));
        outImg->is_chals = outImg->is_chals | ~(channel_mask[ch_idx]);
        outCh = outImg->chals->ch[ch_idx];

        if (!outCh) {
            return EMBEDDIP_ERROR_OUT_OF_MEMORY;
        }
    }

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float sum = 0.0f;

            for (int fy = -k; fy <= k; ++fy)
            {
                for (int fx = -k; fx <= k; ++fx)
                {
                    int iy = y + fy;
                    int ix = x + fx;

                    float val = 0.0f;
                    if (iy >= 0 && iy < height && ix >= 0 && ix < width)
                        val = inCh[iy * width + ix];

                    sum += val * kernel[(fy + k) * size + (fx + k)];
                }
            }

            outCh[y * width + x] = sum;
        }
    }

    return EMBEDDIP_OK;
}

/**
 * @brief Apply 2D convolution filter to an image
 *
 * This is the C counterpart of ImageWrapper::filter2D(). It applies a 2D
 * convolution kernel to an image, supporting grayscale and RGB888 formats.
 *
 * @param[in]  inImg       Input image (must not be NULL)
 * @param[out] outImg      Output image (must be pre-allocated, same size as input)
 * @param[in]  kernel      Flattened kernel array in row-major order (kernelSize x kernelSize)
 * @param[in]  kernelSize  Size of the square kernel (must be odd: 3, 5, 7, etc.)
 *
 * @return EMBEDDIP_OK on success, error code otherwise
 *
 * @note The kernel is a 1D array representing a 2D kernel in row-major order.
 *       For a 3x3 kernel:
 *       kernel[0..8] = [k00, k01, k02, k10, k11, k12, k20, k21, k22]
 *
 * @note Supported formats:
 *       - IMAGE_FORMAT_GRAYSCALE: Filters single channel
 *       - IMAGE_FORMAT_RGB888: Filters R, G, B channels independently
 *
 * Example:
 * @code
 * // 3x3 Gaussian blur kernel
 * float gaussianKernel[9] = {
 *     1.0f/16, 2.0f/16, 1.0f/16,
 *     2.0f/16, 4.0f/16, 2.0f/16,
 *     1.0f/16, 2.0f/16, 1.0f/16
 * };
 *
 * Image *input = NULL, *output = NULL;
 * createImageWH(640, 480, IMAGE_FORMAT_GRAYSCALE, &input);
 * createImageWH(640, 480, IMAGE_FORMAT_GRAYSCALE, &output);
 *
 * embeddip_status_t status = filter2D(input, output, gaussianKernel, 3);
 * if (status != EMBEDDIP_OK) {
 *     printf("Filter failed: %s\n", embeddip_status_str(status));
 * }
 *
 * deleteImage(input);
 * deleteImage(output);
 * @endcode
 */
int filter2D(const Image *inImg, Image *outImg, const float *kernel, int kernelSize)
{
    // Input validation
    if (!inImg || !outImg || !kernel) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    // Validate kernel size (must be odd and >= 1)
    if (kernelSize < 1 || (kernelSize % 2) == 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    // Validate image dimensions match
    if (inImg->width != outImg->width || inImg->height != outImg->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    // Validate image formats match
    if (inImg->format != outImg->format) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    // Create Filter2DContext
    Filter2DContext context;
    context.size = kernelSize;
    context.kernel = (float *)kernel;  // Cast away const for internal use
    context.chal = 0;  // Not used in this context

    // Dispatch filtering based on format
    int status;
    if (inImg->format == IMAGE_FORMAT_GRAYSCALE) {
        // Filter single grayscale channel (ch_idx = 0)
        status = filter2D_single_channel((Image *)inImg, outImg, 0, &context);
        if (status != EMBEDDIP_OK) {
            return status;
        }
    }
    else if (inImg->format == IMAGE_FORMAT_RGB888) {
        // Filter each RGB channel independently
        status = filter2D_single_channel((Image *)inImg, outImg, 1, &context);  // R channel
        if (status != EMBEDDIP_OK) return status;
        status = filter2D_single_channel((Image *)inImg, outImg, 2, &context);  // G channel
        if (status != EMBEDDIP_OK) return status;
        status = filter2D_single_channel((Image *)inImg, outImg, 3, &context);  // B channel
        if (status != EMBEDDIP_OK) return status;
    }
    else {
        // Unsupported format
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }

    return EMBEDDIP_OK;
}

int filter2D_separable(Image *inImg, Image *outImg, int sizeX, float *kernelX, int sizeY, float *kernelY, float delta)
{
    if (!inImg || !outImg || !kernelX || !kernelY) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (kernelY != kernelX || sizeX != sizeY) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    int half = sizeX / 2;

    int width = inImg->width;
    int height = inImg->height;

    // Prepare input channel
    float *inCh = NULL;

    if (!inImg->chals)
    {
        inImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(inImg->chals, 0, sizeof(channels_t));
    }

    if (inImg->is_chals & ~(channel_mask[0]))
    {
        inCh = inImg->chals->ch[0];
    }
    else
    {
        inCh = (float *)memory_alloc(height * width * sizeof(float));
        inImg->chals->ch[0] = inCh;
        inImg->is_chals |= ~(channel_mask[0]);

        const uint8_t *raw = (const uint8_t *)inImg->pixels;
        if (inImg->format == IMAGE_FORMAT_GRAYSCALE)
        {
            for (int i = 0; i < (int)inImg->size; ++i)
                inCh[i] = (float)raw[i];
        }
        else if (inImg->format == IMAGE_FORMAT_RGB888)
        {
            for (int i = 0; i < (int)inImg->size; ++i)
                inCh[i] = (float)raw[i * 3 + 0 - 1];
        }
    }

    // Temp buffer after vertical pass
    float *temp = (float *)memory_alloc(width * height * sizeof(float));

    // Horizontal kernel output
    float *outCh = NULL;

    if (!outImg->chals)
    {
        outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(outImg->chals, 0, sizeof(channels_t));
    }

    if (outImg->is_chals & ~(channel_mask[0]))
    {
        outCh = outImg->chals->ch[0];
    }
    else
    {
        outCh = (float *)memory_alloc(height * width * sizeof(float));
        outImg->chals->ch[0] = outCh;
        outImg->is_chals |= ~(channel_mask[0]);
    }

    // Vertical pass
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float sum = 0.0f;
            for (int ky = -half; ky <= half; ++ky)
            {
                int iy = y + ky;
                if (iy >= 0 && iy < height)
                    sum += inCh[iy * width + x] * kernelY[ky + half];
            }
            temp[y * width + x] = sum;
        }
    }

    // Horizontal pass
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float sum = 0.0f;
            for (int kx = -half; kx <= half; ++kx)
            {
                int ix = x + kx;
                if (ix >= 0 && ix < width)
                    sum += temp[y * width + ix] * kernelX[kx + half];
            }

            sum = (float)(sum * delta); // Uncomment if delta is a normalization factor

            outCh[y * width + x] = sum;
        }
    }

    return EMBEDDIP_OK;
}

/*
void wrapper(ImageOpFunc func, Image *inImg, Image *outImg, void *context)
{
    assert(func && inImg && outImg);
    assert(inImg->format == outImg->format);

    // Ensure channels are allocated for input
    if (!inImg->is_chals)
    {
        inImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        inImg->is_chals = true;
        for (int i = 0; i < 4; ++i)
            inImg->chals->ch[i] = NULL;
    }

    // Ensure channels are allocated for output
    if (!outImg->is_chals)
    {
        outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t)F);
        outImg->is_chals = true;
        for (int i = 0; i < 4; ++i)
            outImg->chals->ch[i] = NULL;
    }

    // Dispatch per format
    if (inImg->format == IMAGE_FORMAT_GRAYSCALE)
    {
        func(inImg, outImg, 0, context); // l channel
    }
    else if (inImg->format == IMAGE_FORMAT_RGB888)
    {
        for (int ch = 1; ch <= 3; ++ch) // r=1, g=2, b=3
            func(inImg, outImg, ch, context);
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
 * @param inImg Input image.
 * @param outImg Output image after min filtering.
 * @param kernelSize Size of the square window (must be odd).
 */

int minFilter(const Image *inImg, Image *outImg, int kernelSize)
{
    if (!inImg || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (kernelSize < 1 || (kernelSize % 2) == 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    int kernelRadius = kernelSize / 2;

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(inImg))
    {

        for (uint32_t y = 0; y < inImg->height; ++y)
        {
            for (uint32_t x = 0; x < inImg->width; ++x)
            {
                uint8_t minPixelValue = UINT8_MAX;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky)
                {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx)
                    {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < (int)inImg->width &&
                            offsetY >= 0 && offsetY < (int)inImg->height)
                        {
                            uint8_t pixelValue = (uint8_t)((uint8_t *)inImg->pixels)[offsetY * inImg->width + offsetX];
                            if (pixelValue < minPixelValue)
                            {
                                minPixelValue = pixelValue;
                            }
                        }
                    }
                }

                outImg->chals->ch[0][y * inImg->width + x] = minPixelValue;
            }
        }
    }
    else
    {

        for (uint32_t y = 0; y < inImg->height; ++y)
        {
            for (uint32_t x = 0; x < inImg->width; ++x)
            {
                float minPixelValue = FLT_MAX;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky)
                {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx)
                    {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < (int)inImg->width &&
                            offsetY >= 0 && offsetY < (int)inImg->height)
                        {
                            float pixelValue = inImg->chals->ch[0][offsetY * inImg->width + offsetX];
                            if (pixelValue < minPixelValue)
                            {
                                minPixelValue = pixelValue;
                            }
                        }
                    }
                }

                outImg->chals->ch[0][y * inImg->width + x] = minPixelValue;
            }
        }
    }

    return EMBEDDIP_OK;
}

/**
 * @brief Applies a max filter (non-linear) to the image using a square window.
 *
 * @param inImg Input image.
 * @param outImg Output image after max filtering.
 * @param kernelSize Size of the square window (must be odd).
 */

int maxFilter(const Image *inImg, Image *outImg, int kernelSize)
{
    if (!inImg || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (kernelSize < 1 || (kernelSize % 2) == 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    int kernelRadius = kernelSize / 2;

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(inImg))
    {
        for (uint32_t y = 0; y < inImg->height; ++y)
        {
            for (uint32_t x = 0; x < inImg->width; ++x)
            {
                uint8_t maxPixelValue = 0;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky)
                {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx)
                    {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < (int)inImg->width &&
                            offsetY >= 0 && offsetY < (int)inImg->height)
                        {
                            uint8_t pixelValue = (uint8_t)((uint8_t *)inImg->pixels)[offsetY * inImg->width + offsetX];
                            if (pixelValue > maxPixelValue)
                            {
                                maxPixelValue = pixelValue;
                            }
                        }
                    }
                }

                outImg->chals->ch[0][y * inImg->width + x] = maxPixelValue;
            }
        }
    }
    else
    {
        for (uint32_t y = 0; y < inImg->height; ++y)
        {
            for (uint32_t x = 0; x < inImg->width; ++x)
            {
                uint8_t maxPixelValue = 0;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky)
                {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx)
                    {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < (int)inImg->width &&
                            offsetY >= 0 && offsetY < (int)inImg->height)
                        {
                            float pixelValue = (float)inImg->chals->ch[0][offsetY * inImg->width + offsetX];
                            if (pixelValue > maxPixelValue)
                            {
                                maxPixelValue = pixelValue;
                            }
                        }
                    }
                }

                outImg->chals->ch[0][y * inImg->width + x] = maxPixelValue;
            }
        }
    }

    return EMBEDDIP_OK;
}

#include <stdlib.h>

/**
 * @brief Applies a median filter (non-linear) to the image using a square window.
 *
 * @param inImg Input image.
 * @param outImg Output image after median filtering.
 * @param kernelSize Size of the square window (must be odd).
 */
int medianFilter(const Image *inImg, Image *outImg, int kernelSize)
{
    if (!inImg || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (kernelSize < 1 || (kernelSize % 2) == 0) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    int kernelRadius = kernelSize / 2;
    int windowArea = kernelSize * kernelSize;

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    // Temporary buffer to store window values
    float *window = (float *)memory_alloc(sizeof(float) * windowArea);

    for (uint32_t y = 0; y < inImg->height; ++y)
    {
        for (uint32_t x = 0; x < inImg->width; ++x)
        {
            int count = 0;

            for (int ky = -kernelRadius; ky <= kernelRadius; ++ky)
            {
                for (int kx = -kernelRadius; kx <= kernelRadius; ++kx)
                {
                    int offsetX = x + kx;
                    int offsetY = y + ky;

                    if (offsetX >= 0 && offsetX < (int)inImg->width &&
                        offsetY >= 0 && offsetY < (int)inImg->height)
                    {
                        float val;
                        if (isChalsEmpty(inImg))
                        {
                            val = (float)((uint8_t *)inImg->pixels)[offsetY * inImg->width + offsetX];
                        }
                        else
                        {
                            val = inImg->chals->ch[0][offsetY * inImg->width + offsetX];
                        }
                        window[count++] = val;
                    }
                }
            }

            // Sort the window and take the median
            for (int i = 0; i < count - 1; ++i)
            {
                for (int j = i + 1; j < count; ++j)
                {
                    if (window[i] > window[j])
                    {
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

            outImg->chals->ch[0][y * inImg->width + x] = median;
        }
    }

    outImg->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

int rgbSplit(const Image *inImg, Image *rImg, Image *gImg, Image *bImg)
/**
 * @brief Splits an RGB888 image into R, G, and B bands.
 *
 * @param[in]  inImg  Input image in RGB format (IMAGE_FORMAT_RGB888).
 * @param[out] rImg   Output red band (grayscale format).
 * @param[out] gImg   Output green band.
 * @param[out] bImg   Output blue band.
 */
{
    CHECK_NULL_INT(inImg);
    CHECK_NULL_INT(rImg);
    CHECK_NULL_INT(gImg);
    CHECK_NULL_INT(bImg);
    CHECK_FORMAT_INT(inImg, IMAGE_FORMAT_RGB888);
    CHECK_FORMAT_INT(rImg, IMAGE_FORMAT_GRAYSCALE);
    CHECK_FORMAT_INT(gImg, IMAGE_FORMAT_GRAYSCALE);
    CHECK_FORMAT_INT(bImg, IMAGE_FORMAT_GRAYSCALE);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *r = (uint8_t *)rImg->pixels;
    uint8_t *g = (uint8_t *)gImg->pixels;
    uint8_t *b = (uint8_t *)bImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i)
    {
        r[i] = in[i * 3 + 0];
        g[i] = in[i * 3 + 1];
        b[i] = in[i * 3 + 2];
    }

    return EMBEDDIP_OK;
}

int rgbMerge(const Image *rImg, const Image *gImg, const Image *bImg, Image *outImg)
/**
 * @brief Merges three grayscale bands into a single RGB888 image.
 *
 * @param[in]  rImg    Red band.
 * @param[in]  gImg    Green band.
 * @param[in]  bImg    Blue band.
 * @param[out] outImg  Output image in RGB format.
 */
{
    CHECK_NULL_INT(rImg);
    CHECK_NULL_INT(gImg);
    CHECK_NULL_INT(bImg);
    CHECK_NULL_INT(outImg);
    CHECK_FORMAT_INT(outImg, IMAGE_FORMAT_RGB888);
    CHECK_FORMAT_INT(rImg, IMAGE_FORMAT_GRAYSCALE);
    CHECK_FORMAT_INT(gImg, IMAGE_FORMAT_GRAYSCALE);
    CHECK_FORMAT_INT(bImg, IMAGE_FORMAT_GRAYSCALE);

    uint8_t *out = (uint8_t *)outImg->pixels;
    const uint8_t *r = (const uint8_t *)rImg->pixels;
    const uint8_t *g = (const uint8_t *)gImg->pixels;
    const uint8_t *b = (const uint8_t *)bImg->pixels;

    for (uint32_t i = 0; i < outImg->size; ++i)
    {
        out[i * 3 + 0] = r[i];
        out[i * 3 + 1] = g[i];
        out[i * 3 + 2] = b[i];
    }

    return EMBEDDIP_OK;
}

int dogFilter(const Image *inImg, Image *outImg, float sigma1, float sigma2)
{
    if (!inImg || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (inImg->format != IMAGE_FORMAT_GRAYSCALE) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    int width = inImg->width;
    int height = inImg->height;
    int size1 = (int)(6 * sigma1 + 1) | 1; // force odd size
    int size2 = (int)(6 * sigma2 + 1) | 1;
    int half1 = size1 / 2;
    int half2 = size2 / 2;

    // Allocate Gaussian kernels
    float *kernel1 = (float *)memory_alloc(size1 * size1 * sizeof(float));
    float *kernel2 = (float *)memory_alloc(size2 * size2 * sizeof(float));

    float sum1 = 0.0f, sum2 = 0.0f;

    // Fill Gaussian kernel1
    for (int y = -half1; y <= half1; ++y)
    {
        for (int x = -half1; x <= half1; ++x)
        {
            float val = expf(-(x * x + y * y) / (2.0f * sigma1 * sigma1));
            kernel1[(y + half1) * size1 + (x + half1)] = val;
            sum1 += val;
        }
    }

    // Normalize kernel1
    for (int i = 0; i < size1 * size1; ++i)
        kernel1[i] /= sum1;

    // Fill Gaussian kernel2
    for (int y = -half2; y <= half2; ++y)
    {
        for (int x = -half2; x <= half2; ++x)
        {
            float val = expf(-(x * x + y * y) / (2.0f * sigma2 * sigma2));
            kernel2[(y + half2) * size2 + (x + half2)] = val;
            sum2 += val;
        }
    }

    // Normalize kernel2
    for (int i = 0; i < size2 * size2; ++i)
        kernel2[i] /= sum2;

    // Allocate intermediate float buffers
    float *src = (float *)memory_alloc(width * height * sizeof(float));
    float *blur1 = (float *)memory_alloc(width * height * sizeof(float));
    float *blur2 = (float *)memory_alloc(width * height * sizeof(float));

    // Convert input pixels to float
    const uint8_t *raw = (const uint8_t *)inImg->pixels;
    for (int i = 0; i < width * height; ++i)
        src[i] = (float)raw[i];

    // Convolve with kernel1 (sigma1)
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float sum = 0.0f;
            for (int fy = -half1; fy <= half1; ++fy)
            {
                for (int fx = -half1; fx <= half1; ++fx)
                {
                    int iy = y + fy;
                    int ix = x + fx;
                    if (iy >= 0 && iy < height && ix >= 0 && ix < width)
                        sum += src[iy * width + ix] * kernel1[(fy + half1) * size1 + (fx + half1)];
                }
            }
            blur1[y * width + x] = sum;
        }
    }

    // Convolve with kernel2 (sigma2)
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float sum = 0.0f;
            for (int fy = -half2; fy <= half2; ++fy)
            {
                for (int fx = -half2; fx <= half2; ++fx)
                {
                    int iy = y + fy;
                    int ix = x + fx;
                    if (iy >= 0 && iy < height && ix >= 0 && ix < width)
                        sum += src[iy * width + ix] * kernel2[(fy + half2) * size2 + (fx + half2)];
                }
            }
            blur2[y * width + x] = sum;
        }
    }

    // Allocate output float channel
    if (!outImg->chals)
    {
        outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(outImg->chals, 0, sizeof(channels_t));
    }
    outImg->chals->ch[0] = (float *)memory_alloc(width * height * sizeof(float));
    outImg->is_chals |= channel_mask[0];
    float *outCh = outImg->chals->ch[0];

    // Compute DoG = |blur1 - blur2|
    for (int i = 0; i < width * height; ++i)
        outCh[i] = fabsf(blur1[i] - blur2[i]);

    // Free temporary resources
    memory_free(kernel1);
    memory_free(kernel2);
    memory_free(src);
    memory_free(blur1);
    memory_free(blur2);

    outImg->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

/**
 * @brief Applies Laplacian of Gaussian (LoG) filtering to a grayscale image.
 *
 * Generates a LoG kernel using the given sigma, applies 2D convolution, and stores
 * the result in outImg->ch[0]. LoG is useful for detecting blob-like or closed contours.
 *
 * @param[in]  inImg   Pointer to grayscale input image.
 * @param[out] outImg  Pointer to output image (float convolution result).
 * @param[in]  sigma   Standard deviation of the Gaussian (controls smoothness).
 */
int logFilter(const Image *inImg, Image *outImg, float sigma)
{
    if (!inImg || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (inImg->format != IMAGE_FORMAT_GRAYSCALE) {
        return EMBEDDIP_ERROR_INVALID_FORMAT;
    }

    int width = inImg->width;
    int height = inImg->height;
    int ksize = ((int)(6 * sigma + 1)) | 1; // force odd
    int half = ksize / 2;
    float s2 = sigma * sigma;
    float s4 = s2 * s2;

    // Allocate LoG kernel
    float *kernel = (float *)memory_alloc(ksize * ksize * sizeof(float));

    float sum = 0.0f;
    for (int y = -half; y <= half; ++y)
    {
        for (int x = -half; x <= half; ++x)
        {
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
    float *src = (float *)memory_alloc(width * height * sizeof(float));
    const uint8_t *raw = (const uint8_t *)inImg->pixels;
    for (int i = 0; i < width * height; ++i)
        src[i] = (float)raw[i];

    // Allocate output float buffer
    if (!outImg->chals)
    {
        outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(outImg->chals, 0, sizeof(channels_t));
    }
    outImg->chals->ch[0] = (float *)memory_alloc(width * height * sizeof(float));
    outImg->is_chals |= channel_mask[0];
    float *outCh = outImg->chals->ch[0];

    // Convolution with LoG kernel
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float sum = 0.0f;
            for (int fy = -half; fy <= half; ++fy)
            {
                for (int fx = -half; fx <= half; ++fx)
                {
                    int iy = y + fy;
                    int ix = x + fx;
                    if (iy >= 0 && iy < height && ix >= 0 && ix < width)
                        sum += src[iy * width + ix] *
                               kernel[(fy + half) * ksize + (fx + half)];
                }
            }
            outCh[y * width + x] = sum;
        }
    }

    memory_free(kernel);
    memory_free(src);
    outImg->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/**
 * @brief Non-maximum suppression on gradient magnitude using gradient phase.
 * Keeps local maxima along gradient direction.
 *
 * @param[in]  magImg   Gradient magnitude (float ch0).
 * @param[in]  phaseImg Gradient phase (float ch0, radians).
 * @param[out] outImg   Suppressed output (float ch0).
 */
void nonMaximumSuppression(const Image *magImg, const Image *phaseImg, Image *outImg)
{
    if (!magImg || !phaseImg || !outImg) return;
    uint32_t w = magImg->width, h = magImg->height;
    if (w != phaseImg->width || h != phaseImg->height) return;
    uint32_t N = w * h;

    const float *mag = magImg->chals->ch[0];
    const float *phase = phaseImg->chals->ch[0];

    if (!outImg->chals)
    {
        outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(outImg->chals, 0, sizeof(channels_t));
    }
    outImg->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    outImg->is_chals = 1;
    float *dst = outImg->chals->ch[0];

    // Iterate, skip borders
    for (uint32_t y = 1; y < h - 1; y++)
    {
        for (uint32_t x = 1; x < w - 1; x++)
        {
            uint32_t idx = y * w + x;
            float angle = phase[idx] * 180.0f / (float)M_PI;
            if (angle < 0)
                angle += 180.0f;

            float m = mag[idx];
            float m1 = 0, m2 = 0;

            if ((angle >= 0 && angle < 22.5) || (angle >= 157.5 && angle <= 180))
            {
                m1 = mag[idx - 1];
                m2 = mag[idx + 1]; // left-right
            }
            else if (angle >= 22.5 && angle < 67.5)
            {
                m1 = mag[idx - w - 1];
                m2 = mag[idx + w + 1]; // diag ↘
            }
            else if (angle >= 67.5 && angle < 112.5)
            {
                m1 = mag[idx - w];
                m2 = mag[idx + w]; // up-down
            }
            else if (angle >= 112.5 && angle < 157.5)
            {
                m1 = mag[idx - w + 1];
                m2 = mag[idx + w - 1]; // diag ↙
            }

            dst[idx] = (m >= m1 && m >= m2) ? m : 0.0f;
        }
    }

    outImg->log = IMAGE_DATA_CH0;
}

#define STRONG 255
#define WEAK 50

/**
 * @brief Apply double thresholding to classify strong/weak edges.
 * Writes to float ch0 (values: 0.0f, weakVal, strongVal).
 */
void doubleThreshold(const Image *inImg, Image *outImg,
                     float lowThresh, float highThresh,
                     float weakVal, float strongVal)
{
    if (!inImg || !outImg) return;
    uint32_t N = inImg->width * inImg->height;
    const float *src = inImg->chals->ch[0];

    if (!outImg->chals)
    {
        outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(outImg->chals, 0, sizeof(channels_t));
    }
    outImg->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    outImg->is_chals = 1;
    float *dst = outImg->chals->ch[0];

    for (uint32_t i = 0; i < N; i++)
    {
        if (src[i] >= highThresh)
            dst[i] = strongVal;
        else if (src[i] >= lowThresh)
            dst[i] = weakVal;
        else
            dst[i] = 0.0f;
    }
}

/**
 * @brief Edge tracking by hysteresis.
 * Promotes weak edges connected to strong edges.
 */
void hysteresis(const Image *inImg, Image *outImg,
                float weakVal, float strongVal)
{
    if (!inImg || !outImg) return;
    uint32_t w = inImg->width, h = inImg->height;
    const float *src = inImg->chals->ch[0];

    if (!outImg->chals)
    {
        outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(outImg->chals, 0, sizeof(channels_t));
    }
    outImg->chals->ch[0] = (float *)memory_alloc((size_t)w * h * sizeof(float));
    outImg->is_chals = 1;
    float *dst = outImg->chals->ch[0];
    memcpy(dst, src, (size_t)w * h * sizeof(float));

    for (uint32_t y = 1; y < h - 1; y++)
    {
        for (uint32_t x = 1; x < w - 1; x++)
        {
            uint32_t idx = y * w + x;
            if (dst[idx] == weakVal)
            {
                bool connected = false;
                for (int j = -1; j <= 1; j++)
                {
                    for (int i = -1; i <= 1; i++)
                    {
                        if (dst[(y + j) * w + (x + i)] == strongVal)
                        {
                            connected = true;
                        }
                    }
                }
                dst[idx] = connected ? strongVal : 0.0f;
            }
        }
    }
    outImg->log = IMAGE_DATA_CH0;
}

static void make_gaussian_and_dgauss_1d(float sigma,
                                        float **G, float **dG, int *ksize)
{
    int k = ((int)(6.f * sigma + 1.f)) | 1; // force odd
    int r = k >> 1;

    float *g = (float *)memory_alloc((size_t)k * sizeof(float));
    float *dg = (float *)memory_alloc((size_t)k * sizeof(float));

    float s2 = sigma * sigma;
    float norm = 1.f / (sqrtf(2.f * (float)M_PI) * sigma);

    float sum_g = 0.f, sum_dg = 0.f;

    for (int i = -r; i <= r; ++i)
    {
        float x = (float)i;
        float gx = norm * expf(-(x * x) / (2.f * s2));
        float dgx = -(x / s2) * gx; // d/dx G(x) = -(x/sigma^2) G(x)
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

static void sep_conv_xy_f32(const float *src, float *dst,
                            int width, int height,
                            const float *kx, int kx_sz,
                            const float *ky, int ky_sz)
{
    int rx = kx_sz >> 1, ry = ky_sz >> 1;
    float *tmp = (float *)memory_alloc((size_t)width * height * sizeof(float));

    // X
    for (int y = 0; y < height; ++y)
    {
        const float *row = src + y * width;
        float *trow = tmp + y * width;
        for (int x = 0; x < width; ++x)
        {
            float acc = 0.f;
            for (int i = -rx; i <= rx; ++i)
            {
                int xx = x + i;
                if (xx < 0)
                    xx = 0;
                if (xx >= width)
                    xx = width - 1;
                acc += row[xx] * kx[i + rx];
            }
            trow[x] = acc;
        }
    }

    // Y
    for (int y = 0; y < height; ++y)
    {
        float *drow = dst + y * width;
        for (int x = 0; x < width; ++x)
        {
            float acc = 0.f;
            for (int j = -ry; j <= ry; ++j)
            {
                int yy = y + j;
                if (yy < 0)
                    yy = 0;
                if (yy >= height)
                    yy = height - 1;
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
 * Writes float results into outIx->chals->ch[0] and outIy->chals->ch[0],
 * mirroring your logFilter() “float in ch0” convention.
 */
int gaussianGradients(const Image *inImg, Image *outIx, Image *outIy, float sigma)
{
    CHECK_NULL_INT(inImg);
    CHECK_NULL_INT(outIx);
    CHECK_NULL_INT(outIy);
    CHECK_FORMAT_INT(inImg, IMAGE_FORMAT_GRAYSCALE);
    int width = inImg->width, height = inImg->height;
    int N = width * height;

    // input to float buffer
    const uint8_t *raw = (const uint8_t *)inImg->pixels;
    float *src = (float *)memory_alloc((size_t)N * sizeof(float));
    for (int i = 0; i < N; ++i)
        src[i] = (float)raw[i];

    // kernels
    float *G = NULL, *dG = NULL;
    int ksz = 0;
    make_gaussian_and_dgauss_1d(sigma, &G, &dG, &ksz);

    // prepare outputs (float channels)
    if (!outIx->chals)
    {
        outIx->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(outIx->chals, 0, sizeof(channels_t));
    }
    if (!outIy->chals)
    {
        outIy->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(outIy->chals, 0, sizeof(channels_t));
    }
    outIx->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    outIy->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    outIx->is_chals = 1;
    outIy->is_chals = 1;
    float *ix = outIx->chals->ch[0];
    float *iy = outIy->chals->ch[0];

    // Ix = (I * dG_x) * G_y
    sep_conv_xy_f32(src, ix, width, height, dG, ksz, G, ksz);
    // Iy = (I * G_x) * dG_y
    sep_conv_xy_f32(src, iy, width, height, G, ksz, dG, ksz);

    memory_free(G);
    memory_free(dG);
    memory_free(src);

    // optional: mark data origin/type if you track it (similar to outImg->log = IMAGE_DATA_CH0)
    outIx->log = IMAGE_DATA_CH0; // reuse your flag; or define IMAGE_DATA_DX if you prefer
    outIy->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/**
 * @brief Compute gradient magnitude sqrt(Ix^2 + Iy^2) from float ch0 of Ix, Iy.
 * Writes float magnitude to outMag->chals->ch[0].
 */
int gradientMagnitude(const Image *IxImg, const Image *IyImg, Image *outMag)
{
    CHECK_NULL_INT(IxImg);
    CHECK_NULL_INT(IyImg);
    CHECK_NULL_INT(outMag);
    uint32_t width = IxImg->width, height = IxImg->height;
    CHECK_CONDITION_INT(width == IyImg->width && height == IyImg->height);
    uint32_t N = width * height;

    const float *ix = IxImg->chals ? IxImg->chals->ch[0] : NULL;
    const float *iy = IyImg->chals ? IyImg->chals->ch[0] : NULL;
    CHECK_NULL_INT(ix);
    CHECK_NULL_INT(iy);

    if (!outMag->chals)
    {
        outMag->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(outMag->chals, 0, sizeof(channels_t));
    }
    outMag->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    outMag->is_chals = 1;
    float *mag = outMag->chals->ch[0];

    for (uint32_t i = 0; i < N; ++i)
    {
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
int gradientPhase(const Image *IxImg, const Image *IyImg, Image *outPhase)
{
    CHECK_NULL_INT(IxImg);
    CHECK_NULL_INT(IyImg);
    CHECK_NULL_INT(outPhase);
    uint32_t width = IxImg->width, height = IyImg->height;
    CHECK_CONDITION_INT(width == IyImg->width && height == IyImg->height);
    uint32_t N = width * height;

    const float *ix = IxImg->chals ? IxImg->chals->ch[0] : NULL;
    const float *iy = IyImg->chals ? IyImg->chals->ch[0] : NULL;
    CHECK_NULL_INT(ix);
    CHECK_NULL_INT(iy);

    if (!outPhase->chals)
    {
        outPhase->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        memset(outPhase->chals, 0, sizeof(channels_t));
    }
    outPhase->chals->ch[0] = (float *)memory_alloc((size_t)N * sizeof(float));
    outPhase->is_chals = 1;
    float *phase = outPhase->chals->ch[0];

    for (uint32_t i = 0; i < N; ++i)
    {
        float gx = ix[i], gy = iy[i];
        phase[i] = atan2f(gy, gx); // radians, [-π, π]
    }

    outPhase->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

int Canny(const Image *inImg, Image *outImg,
           double threshold1, double threshold2,
           int apertureSize, bool L2gradient)
{
    CHECK_NULL_INT(inImg);
    CHECK_NULL_INT(outImg);

    // --- Step 1: Gaussian smoothing + gradients ---
    float sigma = 1.0; // 0.3 * ((apertureSize - 1) * 0.5 - 1) + 0.8; // could derive from apertureSize
    Image *Ix = createImageWH_legacy(inImg->width, inImg->height, inImg->format);
    Image *Iy = createImageWH_legacy(inImg->width, inImg->height, inImg->format);
    gaussianGradients(inImg, Ix, Iy, sigma);

    // --- Step 2: magnitude + phase ---
    Image *Mag = createImageWH_legacy(inImg->width, inImg->height, inImg->format);
    Image *Phase = createImageWH_legacy(inImg->width, inImg->height, inImg->format);
    gradientMagnitude(Ix, Iy, Mag);
    gradientPhase(Ix, Iy, Phase);

    float *data = Mag->chals->ch[0];

    float min = FLT_MAX,
          max = -FLT_MAX;

    // Step 1: Find min and max
    for (uint32_t i = 0; i < inImg->size; i++)
    {
        float v = Mag->chals->ch[0][i];
        if (v < min)
            min = v;
        if (v > max)
            max = v;
    }

    // Step 2: Normalize to [0, 255] and clamp
    if (max != min)
    {
        for (uint32_t i = 0; i < Mag->size; i++)
        {
            float norm = (Mag->chals->ch[0][i] - min) / (max - min);
            data[i] = (float)(norm * 255.0f + 0.5f); // +0.5 for rounding
        }
    }
    else
    {
        // All values are the same, map to 0 or 255
        memset(data, 0, Mag->size); // Or use 255
    }

    // --- Step 3: NMS ---
    Image *Nms = createImageWH_legacy(inImg->width, inImg->height, inImg->format);
    nonMaximumSuppression(Mag, Phase, Nms);

    // --- Step 4: Double threshold ---
    Image *Dt = createImageWH_legacy(inImg->width, inImg->height, inImg->format);
    doubleThreshold(Nms, Dt, (float)threshold1, (float)threshold2, 50.0f, 255.0f);

    // --- Step 5: Hysteresis ---
    hysteresis(Dt, outImg, 50.0f, 255.0f);

    // free temps
    // deleteImage(Ix);
    // deleteImage(Iy);
    // deleteImage(Mag);
    // deleteImage(Phase);
    // deleteImage(Nms);
    // deleteImage(Dt);

    return EMBEDDIP_OK;
}

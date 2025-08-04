#include "filter.h"
#include "float.h"

// Internal helper — filters only one channel
uint8_t channel_mask[] = {
    0xFE,
    0xFD,
    0xFB,
    0xF7,
};

// i am not sure about the kernel.
void filter2D_single_channel(Image *inImg, Image *outImg, int ch_idx, void *ctx)
{

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
        inImg->chals->ch[ch_idx] = (float *)memory_alloc(height * width * BYTES_PER_PIXEL);
        inCh = inImg->chals->ch[ch_idx];
        inImg->is_chals = inImg->is_chals | ~(channel_mask[ch_idx]);

        assert(inCh);
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
        outImg->chals->ch[ch_idx] = (float *)memory_alloc(height * width * BYTES_PER_PIXEL);
        outImg->is_chals = outImg->is_chals | ~(channel_mask[ch_idx]);
        outCh = outImg->chals->ch[ch_idx];
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
}

void filter2D_separable(Image *inImg, Image *outImg, int sizeX, float *kernelX, int sizeY, float *kernelY, float delta)
{
    assert(kernelY == kernelX);

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

void minFilter(const Image *inImg, Image *outImg, int kernelSize)
{
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
                uint8_t minPixelValue = MAX_INTENSITY;

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
}

/**
 * @brief Applies a max filter (non-linear) to the image using a square window.
 *
 * @param inImg Input image.
 * @param outImg Output image after max filtering.
 * @param kernelSize Size of the square window (must be odd).
 */

void maxFilter(const Image *inImg, Image *outImg, int kernelSize)
{
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
}

#include <stdlib.h>

/**
 * @brief Applies a median filter (non-linear) to the image using a square window.
 *
 * @param inImg Input image.
 * @param outImg Output image after median filtering.
 * @param kernelSize Size of the square window (must be odd).
 */
void medianFilter(const Image *inImg, Image *outImg, int kernelSize)
{
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
}

void rgbSplit(const Image *inImg, Image *rImg, Image *gImg, Image *bImg)
/**
 * @brief Splits an RGB888 image into R, G, and B bands.
 *
 * @param[in]  inImg  Input image in RGB format (IMAGE_FORMAT_RGB888).
 * @param[out] rImg   Output red band (grayscale format).
 * @param[out] gImg   Output green band.
 * @param[out] bImg   Output blue band.
 */
{
    assert(inImg && rImg && gImg && bImg);
    assert(inImg->format == IMAGE_FORMAT_RGB888);
    assert(rImg->format == IMAGE_FORMAT_GRAYSCALE);
    assert(gImg->format == IMAGE_FORMAT_GRAYSCALE);
    assert(bImg->format == IMAGE_FORMAT_GRAYSCALE);

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
}

void rgbMerge(const Image *rImg, const Image *gImg, const Image *bImg, Image *outImg)
/**
 * @brief Merges three grayscale bands into a single RGB888 image.
 *
 * @param[in]  rImg    Red band.
 * @param[in]  gImg    Green band.
 * @param[in]  bImg    Blue band.
 * @param[out] outImg  Output image in RGB format.
 */
{
    assert(rImg && gImg && bImg && outImg);
    assert(outImg->format == IMAGE_FORMAT_RGB888);
    assert(rImg->format == IMAGE_FORMAT_GRAYSCALE);
    assert(gImg->format == IMAGE_FORMAT_GRAYSCALE);
    assert(bImg->format == IMAGE_FORMAT_GRAYSCALE);

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
}

// NEW ADDED:

float *createGaussianKernel(int size, float sigma)
{
    float *kernel = (float *)memory_alloc(size * size * sizeof(float));
    int half = size / 2;
    float sum = 0.0f;

    for (int y = -half; y <= half; ++y)
    {
        for (int x = -half; x <= half; ++x)
        {
            float value = expf(-(x * x + y * y) / (2.0f * sigma * sigma));
            kernel[(y + half) * size + (x + half)] = value;
            sum += value;
        }
    }

    // Normalize kernel
    for (int i = 0; i < size * size; ++i)
        kernel[i] /= sum;

    return kernel;
}

void dogFilter(const Image *inImg, Image *outImg, float sigma1, float sigma2)
{
    if (!inImg || !outImg || inImg->format != IMAGE_FORMAT_GRAYSCALE)
        return;

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
void logFilter(const Image *inImg, Image *outImg, float sigma)
{
    if (!inImg || !outImg || inImg->format != IMAGE_FORMAT_GRAYSCALE)
        return;

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
}

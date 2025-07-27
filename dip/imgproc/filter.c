#include "filter.h"
#include "float.h"
#include <memory_manager.h>
#include "common.h"
#include "memory_manager.h"
#include "math.h"
#include "stdlib.h"

// Internal helper — filters only one channel
uint8_t channel_mask[] = {
    0xFE,
    0xFD,
    0xFB,
    0xF7,
};

void filter2D_single_channel(Image *inImg, Image *outImg, int ch_idx, void *ctx)
{

    Filter2DContext *context = (Filter2DContext *)ctx;
    int size = context->size;
    float(*filter)[size] = (float(*)[size])context->kernel;

    int width = inImg->width;
    int height = inImg->height;
    int half = size / 2;

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

            for (int fy = -half; fy <= half; ++fy)
            {
                for (int fx = -half; fx <= half; ++fx)
                {
                    int iy = y + fy;
                    int ix = x + fx;

                    float val = 0.0f;
                    if (iy >= 0 && iy < height && ix >= 0 && ix < width)
                        val = inCh[iy * width + ix];

                    sum += val * filter[fy + half][fx + half];
                }
            }

            outCh[y * width + x] = sum;
        }
    }
}

void filter2D_separable(Image *inImg, Image *outImg, int sizeX, float *kernelX, int sizeY, float *kernelY, float delta)
{
    int size = sizeX;
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

        for (int y = 0; y < inImg->height; ++y)
        {
            for (int x = 0; x < inImg->width; ++x)
            {
                uint8_t minPixelValue = MAX_INTENSITY;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky)
                {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx)
                    {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < inImg->width &&
                            offsetY >= 0 && offsetY < inImg->height)
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

        for (int y = 0; y < inImg->height; ++y)
        {
            for (int x = 0; x < inImg->width; ++x)
            {
                float minPixelValue = FLT_MAX;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky)
                {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx)
                    {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < inImg->width &&
                            offsetY >= 0 && offsetY < inImg->height)
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
        for (int y = 0; y < inImg->height; ++y)
        {
            for (int x = 0; x < inImg->width; ++x)
            {
                uint8_t maxPixelValue = 0;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky)
                {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx)
                    {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < inImg->width &&
                            offsetY >= 0 && offsetY < inImg->height)
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
        for (int y = 0; y < inImg->height; ++y)
        {
            for (int x = 0; x < inImg->width; ++x)
            {
                uint8_t maxPixelValue = 0;

                for (int ky = -kernelRadius; ky <= kernelRadius; ++ky)
                {
                    for (int kx = -kernelRadius; kx <= kernelRadius; ++kx)
                    {
                        int offsetX = x + kx;
                        int offsetY = y + ky;

                        if (offsetX >= 0 && offsetX < inImg->width &&
                            offsetY >= 0 && offsetY < inImg->height)
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

    for (int y = 0; y < inImg->height; ++y)
    {
        for (int x = 0; x < inImg->width; ++x)
        {
            int count = 0;

            for (int ky = -kernelRadius; ky <= kernelRadius; ++ky)
            {
                for (int kx = -kernelRadius; kx <= kernelRadius; ++kx)
                {
                    int offsetX = x + kx;
                    int offsetY = y + ky;

                    if (offsetX >= 0 && offsetX < inImg->width &&
                        offsetY >= 0 && offsetY < inImg->height)
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

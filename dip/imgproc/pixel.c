#include "image.h"
#include "assert.h"

/**
 * @brief Applies negative transformation to an image.
 *
 * This function computes the negative of a given image by subtracting each pixel value
 * from the maximum intensity value (typically 255 for 8-bit grayscale images).
 *
 * @param[in]  inImg   Pointer to the input image structure (source image).
 * @param[out] outImg  Pointer to the output image structure (resulting negative image).
 *
 * @note Assumes both images are of the same size and format, and `MAX_INTENSITY` is defined appropriately.
 */
void negative(const Image *inImg, Image *outImg)
{
    uint8_t *imgData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    for (int i = 0; i < inImg->size; ++i)
    {
        outData[i] = MAX_INTENSITY - imgData[i];
    }
}

#include <math.h>

/**
 * @brief Applies a power-law (gamma) transformation and stores the result in float channels.
 *
 * This function normalizes the pixel data to [0, 1], applies gamma correction,
 * and stores the result in `outImg->chals`. It ensures high-precision results
 * for further processing (e.g., Fourier or filtering).
 *
 * @param[in]  inImg   Pointer to the input image (supports GRAYSCALE or RGB).
 * @param[out] outImg  Pointer to the output image. Allocated `chals` field will be filled.
 * @param[in]  gamma   Gamma value (e.g., <1 brightens, >1 darkens).
 * @param[in]  c       Scaling constant (typically 1.0).
 *
 * @note Input must have 8-bit depth. Output must have `chals` allocated.
 */
void powerTransform(const Image *inImg, Image *outImg, float gamma, float c)
{
    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(inImg))
    {
        createChals(inImg, inImg->depth);
        uint8_t *imgData = (uint8_t *)inImg->pixels;

        for (uint32_t i = 0; i < inImg->width * inImg->height * inImg->depth; ++i)
        {
            float norm = imgData[i] / 255.0f;
            outImg->chals->ch[0][i] = c * powf(norm, gamma);
        }
    }
    else
    {
        float *imgData = (uint8_t *)inImg->chals->ch[0];

        for (uint32_t i = 0; i < inImg->width * inImg->height * inImg->depth; ++i)
        {
            float norm = imgData[i] / 255.0f;
            outImg->chals->ch[0][i] = c * powf(norm, gamma);
        }
    }
}

/**
 * @brief Applies linear scaling and bias followed by absolute value and clamps to 8-bit.
 *
 * This function simulates OpenCV's convertScaleAbs: dst = saturate_cast<uchar>(abs(alpha * src + beta))
 *
 * @param[in]  inImg   Pointer to the input image (supports GRAYSCALE or RGB, 8-bit).
 * @param[out] outImg  Pointer to the output image. Output must have pixels allocated.
 * @param[in]  alpha   Gain factor.
 * @param[in]  beta    Bias added after scaling.
 */
void convertScaleAbs(const Image *inImg, Image *outImg, float alpha, float beta)
{

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(inImg))
    {
        createChals(inImg, inImg->depth);

        uint8_t *src = (uint8_t *)inImg->pixels;

        for (uint32_t i = 0; i < inImg->width * inImg->height; ++i)
        {
            outImg->chals->ch[0][i] = alpha * (float)src[i] + beta;
        }
    }
    else
    {
        float *src = (float *)inImg->chals->ch[0];

        for (uint32_t i = 0; i < inImg->size; ++i)
        {
            outImg->chals->ch[0][i] = alpha * src[i] + beta;
        }
    }
}

/**
 * @brief Apply a piecewise linear transformation to an image.
 *
 * This function maps each pixel's intensity based on user-defined breakpoints
 * and corresponding output values, enabling custom contrast adjustment.
 * Assumes input is 8-bit grayscale and output `chals` is allocated.
 *
 * @param[in]  inImg        Input grayscale image.
 * @param[out] outImg       Output image after transformation.
 * @param[in]  breakpoints  Intensity breakpoints (must be sorted, in [0, 255]).
 * @param[in]  values       Corresponding output values (in [0, 1] or desired float range).
 * @param[in]  numPoints    Number of breakpoints (must be ≥2 and match values).
 */
void piecewiseTransform(const Image *inImg, Image *outImg,
                        const uint8_t *breakpoints, const uint8_t *values,
                        int numPoints)
{
    if (inImg->depth != 1)
    {
        // Only GRAYSCALE is supported
        return;
    }

    if (isChalsEmpty(inImg))
    {
        createChals(inImg, inImg->depth);
    }

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    uint8_t *imgData = (uint8_t *)inImg->pixels;
    float *outData = outImg->chals->ch[0];
    int totalPixels = inImg->width * inImg->height;

    for (int i = 0; i < totalPixels; ++i)
    {
        uint8_t pixel = imgData[i];

        // Handle out-of-range pixels (before first or after last breakpoint)
        if (pixel <= breakpoints[0])
        {
            outData[i] = values[0] / 255.0f;
        }
        else if (pixel >= breakpoints[numPoints - 1])
        {
            outData[i] = values[numPoints - 1] / 255.0f;
        }
        else
        {
            // Find the segment [breakpoints[j], breakpoints[j+1]] where pixel falls
            for (int j = 0; j < numPoints - 1; ++j)
            {
                if (pixel >= breakpoints[j] && pixel <= breakpoints[j + 1])
                {
                    float x0 = breakpoints[j];
                    float x1 = breakpoints[j + 1];
                    float y0 = values[j] / 255.0f;
                    float y1 = values[j + 1] / 255.0f;

                    // Linear interpolation
                    outData[i] = y0 + ((pixel - x0) * (y1 - y0)) / (x1 - x0);
                    break;
                }
            }
        }
    }
}

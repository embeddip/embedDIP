#include <imgproc/histogram.h>
/**
 * @brief Forms a histogram from the grayscale values of the input image.
 *
 * This function counts the occurrences of each grayscale value (0–255)
 * in the input image and stores them in the `histogram` array.
 * Only works for 8-bit grayscale images.
 *
 * @param[in]  inImg     Pointer to the input image.
 * @param[out] histogram Integer array of 256 elements to store the histogram.
 * @return Total number of pixels processed.
 */
int histForm(const Image *inImg, int *histogram)
{
    if (!inImg || !inImg->pixels || inImg->depth != 1)
        return 0; // Only supports 8-bit grayscale

    // Clear histogram
    for (int i = 0; i < 256; ++i)
        histogram[i] = 0;

    int totalPixels = inImg->width * inImg->height;
    uint8_t *data = (uint8_t *)inImg->pixels;

    for (int i = 0; i < totalPixels; ++i)
    {
        uint8_t pixel = data[i];
        histogram[pixel]++;
    }

    return totalPixels;
}

/**
 * @brief Performs histogram equalization and outputs normalized float values.
 *
 * This function computes the histogram and CDF of the input grayscale image.
 * It maps each pixel intensity to a float value in [0.0, 1.0] using the normalized CDF,
 * and stores the result in `outImg->chals->ch[0][i]`.
 *
 * @param[in]  inImg   Pointer to the input 8-bit grayscale image.
 * @param[out] outImg  Pointer to the output image with float channel data.
 * @return Total number of pixels processed, or 0 on failure.
 */
int histEq(const Image *inImg, Image *outImg)
{

    int totalPixels = inImg->width * inImg->height;

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(inImg))
    {
        createChals(inImg, inImg->depth);

        int histogram[256] = {0};
        int totalPixels = inImg->width * inImg->height;
        uint8_t *inputData = (uint8_t *)inImg->pixels;

        // Compute histogram
        for (int i = 0; i < totalPixels; ++i)
            histogram[inputData[i]]++;

        // Compute CDF
        int cdf[256];
        cdf[0] = histogram[0];
        for (int i = 1; i < 256; ++i)
            cdf[i] = cdf[i - 1] + histogram[i];

        // Normalize CDF to [0.0, 1.0]
        float cdfMin = (float)cdf[0];
        float scale = 1.0f / (totalPixels - cdf[0]); // avoid zero division
        float cdfNormalized[256];
        for (int i = 0; i < 256; ++i)
            cdfNormalized[i] = (cdf[i] - cdfMin) * scale;

        // Apply equalization and store float output
        for (int i = 0; i < totalPixels; ++i)
        {
            uint8_t pixel = inputData[i];
            outImg->chals->ch[0][i] = cdfNormalized[pixel]; // float value in [0.0, 1.0]
        }
    }
    else
    {

        int histogram[256] = {0};
        int totalPixels = inImg->width * inImg->height;
        float *inputData = (float *)inImg->chals->ch[0];

        // Compute histogram
        for (int i = 0; i < totalPixels; ++i)
            histogram[(uint8_t)inputData[i]]++;

        // Compute CDF
        int cdf[256];
        cdf[0] = histogram[0];
        for (int i = 1; i < 256; ++i)
            cdf[i] = cdf[i - 1] + histogram[i];

        // Normalize CDF to [0.0, 1.0]
        float cdfMin = (float)cdf[0];
        float scale = 1.0f / (totalPixels - cdf[0]); // avoid zero division
        float cdfNormalized[256];
        for (int i = 0; i < 256; ++i)
            cdfNormalized[i] = (cdf[i] - cdfMin) * scale;

        // Apply equalization and store float output
        for (int i = 0; i < totalPixels; ++i)
        {
            uint8_t pixel = inputData[i];
            outImg->chals->ch[0][i] = cdfNormalized[pixel]; // float value in [0.0, 1.0]
        }
    }

    return totalPixels;
}

/**
 * @brief Perform histogram specification (matching) on a grayscale image.
 *
 * This function adjusts the grayscale levels of the input image to match a specified
 * target histogram. The result is written as normalized float values to
 * `outImg->chals->ch[0][i]`.
 *
 * @param[in]  inImg            Pointer to the input grayscale image.
 * @param[out] outImg           Pointer to the output image (float channel, same size).
 * @param[in]  targetHistogram  Pointer to the 256-bin target histogram array.
 * @return Total number of pixels processed, or 0 on failure.
 */
int histSpec(const Image *inImg, Image *outImg, const int *targetHistogram)
{

    int histogram[256] = {0};
    int totalPixels = inImg->width * inImg->height;

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(inImg))
    {
        createChals(inImg, inImg->depth);

        const int totalPixels = inImg->width * inImg->height;
        const uint8_t *inputData = (uint8_t *)inImg->pixels;
        float *outputData = outImg->chals->ch[0];

        // Step 1: Compute input histogram
        int inputHistogram[256] = {0};
        for (int i = 0; i < totalPixels; ++i)
            inputHistogram[inputData[i]]++;

        // Step 2: Compute input and target CDFs
        float inputCDF[256] = {0}, targetCDF[256] = {0};

        inputCDF[0] = (float)inputHistogram[0];
        targetCDF[0] = (float)targetHistogram[0];
        for (int i = 1; i < 256; ++i)
        {
            inputCDF[i] = inputCDF[i - 1] + inputHistogram[i];
            targetCDF[i] = targetCDF[i - 1] + targetHistogram[i];
        }

        for (int i = 0; i < 256; ++i)
        {
            inputCDF[i] /= totalPixels;
            targetCDF[i] /= targetCDF[255]; // normalize to 1.0
        }

        // Step 3: Build mapping from input levels to output levels
        uint8_t mapping[256];
        for (int i = 0; i < 256; ++i)
        {
            float diffMin = 1.0f;
            int bestMatch = 0;
            for (int j = 0; j < 256; ++j)
            {
                float diff = fabsf(inputCDF[i] - targetCDF[j]);
                if (diff < diffMin)
                {
                    diffMin = diff;
                    bestMatch = j;
                }
            }
            mapping[i] = (uint8_t)bestMatch;
        }

        // Step 4: Apply mapping and normalize output to [0.0, 1.0]
        for (int i = 0; i < totalPixels; ++i)
        {
            uint8_t original = inputData[i];
            outputData[i] = mapping[original] / 255.0f;
        }
    }
    else
    {
        const int totalPixels = inImg->width * inImg->height;
        const float *inputData = (float *)inImg->chals->ch[0];
        float *outputData = outImg->chals->ch[0];

        // Step 1: Compute input histogram
        int inputHistogram[256] = {0};
        for (int i = 0; i < totalPixels; ++i)
            inputHistogram[(uint8_t)inputData[i]]++;

        // Step 2: Compute input and target CDFs
        float inputCDF[256] = {0}, targetCDF[256] = {0};

        inputCDF[0] = (float)inputHistogram[0];
        targetCDF[0] = (float)targetHistogram[0];
        for (int i = 1; i < 256; ++i)
        {
            inputCDF[i] = inputCDF[i - 1] + inputHistogram[i];
            targetCDF[i] = targetCDF[i - 1] + targetHistogram[i];
        }

        for (int i = 0; i < 256; ++i)
        {
            inputCDF[i] /= totalPixels;
            targetCDF[i] /= targetCDF[255]; // normalize to 1.0
        }

        // Step 3: Build mapping from input levels to output levels
        uint8_t mapping[256];
        for (int i = 0; i < 256; ++i)
        {
            float diffMin = 1.0f;
            int bestMatch = 0;
            for (int j = 0; j < 256; ++j)
            {
                float diff = fabsf(inputCDF[i] - targetCDF[j]);
                if (diff < diffMin)
                {
                    diffMin = diff;
                    bestMatch = j;
                }
            }
            mapping[i] = (uint8_t)bestMatch;
        }

        // Step 4: Apply mapping and normalize output to [0.0, 1.0]
        for (int i = 0; i < totalPixels; ++i)
        {
            uint8_t original = inputData[i];
            outputData[i] = mapping[original] / 255.0f;
        }
    }

    return totalPixels;
}
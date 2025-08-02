#include <stdint.h>
#include <float.h>
#include <assert.h>
#include "image.h" // Your Image and channels_t definitions

/**
 * @brief Resizes a single-channel image to a square output using nearest-neighbor interpolation.
 *
 * @param[in]  inImg        Pointer to the input image.
 * @param[out] outImg       Pointer to the output image. It must already be allocated to (fourier_size × fourier_size).
 * @param[in]  size Desired width and height of the output image.
 */
void resize(Image *inImg, Image *outImg, int size)
{

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    float width_ratio = (float)inImg->width / size;
    float height_ratio = (float)inImg->height / size;

    // Clear output buffer to white (0xFF)
    for (int y = 0; y < outImg->width * outImg->height; y++)
        ((float *)outImg->chals->ch[0])[y] = 255.0f;

    // Resize using nearest neighbor
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            int nearest_x = (int)(x * width_ratio);
            int nearest_y = (int)(y * height_ratio);

            if (nearest_x >= inImg->width)
                nearest_x = inImg->width - 1;
            if (nearest_y >= inImg->height)
                nearest_y = inImg->height - 1;

            outImg->chals->ch[0][y * size + x] =
                (float)((uint8_t *)inImg->pixels)[nearest_y * inImg->width + nearest_x];
        }
    }

    outImg->width = size;
    outImg->height = size;
    outImg->size = size * size * inImg->depth;
}

/**
 * @brief Performs pixel-wise addition of two float-valued grayscale images.
 *
 * This function adds the corresponding float values from `img1` and `img2`, and stores
 * the result in `outImg->chals->ch[0][i]`.
 *
 * Assumes all images have the same resolution and float output channel allocated.
 *
 * @param[in]  img1   Pointer to the first image (float channel expected).
 * @param[in]  img2   Pointer to the second image (float channel expected).
 * @param[out] outImg Pointer to the output image (float channel).
 */
void add(const Image *img1, const Image *img2, Image *outImg)
{

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(img1) && isChalsEmpty(img2))
    {
        int totalPixels = img1->width * img1->height * img1->depth;
        ;

        uint8_t *data1 = img1->chals->ch[0];
        uint8_t *data2 = img2->chals->ch[0];
        float *outData = outImg->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i)
        {
            outData[i] = data1[i] + data2[i];
        }
    }
    else if (isChalsEmpty(img1) && !isChalsEmpty(img2))
    {
        int totalPixels = img1->width * img1->height * img1->depth;

        uint8_t *data1 = img1->chals->ch[0];
        float *data2 = img2->chals->ch[0];
        float *outData = outImg->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i)
        {
            outData[i] = data1[i] + data2[i];
        }
    }
    else if (!isChalsEmpty(img1) && isChalsEmpty(img2))
    {
        int totalPixels = img1->width * img1->height * img1->depth;
        ;

        float *data1 = img1->chals->ch[0];
        uint8_t *data2 = img2->chals->ch[0];
        float *outData = outImg->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i)
        {
            outData[i] = data1[i] + data2[i];
        }
    }
    else
    {
        int totalPixels = img1->width * img1->height * img1->depth;
        ;

        float *data1 = img1->chals->ch[0];
        float *data2 = img2->chals->ch[0];
        float *outData = outImg->chals->ch[0];

        for (int i = 0; i < totalPixels; ++i)
        {
            outData[i] = data1[i] + data2[i];
        }
    }
}

void dist(const Image *inImg, Image *outImg, uint8_t R_ref, uint8_t G_ref, uint8_t B_ref)
/**
 * @brief Computes the color distance of each pixel in an RGB
 * image to a given reference color.
 *
 * @param[in] inImg Pointer to the input RGB image (3 channels, interleaved as RGBRGB...).
 * @param[out] outImg Pointer to the output grayscale image (1 channel, same width and height as input).
 * @param[in] R_ref Reference Red channel value (0–255).
 * @param[in] G_ref Reference Green channel value (0–255).
 * @param[in] B_ref Reference Blue channel value (0–255).
 */
{
    int totalPixels = inImg->width * inImg->height;

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, outImg->depth);
        outImg->is_chals = 1;
    }

    if (isChalsEmpty(inImg))
    {
        // Raw byte access
        uint8_t *inData = (uint8_t *)inImg->pixels;

        for (int i = 0; i < totalPixels; ++i)
        {
            int idx = i * 3;
            uint8_t R = inData[idx];
            uint8_t G = inData[idx + 1];
            uint8_t B = inData[idx + 2];

            float d = sqrtf((R - R_ref) * (R - R_ref) +
                            (G - G_ref) * (G - G_ref) +
                            (B - B_ref) * (B - B_ref));

            outImg->chals->ch[0][i] = d;
        }
    }
    else
    {
        // Channel access (float-based)
        float *R_ch = inImg->chals->ch[0];
        float *G_ch = inImg->chals->ch[1];
        float *B_ch = inImg->chals->ch[2];

        for (int i = 0; i < totalPixels; ++i)
        {
            float d = sqrtf((R_ch[i] - R_ref) * (R_ch[i] - R_ref) +
                            (G_ch[i] - G_ref) * (G_ch[i] - G_ref) +
                            (B_ch[i] - B_ref) * (B_ch[i] - B_ref));

            outImg->chals->ch[0][i] = d;
        }
    }
}
/*
static inline uint8_t normalize_to_u8(float val, float min, float max)
{
    if (max - min < 1e-5f) // avoid divide by zero
        return 0;
    float norm = (val - min) / (max - min);
    if (norm < 0.0f)
        norm = 0.0f;
    if (norm > 1.0f)
        norm = 1.0f;
    return (uint8_t)(norm * 255.0f);
}

void normalize(Image *inImg)
{
    float *data = (float *)inImg->chals->ch[0];

    float min = FLT_MAX, max = -FLT_MAX;
    for (int i = 0; i < inImg->size; i++)
    {
        float v = inImg->chals->ch[0][i];
        if (v < min)
            min = v;
        if (v > max)
            max = v;
    }
    for (int i = 0; i < inImg->size; i++)
    {
        data[i] = normalize_to_u8(inImg->chals->ch[0][i], min, max);
    }
}
*/

static inline uint8_t normalize_to_u8(uint8_t val, uint8_t min, uint8_t max)
{
    if (max - min < 1e-5f) // avoid divide by zero
        return 0;
    float norm = ((float)val - (float)min) / ((float)max - (float)min);
    if (norm < 0.0f)
        norm = 0.0f;
    if (norm > 1.0f)
        norm = 1.0f;
    return (uint8_t)(norm * 255.0f);
}

void normalize(Image *inImg)
{
    uint8_t *data = (uint8_t *)inImg->pixels;

    uint8_t min = 255, max = 0;
    for (int i = 0; i < inImg->size; i++)
    {
        uint8_t v = (uint8_t)data[i];
        if (v < min)
            min = v;
        if (v > max)
            max = v;
    }
    for (int i = 0; i < inImg->size; i++)
    {
        data[i] = normalize_to_u8(data[i], min, max);
    }
}

void convertTo(Image *inImg)
{

    uint8_t *data = (uint8_t *)inImg->pixels;

    switch (inImg->format)
    {
    case IMAGE_FORMAT_GRAYSCALE:
    {
        if (inImg->log == IMAGE_DATA_CH0)
        {
            float min = FLT_MAX, max = -FLT_MAX;

            // Step 1: Find min and max
            for (int i = 0; i < inImg->size; i++)
            {
                float v = inImg->chals->ch[0][i];
                if (v < min)
                    min = v;
                if (v > max)
                    max = v;
            }

            // Step 2: Normalize to [0, 255] and clamp
            if (max != min)
            {
                for (int i = 0; i < inImg->size; i++)
                {
                    float norm = (inImg->chals->ch[0][i] - min) / (max - min);
                    data[i] = (uint8_t)(norm * 255.0f + 0.5f); // +0.5 for rounding
                }
            }
            else
            {
                // All values are the same, map to 0 or 255
                memset(data, 0, inImg->size); // Or use 255
            }
        }
        else if (inImg->log == IMAGE_DATA_PIXELS)
        {
            uint8_t min = 255, max = 0;
            for (int i = 0; i < inImg->size; i++)
            {
                uint8_t v = (uint8_t)data[i];
                if (v < min)
                    min = v;
                if (v > max)
                    max = v;
            }
            for (int i = 0; i < inImg->size; i++)
            {
                data[i] = normalize_to_u8(data[i], min, max);
            }
        }

        break;
    }

    case IMAGE_FORMAT_RGB888:
    case IMAGE_FORMAT_YUV:
    case IMAGE_FORMAT_HSI:
    {
        int ch_r = 1, ch_g = 2, ch_b = 3;
        float min_r = FLT_MAX, max_r = -FLT_MAX;
        float min_g = FLT_MAX, max_g = -FLT_MAX;
        float min_b = FLT_MAX, max_b = -FLT_MAX;

        for (int i = 0; i < inImg->size; i++)
        {
            if (inImg->chals->ch[ch_r][i] < min_r)
                min_r = inImg->chals->ch[ch_r][i];
            if (inImg->chals->ch[ch_r][i] > max_r)
                max_r = inImg->chals->ch[ch_r][i];
            if (inImg->chals->ch[ch_g][i] < min_g)
                min_g = inImg->chals->ch[ch_g][i];
            if (inImg->chals->ch[ch_g][i] > max_g)
                max_g = inImg->chals->ch[ch_g][i];
            if (inImg->chals->ch[ch_b][i] < min_b)
                min_b = inImg->chals->ch[ch_b][i];
            if (inImg->chals->ch[ch_b][i] > max_b)
                max_b = inImg->chals->ch[ch_b][i];
        }

        for (int i = 0; i < inImg->size; i++)
        {
            data[i * 3 + 0] = normalize_to_u8(inImg->chals->ch[ch_b][i], min_b, max_b);
            data[i * 3 + 1] = normalize_to_u8(inImg->chals->ch[ch_g][i], min_g, max_g);
            data[i * 3 + 2] = normalize_to_u8(inImg->chals->ch[ch_r][i], min_r, max_r);
        }
        break;
    }

    case IMAGE_FORMAT_RGB565:
    {
        float min_r = FLT_MAX, max_r = -FLT_MAX;
        float min_g = FLT_MAX, max_g = -FLT_MAX;
        float min_b = FLT_MAX, max_b = -FLT_MAX;

        for (int i = 0; i < inImg->size; i++)
        {
            if (inImg->chals->ch[1][i] < min_r)
                min_r = inImg->chals->ch[1][i];
            if (inImg->chals->ch[1][i] > max_r)
                max_r = inImg->chals->ch[1][i];
            if (inImg->chals->ch[2][i] < min_g)
                min_g = inImg->chals->ch[2][i];
            if (inImg->chals->ch[2][i] > max_g)
                max_g = inImg->chals->ch[2][i];
            if (inImg->chals->ch[3][i] < min_b)
                min_b = inImg->chals->ch[3][i];
            if (inImg->chals->ch[3][i] > max_b)
                max_b = inImg->chals->ch[3][i];
        }

        for (int i = 0; i < inImg->size; i++)
        {
            uint8_t r = normalize_to_u8(inImg->chals->ch[1][i], min_r, max_r);
            uint8_t g = normalize_to_u8(inImg->chals->ch[2][i], min_g, max_g);
            uint8_t b = normalize_to_u8(inImg->chals->ch[3][i], min_b, max_b);

            uint16_t rgb565 = (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
            data[i * 2 + 0] = (uint8_t)(rgb565 & 0xFF);
            data[i * 2 + 1] = (uint8_t)((rgb565 >> 8) & 0xFF);
        }
        break;
    }

    default:
        break;
    }

    inImg->log = IMAGE_DATA_PIXELS;
}
#include <stdint.h>
#include <float.h>
#include <assert.h>
#include "image.h" // Your Image and channels_t definitions

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

void convertTo(Image *inImg)
{
    assert(inImg && inImg->pixels && inImg->chals && inImg->is_chals);

    uint8_t *data = (uint8_t *)inImg->pixels;

    switch (inImg->format)
    {
    case IMAGE_FORMAT_GRAYSCALE:
    {
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
        break;
    }

    case IMAGE_FORMAT_RGB888:
    case IMAGE_FORMAT_YUV:
    case IMAGE_FORMAT_HSV:
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
}

#include "image.h"
#include "math.h"
#include "color.h"

void cvtColor(const Image *inImg, Image *outImg, int code)
{
    switch (code)
    {
    case CVT_RGB5652GRAY:
        rgb565_to_grayscale(inImg, outImg);
        break;
    case CVT_RGB2YUV:
        rgb_to_yuv(inImg, outImg);
        break;
    case CVT_RGB2HSV:
        rgb_to_hsv(inImg, outImg);
        break;
    case CVT_GRAY2RGB:
        grayscale_to_rgb(inImg, outImg);
        break;
    case CVT_RGB2GRAY:
        rgb_to_grayscale(inImg, outImg);
        break;
    case CVT_RGB2RGB565:
        rgb888_to_rgb565(inImg, outImg);
        break;
    case CVT_RGB5652HSV:
        rgb565_to_hsv(inImg, outImg);
        break;
    case CVT_HSV2RGB:
        hsv_to_rgb(inImg, outImg);
        break;
    case CVT_HSV2RGB565:
        hsv_to_rgb565(inImg, outImg);
        break;
    case CVT_RGB2YUV_ALT:
        rgb888_to_yuv(inImg, outImg);
        break;
    case CVT_YUV2RGB:
        yuv_to_rgb888(inImg, outImg);
        break;
    default:
        // Unsupported conversion
        break;
    }
}

static void rgb565_to_grayscale(const Image *inImg, Image *outImg)
{
    const uint16_t *inData = (uint16_t *)inImg->pixels_u8;
    uint8_t *outData = outImg->pixels_u8;

    for (int i = 0; i < inImg->size / 2; ++i)
    {
        uint16_t pixel = inData[i];
        uint8_t r = ((pixel >> 11) & 0x1F) << 3;
        uint8_t g = ((pixel >> 5) & 0x3F) << 2;
        uint8_t b = (pixel & 0x1F) << 3;
        outData[i] = (uint8_t)((r * 0.299f) + (g * 0.587f) + (b * 0.114f));
    }
}

static void rgb_to_yuv(const Image *inImg, Image *outImg)
{
    const uint8_t *inData = inImg->pixels_u8;
    uint8_t *outData = outImg->pixels_u8;

    for (int i = 0; i < inImg->size * 3; i += 3)
    {
        uint8_t r = inData[i];
        uint8_t g = inData[i + 1];
        uint8_t b = inData[i + 2];

        uint8_t y = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
        uint8_t u = (uint8_t)(-0.14713 * r - 0.28886 * g + 0.436 * b + 128);
        uint8_t v = (uint8_t)(0.615 * r - 0.51499 * g - 0.10001 * b + 128);

        outData[i] = y;
        outData[i + 1] = u;
        outData[i + 2] = v;
    }
}

static void grayscale_to_rgb(const Image *inImg, Image *outImg)
{
    const uint8_t *inData = inImg->pixels_u8;
    uint8_t *outData = outImg->pixels_u8;

    for (int i = 0; i < inImg->size; ++i)
    {
        uint8_t gray = inData[i];
        outData[i * 3] = gray;
        outData[i * 3 + 1] = gray;
        outData[i * 3 + 2] = gray;
    }
}

static void rgb888_to_rgb565(const Image *inImg, Image *outImg)
{
    const uint8_t *in = inImg->pixels_u8;
    uint16_t *out = (uint16_t *)outImg->pixels_u8;

    for (int i = 0; i < inImg->size; ++i)
    {
        uint8_t r = in[i * 3];
        uint8_t g = in[i * 3 + 1];
        uint8_t b = in[i * 3 + 2];
        out[i] = RGB888_TO_RGB565(r, g, b);
    }
}

static void rgb_to_grayscale(const Image *inImg, Image *outImg)
{
    const uint8_t *in = inImg->pixels_u8;
    uint8_t *out = outImg->pixels_u8;

    for (int i = 0; i < inImg->size; ++i)
    {
        uint8_t r = in[i * 3];
        uint8_t g = in[i * 3 + 1];
        uint8_t b = in[i * 3 + 2];
        out[i] = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
    }
}

static void rgb888_to_yuv(const Image *inImg, Image *outImg)
{
    const uint8_t *in = inImg->pixels_u8;
    uint8_t *out = outImg->pixels_u8;

    for (int i = 0; i < inImg->size; ++i)
    {
        uint8_t r = in[i * 3];
        uint8_t g = in[i * 3 + 1];
        uint8_t b = in[i * 3 + 2];

        uint8_t y = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
        uint8_t u = (uint8_t)(-0.169 * r - 0.331 * g + 0.5 * b + 128);
        uint8_t v = (uint8_t)(0.5 * r - 0.419 * g - 0.081 * b + 128);

        out[i * 3] = y;
        out[i * 3 + 1] = u;
        out[i * 3 + 2] = v;
    }
}

static void yuv_to_rgb888(const Image *inImg, Image *outImg)
{
    const uint8_t *in = inImg->pixels_u8;
    uint8_t *out = outImg->pixels_u8;

    for (int i = 0; i < inImg->size; ++i)
    {
        int y = in[i * 3];
        int u = in[i * 3 + 1] - 128;
        int v = in[i * 3 + 2] - 128;

        int r = (int)(y + 1.403 * v);
        int g = (int)(y - 0.344 * u - 0.714 * v);
        int b = (int)(y + 1.770 * u);

        out[i * 3] = (uint8_t)fminf(fmaxf(r, 0), 255);
        out[i * 3 + 1] = (uint8_t)fminf(fmaxf(g, 0), 255);
        out[i * 3 + 2] = (uint8_t)fminf(fmaxf(b, 0), 255);
    }
}

static void rgb_to_hsv(const Image *inImg, Image *outImg)
{
    const uint8_t *in = inImg->pixels_u8;
    uint8_t *out = outImg->pixels_u8;

    for (int i = 0; i < inImg->size; ++i)
    {
        float r = in[i * 3] / 255.0f;
        float g = in[i * 3 + 1] / 255.0f;
        float b = in[i * 3 + 2] / 255.0f;

        float max = fmaxf(fmaxf(r, g), b);
        float min = fminf(fminf(r, g), b);
        float delta = max - min;

        float h, s, v = max;

        if (delta == 0)
            h = 0;
        else if (max == r)
            h = 60 * fmodf((g - b) / delta, 6.0f);
        else if (max == g)
            h = 60 * ((b - r) / delta + 2);
        else
            h = 60 * ((r - g) / delta + 4);

        if (h < 0)
            h += 360.0f;
        s = max == 0 ? 0 : (delta / max);

        out[i * 3] = (uint8_t)(h / 360.0f * 255.0f);
        out[i * 3 + 1] = (uint8_t)(s * 255.0f);
        out[i * 3 + 2] = (uint8_t)(v * 255.0f);
    }
}

static void hsv_to_rgb(const Image *inImg, Image *outImg)
{
    const uint8_t *in = inImg->pixels_u8;
    uint8_t *out = outImg->pixels_u8;

    for (int i = 0; i < inImg->size; ++i)
    {
        float h = in[i * 3] / 255.0f * 360.0f;
        float s = in[i * 3 + 1] / 255.0f;
        float v = in[i * 3 + 2] / 255.0f;

        float c = v * s;
        float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
        float m = v - c;

        float r_, g_, b_;
        if (h < 60)
        {
            r_ = c;
            g_ = x;
            b_ = 0;
        }
        else if (h < 120)
        {
            r_ = x;
            g_ = c;
            b_ = 0;
        }
        else if (h < 180)
        {
            r_ = 0;
            g_ = c;
            b_ = x;
        }
        else if (h < 240)
        {
            r_ = 0;
            g_ = x;
            b_ = c;
        }
        else if (h < 300)
        {
            r_ = x;
            g_ = 0;
            b_ = c;
        }
        else
        {
            r_ = c;
            g_ = 0;
            b_ = x;
        }

        out[i * 3] = (uint8_t)((r_ + m) * 255.0f);
        out[i * 3 + 1] = (uint8_t)((g_ + m) * 255.0f);
        out[i * 3 + 2] = (uint8_t)((b_ + m) * 255.0f);
    }
}

static void rgb565_to_hsv(const Image *inImg, Image *outImg)
{
    const uint16_t *in = (uint16_t *)inImg->pixels_u8;
    uint8_t *out = outImg->pixels_u8;

    for (int i = 0; i < inImg->size; ++i)
    {
        uint8_t r, g, b;
        RGB565_TO_RGB888(in[i], r, g, b);

        float rf = r / 255.0f;
        float gf = g / 255.0f;
        float bf = b / 255.0f;

        float max = fmaxf(fmaxf(rf, gf), bf);
        float min = fminf(fminf(rf, gf), bf);
        float delta = max - min;

        float h, s, v = max;

        if (delta == 0)
            h = 0;
        else if (max == rf)
            h = 60 * fmodf((gf - bf) / delta, 6.0f);
        else if (max == gf)
            h = 60 * ((bf - rf) / delta + 2);
        else
            h = 60 * ((rf - gf) / delta + 4);

        if (h < 0)
            h += 360.0f;
        s = (max == 0) ? 0 : (delta / max);

        out[i * 3] = (uint8_t)(h / 360.0f * 255.0f); // H
        out[i * 3 + 1] = (uint8_t)(s * 255.0f);      // S
        out[i * 3 + 2] = (uint8_t)(v * 255.0f);      // V
    }
}

static void hsv_to_rgb565(const Image *inImg, Image *outImg)
{
    const uint8_t *in = inImg->pixels_u8;
    uint16_t *out = (uint16_t *)outImg->pixels_u8;

    for (int i = 0; i < inImg->size; ++i)
    {
        float h = in[i * 3] / 255.0f * 360.0f;
        float s = in[i * 3 + 1] / 255.0f;
        float v = in[i * 3 + 2] / 255.0f;

        float c = v * s;
        float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
        float m = v - c;

        float rf, gf, bf;

        if (h < 60)
        {
            rf = c;
            gf = x;
            bf = 0;
        }
        else if (h < 120)
        {
            rf = x;
            gf = c;
            bf = 0;
        }
        else if (h < 180)
        {
            rf = 0;
            gf = c;
            bf = x;
        }
        else if (h < 240)
        {
            rf = 0;
            gf = x;
            bf = c;
        }
        else if (h < 300)
        {
            rf = x;
            gf = 0;
            bf = c;
        }
        else
        {
            rf = c;
            gf = 0;
            bf = x;
        }

        uint8_t r = (uint8_t)((rf + m) * 255.0f);
        uint8_t g = (uint8_t)((gf + m) * 255.0f);
        uint8_t b = (uint8_t)((bf + m) * 255.0f);

        out[i] = RGB888_TO_RGB565(r, g, b);
    }
}

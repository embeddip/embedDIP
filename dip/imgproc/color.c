#include "image.h"
#include "math.h"
#include "color.h"
#include "assert.h"

void cvtColor(const Image *inImg, Image *outImg, ColorConversionCode code)
{
    switch (code)
    {
    case CVT_RGB565_TO_GRAY:
        //rgb565_to_grayscale(inImg, outImg);
        break;
    case CVT_RGB_TO_YUV:
        //rgb_to_yuv(inImg, outImg);
        break;
    case CVT_RGB_TO_HSV:
        //rgb_to_hsv(inImg, outImg);
        break;
    case CVT_GRAY_TO_RGB:
        //grayscale_to_rgb(inImg, outImg);
        break;
    case CVT_RGB_TO_GRAY:
        //rgb_to_grayscale(inImg, outImg);
        break;
    case CVT_RGB_TO_RGB565:
        //rgb888_to_rgb565(inImg, outImg);
        break;
    case CVT_RGB565_TO_HSV:
        //rgb565_to_hsv(inImg, outImg);
        break;
    case CVT_HSV_TO_RGB:
        //hsv_to_rgb(inImg, outImg);
        break;
    case CVT_HSV_TO_RGB565:
        //hsv_to_rgb565(inImg, outImg);
        break;
    case CVT_RGB_TO_YUV_ALT:
        //rgb888_to_yuv(inImg, outImg);
        break;
    case CVT_YUV_TO_RGB:
        //yuv_to_rgb888(inImg, outImg);
        break;
    case CVT_RGB565_TO_RGB:
        convert_rgb565_to_rgb888(inImg, outImg);
        break;
    default:
        // Unsupported conversion
        break;
    }
}

void convert_rgb565_to_rgb888(Image *inImg, Image *outImg)
{

    const uint8_t *in_rgb565 = (const uint8_t *)inImg->pixels;
    uint8_t *out_rgb888 = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; i++)
    {
        uint16_t pixel = ((uint16_t *)in_rgb565)[i];

        // Avoid << 3 where possible, extend bits via duplication
        out_rgb888[i * 3 + 0] = ((pixel >> 8) & 0xF8) | ((pixel >> 13) & 0x07); // R
        out_rgb888[i * 3 + 1] = ((pixel >> 3) & 0xFC) | ((pixel >> 9) & 0x03);  // G
        out_rgb888[i * 3 + 2] = ((pixel << 3) & 0xF8) | ((pixel >> 2) & 0x07);  // B
    }
}

void rgb888_to_grayscale_inplace(Image *inImg)
{
    assert(inImg->depth == IMAGE_DEPTH_U8);
    assert(inImg->format == IMAGE_FORMAT_RGB888);

    // Create a temporary output image
    Image *outImg = createImage(IMAGE_RES_WQVGA, IMAGE_FORMAT_GRAYSCALE);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i)
    {
        uint8_t r = in[i * 3];
        uint8_t g = in[i * 3 + 1];
        uint8_t b = in[i * 3 + 2];
        out[i] = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
    }

    // Update inImg metadata to reflect the grayscale image
    inImg->depth = IMAGE_DEPTH_U8;
    inImg->format = IMAGE_FORMAT_GRAYSCALE;
    // inImg->size remains unchanged
}

void rgb888_to_yuv(const Image *inImg, Image *outImg)
{
    assert(inImg->depth == IMAGE_DEPTH_U8);
    assert(inImg->format == IMAGE_FORMAT_RGB888);
    assert(outImg->depth == IMAGE_DEPTH_U8);
    assert(outImg->format == IMAGE_FORMAT_YUV);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i)
    {
        uint8_t r = in[i * 3];
        uint8_t g = in[i * 3 + 1];
        uint8_t b = in[i * 3 + 2];

        uint8_t y = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
        uint8_t u = (uint8_t)(-0.14713 * r - 0.28886 * g + 0.436 * b + 128);
        uint8_t v = (uint8_t)(0.615 * r - 0.51499 * g - 0.10001 * b + 128);

        out[i * 3] = y;
        out[i * 3 + 1] = u;
        out[i * 3 + 2] = v;
    }
}

void grayscale_to_rgb(const Image *inImg, Image *outImg)
{
    assert(inImg->depth == IMAGE_DEPTH_U8);
    assert(inImg->format == IMAGE_FORMAT_GRAYSCALE);
    assert(outImg->depth == IMAGE_DEPTH_U8);
    assert(outImg->format == IMAGE_FORMAT_RGB888);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i)
    {
        uint8_t gray = in[i];
        out[i * 3] = gray;
        out[i * 3 + 1] = gray;
        out[i * 3 + 2] = gray;
    }
}

void rgb888_to_rgb565(const Image *inImg, Image *outImg)
{
    assert(inImg->depth == IMAGE_DEPTH_U8);
    assert(inImg->format == IMAGE_FORMAT_RGB888);
    assert(outImg->depth == IMAGE_DEPTH_U8);
    assert(outImg->format == IMAGE_FORMAT_RGB565);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint16_t *out = (uint16_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i)
    {
        uint8_t r = in[i * 3];
        uint8_t g = in[i * 3 + 1];
        uint8_t b = in[i * 3 + 2];
        out[i] = RGB888_TO_RGB565(r, g, b);
    }
}

void yuv_to_rgb888(const Image *inImg, Image *outImg)
{
    assert(inImg->depth == IMAGE_DEPTH_U8);
    assert(inImg->format == IMAGE_FORMAT_YUV);
    assert(outImg->depth == IMAGE_DEPTH_U8);
    assert(outImg->format == IMAGE_FORMAT_RGB888);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i)
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

void rgb888_to_hsv(const Image *inImg, Image *outImg)
{
    assert(inImg->depth == IMAGE_DEPTH_U8);
    assert(inImg->format == IMAGE_FORMAT_RGB888);
    assert(outImg->depth == IMAGE_DEPTH_U8);
    assert(outImg->format == IMAGE_FORMAT_HSV);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i)
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

void hsv_to_rgb888(const Image *inImg, Image *outImg)
{
    assert(inImg->depth == IMAGE_DEPTH_U8);
    assert(inImg->format == IMAGE_FORMAT_HSV);
    assert(outImg->depth == IMAGE_DEPTH_U8);
    assert(outImg->format == IMAGE_FORMAT_RGB888);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i)
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

void rgb565_to_hsv(const Image *inImg, Image *outImg)
{
    assert(inImg->depth == IMAGE_DEPTH_U8);
    assert(inImg->format == IMAGE_FORMAT_RGB565);
    assert(outImg->depth == IMAGE_DEPTH_U8);
    assert(outImg->format == IMAGE_FORMAT_HSV);

    const uint16_t *in = (const uint16_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i)
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

        out[i * 3] = (uint8_t)(h / 360.0f * 255.0f);
        out[i * 3 + 1] = (uint8_t)(s * 255.0f);
        out[i * 3 + 2] = (uint8_t)(v * 255.0f);
    }
}

void hsv_to_rgb565(const Image *inImg, Image *outImg)
{
    assert(inImg->depth == IMAGE_DEPTH_U8);
    assert(inImg->format == IMAGE_FORMAT_HSV);
    assert(outImg->depth == IMAGE_DEPTH_U8);
    assert(outImg->format == IMAGE_FORMAT_RGB565);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint16_t *out = (uint16_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i)
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

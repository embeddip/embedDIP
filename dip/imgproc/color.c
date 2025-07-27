#include "color.h"

// Static conversion functions

// ----------- Helper Macro for RGB Conversion ------------
#define RGB888_TO_RGB565(r, g, b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))
#define RGB565_TO_RGB888(pixel, r, g, b)                    \
    do                                                      \
    {                                                       \
        r = ((pixel >> 8) & 0xF8) | ((pixel >> 13) & 0x07); \
        g = ((pixel >> 3) & 0xFC) | ((pixel >> 9) & 0x03);  \
        b = ((pixel << 3) & 0xF8) | ((pixel >> 2) & 0x07);  \
    } while (0)

// ----------- RGB888 → GRAYSCALE ------------
static void rgb888_to_grayscale(const Image *inImg, Image *outImg)
{
    assert(inImg->depth == IMAGE_DEPTH_U24 && inImg->format == IMAGE_FORMAT_RGB888);
    assert(outImg->depth == IMAGE_DEPTH_U8 && outImg->format == IMAGE_FORMAT_GRAYSCALE);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i)
    {
        uint8_t r = in[i * 3];
        uint8_t g = in[i * 3 + 1];
        uint8_t b = in[i * 3 + 2];
        out[i] = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
    }
}

// ----------- GRAYSCALE → RGB888 ------------
static void grayscale_to_rgb(const Image *inImg, Image *outImg)
{
    assert(inImg->depth == IMAGE_DEPTH_U8 && inImg->format == IMAGE_FORMAT_GRAYSCALE);
    assert(outImg->depth == IMAGE_DEPTH_U24 && outImg->format == IMAGE_FORMAT_RGB888);

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

// ----------- RGB888 → RGB565 ------------
static void rgb888_to_rgb565(const Image *inImg, Image *outImg)
{
    assert(inImg->format == IMAGE_FORMAT_RGB888 && inImg->depth == IMAGE_DEPTH_U24);
    assert(outImg->format == IMAGE_FORMAT_RGB565 && outImg->depth == IMAGE_DEPTH_U16);

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

// ----------- RGB565 → RGB888 ------------
static void convert_rgb565_to_rgb888(const Image *inImg, Image *outImg)
{
    assert(inImg->format == IMAGE_FORMAT_RGB565 && inImg->depth == IMAGE_DEPTH_U16);
    assert(outImg->format == IMAGE_FORMAT_RGB888 && outImg->depth == IMAGE_DEPTH_U24);

    const uint16_t *in = (const uint16_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

    for (uint32_t i = 0; i < inImg->size; ++i)
    {
        uint8_t r, g, b;
        RGB565_TO_RGB888(in[i], r, g, b);
        out[i * 3] = r;
        out[i * 3 + 1] = g;
        out[i * 3 + 2] = b;
    }
}

// ----------- RGB888 → YUV ------------
static void rgb888_to_yuv(const Image *inImg, Image *outImg)
{
    assert(inImg->format == IMAGE_FORMAT_RGB888 && outImg->format == IMAGE_FORMAT_YUV);

    const uint8_t *in = (const uint8_t *)inImg->pixels;
    uint8_t *out = (uint8_t *)outImg->pixels;

#define CLIP(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))

    for (uint32_t i = 0; i < inImg->size; ++i)
    {
        uint8_t r = in[i * 3];
        uint8_t g = in[i * 3 + 1];
        uint8_t b = in[i * 3 + 2];

        int y = (int)(roundf(0.299f * r + 0.587f * g + 0.114f * b));
        int u = (int)(roundf(-0.14713f * r - 0.28886f * g + 0.436f * b)) + 128;
        int v = (int)(roundf(0.615f * r - 0.51499f * g - 0.10001f * b)) + 128;

        out[i * 3] = (uint8_t)CLIP(y);
        out[i * 3 + 1] = (uint8_t)CLIP(u);
        out[i * 3 + 2] = (uint8_t)CLIP(v);
    }
}

// ----------- YUV → RGB888 ------------
static void yuv_to_rgb888(const Image *inImg, Image *outImg)
{
    assert(inImg->format == IMAGE_FORMAT_YUV && outImg->format == IMAGE_FORMAT_RGB888);

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

// ----------- RGB888 → HSI ------------
static void rgb888_to_HSI(const Image *inImg, Image *outImg)
{
    assert(inImg->format == IMAGE_FORMAT_RGB888 && outImg->format == IMAGE_FORMAT_HSI);

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

        float h = 0.0f, s, v = max;

        if (delta != 0)
        {
            if (max == r)
                h = 60 * fmodf((g - b) / delta, 6.0f);
            else if (max == g)
                h = 60 * ((b - r) / delta + 2);
            else
                h = 60 * ((r - g) / delta + 4);
            if (h < 0)
                h += 360.0f;
        }

        s = (max == 0) ? 0 : (delta / max);

        out[i * 3] = (uint8_t)(h / 360.0f * 255.0f);
        out[i * 3 + 1] = (uint8_t)(s * 255.0f);
        out[i * 3 + 2] = (uint8_t)(v * 255.0f);
    }
}

// ----------- HSI → RGB888 ------------
static void HSI_to_rgb888(const Image *inImg, Image *outImg)
{
    assert(inImg->format == IMAGE_FORMAT_HSI && outImg->format == IMAGE_FORMAT_RGB888);

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

// ----------- RGB565 → HSI ------------
static void rgb565_to_HSI(const Image *inImg, Image *outImg)
{
    assert(inImg->format == IMAGE_FORMAT_RGB565);
    assert(outImg->format == IMAGE_FORMAT_HSI);
    assert(inImg->depth == IMAGE_DEPTH_U8 && outImg->depth == IMAGE_DEPTH_U8);

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

        float h = 0.0f, s = 0.0f, v = max;

        if (delta > 0.0001f)
        {
            if (max == rf)
                h = 60.0f * fmodf(((gf - bf) / delta), 6.0f);
            else if (max == gf)
                h = 60.0f * (((bf - rf) / delta) + 2.0f);
            else // max == bf
                h = 60.0f * (((rf - gf) / delta) + 4.0f);

            if (h < 0.0f)
                h += 360.0f;

            s = delta / max;
        }

        out[i * 3] = (uint8_t)(h / 360.0f * 255.0f);
        out[i * 3 + 1] = (uint8_t)(s * 255.0f);
        out[i * 3 + 2] = (uint8_t)(v * 255.0f);
    }
}

// ----------- HSI → RGB565 ------------
static void HSI_to_rgb565(const Image *inImg, Image *outImg)
{
    assert(inImg->format == IMAGE_FORMAT_HSI && outImg->format == IMAGE_FORMAT_RGB565);

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

        uint8_t r = (uint8_t)((r_ + m) * 255.0f);
        uint8_t g = (uint8_t)((g_ + m) * 255.0f);
        uint8_t b = (uint8_t)((b_ + m) * 255.0f);

        out[i] = RGB888_TO_RGB565(r, g, b);
    }
}

void cvtColor(const Image *inImg, Image *outImg, ColorConversionCode code)
{
    assert(inImg && outImg);

    switch (code)
    {
    // ------------------- RGB888 TO -------------------
    case CVT_RGB888_TO_GRAYSCALE:
        rgb888_to_grayscale(inImg, outImg);
        break;

    case CVT_RGB888_TO_RGB565:
        rgb888_to_rgb565(inImg, outImg);
        break;

    case CVT_RGB888_TO_YUV:
        rgb888_to_yuv(inImg, outImg);
        break;

    case CVT_RGB888_TO_HSI:
        rgb888_to_HSI(inImg, outImg);
        break;

    // ------------------- RGB565 TO -------------------
    case CVT_RGB565_TO_RGB888:
        convert_rgb565_to_rgb888(inImg, outImg);
        break;

    case CVT_RGB565_TO_GRAYSCALE:
    {
        Image *tmp = (Image *)createImageWH(inImg->width, inImg->height, IMAGE_FORMAT_RGB888);
        convert_rgb565_to_rgb888(inImg, tmp);
        rgb888_to_grayscale(tmp, outImg);
        deleteImage(tmp);
        break;
    }

    case CVT_RGB565_TO_YUV:
    {
        Image *tmp = (Image *)createImageWH(inImg->width, inImg->height, IMAGE_FORMAT_RGB888);
        convert_rgb565_to_rgb888(inImg, tmp);
        rgb888_to_yuv(tmp, outImg);
        deleteImage(tmp);
        break;
    }

    case CVT_RGB565_TO_HSI:
        rgb565_to_HSI(inImg, outImg);
        break;

    // ------------------- GRAYSCALE TO -------------------
    case CVT_GRAYSCALE_TO_RGB888:
        grayscale_to_rgb(inImg, outImg);
        break;

    case CVT_GRAYSCALE_TO_RGB565:
    {
        Image *tmp = (Image *)createImageWH(inImg->width, inImg->height, IMAGE_FORMAT_RGB888);
        grayscale_to_rgb(inImg, tmp);
        rgb888_to_rgb565(tmp, outImg);
        deleteImage(tmp);
        break;
    }

    case CVT_GRAYSCALE_TO_YUV:
    {
        Image *tmp = (Image *)createImageWH(inImg->width, inImg->height, IMAGE_FORMAT_RGB888);
        grayscale_to_rgb(inImg, tmp);
        rgb888_to_yuv(tmp, outImg);
        deleteImage(tmp);
        break;
    }

    case CVT_GRAYSCALE_TO_HSI:
    {
        Image *tmp = (Image *)createImageWH(inImg->width, inImg->height, IMAGE_FORMAT_RGB888);
        grayscale_to_rgb(inImg, tmp);
        rgb888_to_HSI(tmp, outImg);
        deleteImage(tmp);
        break;
    }

    // ------------------- YUV TO -------------------
    case CVT_YUV_TO_RGB888:
        yuv_to_rgb888(inImg, outImg);
        break;

    case CVT_YUV_TO_RGB565:
    {
        Image *tmp = (Image *)createImageWH(inImg->width, inImg->height, IMAGE_FORMAT_RGB888);
        yuv_to_rgb888(inImg, tmp);
        rgb888_to_rgb565(tmp, outImg);
        deleteImage(tmp);
        break;
    }

    case CVT_YUV_TO_GRAYSCALE:
    {
        Image *tmp = (Image *)createImageWH(inImg->width, inImg->height, IMAGE_FORMAT_RGB888);
        yuv_to_rgb888(inImg, tmp);
        rgb888_to_grayscale(tmp, outImg);
        deleteImage(tmp);
        break;
    }

    case CVT_YUV_TO_HSI:
    {
        Image *tmp = (Image *)createImageWH(inImg->width, inImg->height, IMAGE_FORMAT_RGB888);
        yuv_to_rgb888(inImg, tmp);
        rgb888_to_HSI(tmp, outImg);
        deleteImage(tmp);
        break;
    }

    // ------------------- HSI TO -------------------
    case CVT_HSI_TO_RGB888:
        HSI_to_rgb888(inImg, outImg);
        break;

    case CVT_HSI_TO_RGB565:
        HSI_to_rgb565(inImg, outImg);
        break;

    case CVT_HSI_TO_GRAYSCALE:
    {
        Image *tmp = (Image *)createImageWH(inImg->width, inImg->height, IMAGE_FORMAT_RGB888);
        HSI_to_rgb888(inImg, tmp);
        rgb888_to_grayscale(tmp, outImg);
        deleteImage(tmp);
        break;
    }

    case CVT_HSI_TO_YUV:
    {
        Image *tmp = (Image *)createImageWH(inImg->width, inImg->height, IMAGE_FORMAT_RGB888);
        HSI_to_rgb888(inImg, tmp);
        rgb888_to_yuv(tmp, outImg);
        deleteImage(tmp);
        break;
    }

    // ------------------- COPY -------------------
    case CVT_COPY:
        assert(inImg->format == outImg->format);
        memcpy(outImg->pixels, inImg->pixels, inImg->size * inImg->depth);
        break;

    default:
        assert(!"Unsupported color conversion code");
        break;
    }
}

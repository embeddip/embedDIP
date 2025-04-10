#include "image.h"
#include "math.h"

#define RGB565_TO_RGB888(pixel, r, g, b) \
    do                                   \
    {                                    \
        r = ((pixel >> 11) & 0x1F) << 3; \
        g = ((pixel >> 5) & 0x3F) << 2;  \
        b = (pixel & 0x1F) << 3;         \
    } while (0)

#define RGB888_TO_RGB565(r, g, b) \
    (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

// Conversion codes
enum
{
    CVT_RGB5652GRAY,
    CVT_RGB2YUV,
    CVT_RGB2HSV,
    CVT_GRAY2RGB,
    CVT_RGB2GRAY,
    CVT_RGB2RGB565,
    CVT_RGB5652HSV,
    CVT_HSV2RGB,
    CVT_HSV2RGB565,
    CVT_RGB2YUV_ALT,
    CVT_YUV2RGB
};

void cvtColor(const Image *inImg, Image *outImg, int code);
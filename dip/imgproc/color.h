#include "image.h"
#include "math.h"
#include "common.h"
#include "assert.h"
#include "memory_manager.h"

typedef enum
{
    // RGB888 to others
    CVT_RGB888_TO_GRAYSCALE,
    CVT_RGB888_TO_RGB565,
    CVT_RGB888_TO_YUV,
    CVT_RGB888_TO_HSI,

    // RGB565 to others
    CVT_RGB565_TO_RGB888,
    CVT_RGB565_TO_GRAYSCALE,
    CVT_RGB565_TO_YUV,
    CVT_RGB565_TO_HSI,

    // GRAYSCALE to others
    CVT_GRAYSCALE_TO_RGB888,
    CVT_GRAYSCALE_TO_RGB565,
    CVT_GRAYSCALE_TO_YUV,
    CVT_GRAYSCALE_TO_HSI,

    // YUV to others
    CVT_YUV_TO_RGB888,
    CVT_YUV_TO_RGB565,
    CVT_YUV_TO_GRAYSCALE,
    CVT_YUV_TO_HSI,

    // HSI to others
    CVT_HSI_TO_RGB888,
    CVT_HSI_TO_RGB565,
    CVT_HSI_TO_GRAYSCALE,
    CVT_HSI_TO_YUV,

    // Identity (copy)
    CVT_COPY
} ColorConversionCode;

/**
 * @brief Converts the color format of the input image to a different format.
 *
 * @param[in]  inImg Pointer to the input image structure
 * @param[out] outImg Pointer to the output image structure
 * @param[in]  code Conversion code defined in ColorConversionCode enum
 */
void cvtColor(const Image *inImg, Image *outImg, ColorConversionCode code);
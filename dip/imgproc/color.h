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
typedef enum {
    CVT_RGB565_TO_GRAY,      /**< Convert from RGB565 to Grayscale */
    CVT_RGB_TO_YUV,          /**< Convert from RGB888 to YUV */
    CVT_RGB_TO_HSV,          /**< Convert from RGB888 to HSV */
    CVT_GRAY_TO_RGB,         /**< Convert from Grayscale to RGB888 */
    CVT_RGB_TO_GRAY,         /**< Convert from RGB888 to Grayscale */
    CVT_RGB_TO_RGB565,       /**< Convert from RGB888 to RGB565 */
    CVT_RGB565_TO_HSV,       /**< Convert from RGB565 to HSV */
    CVT_HSV_TO_RGB,          /**< Convert from HSV to RGB888 */
    CVT_HSV_TO_RGB565,       /**< Convert from HSV to RGB565 */
    CVT_RGB_TO_YUV_ALT,      /**< Alternate RGB888 to YUV conversion */
    CVT_YUV_TO_RGB,          /**< Convert from YUV to RGB888 */
    CVT_RGB565_TO_RGB,       /**< Convert from RGB565 to RGB888 */
    CVT_RGB888_TO_RGB565,    /**< Convert from RGB888 to RGB565 (alias of CVT_RGB_TO_RGB565) */
    CVT_RGB888_TO_GRAY,      /**< Convert from RGB888 to Grayscale (alias of CVT_RGB_TO_GRAY) */
    CVT_RGB888_TO_HSV,       /**< Convert from RGB888 to HSV (alias of CVT_RGB_TO_HSV) */
    CVT_RGB888_TO_YUV,       /**< Convert from RGB888 to YUV (alias of CVT_RGB_TO_YUV) */
    CVT_RGB888_TO_YUV_ALT,   /**< Alternate method (alias of CVT_RGB_TO_YUV_ALT) */
    CVT_YUV_TO_RGB888,       /**< Convert from YUV to RGB888 (alias of CVT_YUV_TO_RGB) */
    CVT_HSV_TO_RGB888,       /**< Convert from HSV to RGB888 (alias of CVT_HSV_TO_RGB) */
    CVT_HSV_TO_RGB565_ALT,   /**< Alternative method from HSV to RGB565 if any */
    CVT_RGB888_TO_RGB565_ALT /**< Alternate method if RGB565 encoding varies */
} ColorConversionCode;

/**
 * @brief Converts the color format of the input image to a different format.
 *
 * @param[in]  inImg Pointer to the input image structure
 * @param[out] outImg Pointer to the output image structure
 * @param[in]  code Conversion code defined in ColorConversionCode enum
 */
void cvtColor(const Image *inImg, Image *outImg, ColorConversionCode code);
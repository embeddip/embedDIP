#ifndef PIXEL_H
#define PIXEL_H

#include "core/image.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        float gamma;
        float c;
    } PTContext;

    typedef struct
    {
        float rho;
        float theta;
        int votes;
    } HoughLine;

    typedef struct
    {
        uint8_t *data;  // kernel[i * size + j], row-major
        uint8_t size;   // kernel is square (size x size)
        uint8_t anchor; // usually size / 2
    } Kernel;

    typedef enum
    {
        MORPH_RECT = 0,   ///< Rectangle shape (full kernel)
        MORPH_CROSS = 1,  ///< Cross-shaped kernel (vertical and horizontal line)
        MORPH_ELLIPSE = 2 ///< Elliptical (rounded) shape
    } MorphShape;

    void negative(const Image *in, Image *out);

    void powerTransform(const Image *inImg, Image *outImg, float gamma);

    void convertScaleAbs(const Image *inImg, Image *outImg, float alpha, float beta);

    void piecewiseTransform(const Image *inImg, Image *outImg,
                            const uint8_t *breakpoints, const uint8_t *values,
                            int numPoints);

    void grayscaleThreshold(const Image *inImg, Image *outImg, uint8_t threshold);

    uint8_t OtsuThreshold(const uint8_t *imgData, int size);
    void grayscaleOtsu(const Image *inImg, Image *outImg);

    void grayscaleThresholdLocalOtsu(const Image *inImg, Image *outImg, int blockSize);

    void grayscaleKMeans(const Image *inImg, Image *outImg, int k);

    void grayscaleRegionGrowing(const Image *inImg, Image *outImg, int seedX, int seedY, uint8_t tolerance);

    void colorRegionGrowing(const Image *inImg, Image *outMask, int seedX, int seedY, float tolerance);

    void houghTransform(const Image *edgeImg, int **accumulator, int numRho, int numTheta,
                        float rhoRes, float thetaRes);

    int extractLines(int **accumulator, int numRho, int numTheta, float rhoRes, float thetaRes,
                     int threshold, float rhoMax, HoughLine *lines, int maxLines);

    void drawLine(Image *img, int x0, int y0, int x1, int y1, uint8_t color);

    void drawLineOnImage(Image *img, float rho, float theta, uint8_t color);

    void connectedComponents(const Image *inImg, Image *outImg);

    void getStructuringElement(Kernel *kernel, MorphShape shape, uint8_t size);

    void erode(const Image *src, Image *dst, const Kernel *kernel, uint8_t iterations);

    void dilate(const Image *src, Image *dst, const Kernel *kernel, uint8_t iterations);

    void opening(const Image *inImg, Image *outImg, const Kernel *kernel, uint8_t iterations);

    void closing(const Image *inImg, Image *outImg, const Kernel *kernel, uint8_t iterations);

    void _and(const Image *img1, const Image *img2, Image *outImg);

    void _or(const Image *img1, const Image *img2, Image *outImg);

    void _xor(const Image *img1, const Image *img2, Image *outImg);

    void _not(const Image *inImg, Image *outImg);

    void grabCutLite(Image *inImg, Image *outImg, Rect roi, int iterations);

    void grabCutLite888(Image *inImg, Image *outImg, Rect roi, int iterations);

    void grabCutLite_working(Image *inImg, Image *maskImg, int iterations);

    void grabCutGrayscaleRealistic(const Image *inImg, Image *outMask, Rect roi, int max_iter);

    void grabCutRGB(const Image *inImg, Image *outMask, Rect roi, int max_iter);

    /**
     * @brief Segments an HSI image using K-means clustering.
     *
     * This function applies K-means clustering to an HSI image by treating each pixel
     * as a 3D vector (Hue, Saturation, Intensity). It assigns each pixel to a cluster
     * and sets the output to the corresponding cluster center color.
     *
     * @param[in]  inImg   Pointer to input HSI image.
     * @param[out] outImg  Pointer to output segmented HSI image.
     * @param[in]  k       Number of clusters.
     */
    void colorKMeans(const Image *inImg, Image *outImg, int k);

#ifdef __cplusplus
}
#endif

#endif // PIXEL_H

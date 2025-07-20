#ifndef PIXEL_H
#define PIXEL_H

#include "image.h"

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

    void powerTransform(const Image *inImg, Image *outImg, float gamma, float c);

    void convertScaleAbs(const Image *inImg, Image *outImg, float alpha, float beta);

    void piecewiseTransform(const Image *inImg, Image *outImg,
                            const uint8_t *breakpoints, const uint8_t *values,
                            int numPoints);

    void grayscaleThreshold(const Image *inImg, Image *outImg, uint8_t threshold);

    uint8_t computeOtsuThreshold(const uint8_t *imgData, int size);
    void grayscaleOtsu(const Image *inImg, Image *outImg);

    void grayscaleThresholdLocalOtsu(const Image *inImg, Image *outImg, int blockSize);

    void grayscaleKMeans(const Image *inImg, Image *outImg, int k);

    void grayscaleRegionGrowing(const Image *inImg, Image *outImg, int seedX, int seedY, uint8_t tolerance);

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

#ifdef __cplusplus
}
#endif

#endif // PIXEL_H

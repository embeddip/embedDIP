#ifndef PIXEL_H
#define PIXEL_H

#include "core/image.h"
#include "core/memory_manager.h"
#include <board/common.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <float.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        int x;
        int y;
    } Point;

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
        MORPH_RECT = 0,   ///< Rectangleangle shape (full kernel)
        MORPH_CROSS = 1,  ///< Cross-shaped kernel (vertical and horizontal line)
        MORPH_ELLIPSE = 2 ///< Elliptical (rounded) shape
    } MorphShape;

    int negative(const Image *in, Image *out);

    int powerTransform(const Image *inImg, Image *outImg, float gamma);

    int convertScaleAbs(const Image *inImg, Image *outImg, float alpha, float beta);

    int piecewiseTransform(const Image *inImg, Image *outImg,
                            const uint8_t *breakpoints, const uint8_t *values,
                            int numPoints);

    int grayscaleThreshold(const Image *inImg, Image *outImg, uint8_t threshold);

    uint8_t OtsuThreshold(const uint8_t *imgData, int size);
    int grayscaleOtsu(const Image *inImg, Image *outImg);

    int grayscaleThresholdLocalOtsu(const Image *inImg, Image *outImg, int blockSize);

    int grayscaleKMeans(const Image *inImg, Image *outImg, int k);

    embeddip_status_t grayscaleRegionGrowing(const Image *inImg,
                                              Image *outImg,
                                              const Point *seeds,
                                              int numSeeds,
                                              uint8_t tolerance);

    embeddip_status_t colorRegionGrowing_single(const Image *inImg, Image *outMask, int seedX, int seedY, float tolerance);

    embeddip_status_t colorRegionGrowing(const Image *inImg,
                                          Image *outImg,
                                          const Point *seeds,
                                          int numSeeds,
                                          float tolerance);

    embeddip_status_t colorKMeans(const Image *inImg, Image *outImg, int k);

    int houghTransform(const Image *edgeImg, int **accumulator, int numRho, int numTheta,
                        float rhoRes, float thetaRes);

    int extractLines(int **accumulator, int numRho, int numTheta, float rhoRes, float thetaRes,
                     int threshold, float rhoMax, HoughLine *lines, int maxLines);

    int drawLine(Image *img, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint8_t color);

    int drawLineOnImage(Image *img, float rho, float theta, uint8_t color);

    int connectedComponents(const Image *inImg, Image *outImg);

    int extractComponent(const Image *labeledImg, Image *objImg, int targetLabel);

    embeddip_status_t getStructuringElement(Kernel *kernel, MorphShape shape, uint8_t size);

    embeddip_status_t erode(const Image *src, Image *dst, const Kernel *kernel, uint8_t iterations);

    embeddip_status_t dilate(const Image *src, Image *dst, const Kernel *kernel, uint8_t iterations);

    embeddip_status_t opening(const Image *inImg, Image *outImg, const Kernel *kernel, uint8_t iterations);

    embeddip_status_t closing(const Image *inImg, Image *outImg, const Kernel *kernel, uint8_t iterations);

    embeddip_status_t _and_(const Image *img1, const Image *img2, Image *outImg);

    int _or(const Image *img1, const Image *img2, Image *outImg);

    int _xor(const Image *img1, const Image *img2, Image *outImg);

    int _not(const Image *inImg, Image *outImg);

    embeddip_status_t grabCutLite(Image *inImg, Image *outImg, Rectangle roi, int iterations);

    int grabCutLite888(Image *inImg, Image *outImg, Rectangle roi, int iterations);

    int grabCutLite_working(Image *inImg, Image *maskImg, int iterations);

    int grabCutGrayscaleRealistic(const Image *inImg, Image *outMask, Rectangle roi, int max_iter);

    int grabCutRGB(const Image *inImg, Image *outMask, Rectangle roi, int max_iter);

    /**
     * @brief Segments an HSI image using K-means clustering (old implementation).
     *
     * This function applies K-means clustering to an HSI image by treating each pixel
     * as a 3D vector (Hue, Saturation, Intensity). It assigns each pixel to a cluster
     * and sets the output to the corresponding cluster center color.
     *
     * @param[in]  inImg   Pointer to input HSI image.
     * @param[out] outImg  Pointer to output segmented HSI image.
     * @param[in]  k       Number of clusters.
     */
    embeddip_status_t colorKMeans_old(const Image *inImg, Image *outImg, int k);

    int resize(Image *inImg, Image *outImg, int outWidth, int outHeight);

    int dist(const Image *inImg, Image *outImg, uint8_t R_ref, uint8_t G_ref, uint8_t B_ref);

    int add(const Image *img1, const Image *img2, Image *outImg);

    int normalize(Image *inImg);

    /**
     * @brief Converts raw pixel data to high-precision floating-point channels.
     *
     * This function allocates and fills `chals` from raw `pixels` depending on format and depth.
     *
     * @param inImg Pointer to the image whose pixels will be converted.
     */
    int convertTo(Image *inImg);

    int _or(const Image *a, const Image *b, Image *out);

#ifdef __cplusplus
}
#endif

#endif // PIXEL_H

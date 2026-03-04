#ifndef FILTER_H
#define FILTER_H

#include "core/image.h"
#include <core/memory_manager.h>
#include <board/common.h>
#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        int size;
        float *kernel; // Flat array: size x size
        int chal;
    } Filter2DContext;

    typedef struct
    {
        int sizeX;
        const float *kernelX;
        int sizeY;
        const float *kernelY;
        float delta;
    } SepFilter2DContext;

    void filter2D_single_channel(Image *inImg, Image *outImg, int ch_idx, void *ctx);

    void filter2D_separable(Image *inImg, Image *outImg, int sizeX, float *kernelX, int sizeY, float *kernelY, float delta);

    void minFilter(const Image *inImg, Image *outImg, int kernelSize);

    void maxFilter(const Image *inImg, Image *outImg, int kernelSize);

    void medianFilter(const Image *inImg, Image *outImg, int kernelSize);

    void rgbSplit(const Image *inImg, Image *rImg, Image *gImg, Image *bImg);

    void rgbMerge(const Image *rImg, const Image *gImg, const Image *bImg, Image *outImg);

    void dogFilter(const Image *inImg, Image *outImg, float sigma1, float sigma2);

    void logFilter(const Image *inImg, Image *outImg, float sigma);

    void gaussianGradients(const Image *inImg, Image *outIx, Image *outIy, float sigma);

    void gradientMagnitude(const Image *IxImg, const Image *IyImg, Image *outMag);

    void gradientPhase(const Image *IxImg, const Image *IyImg, Image *outPhase);

    void Canny(const Image *inImg, Image *outImg, double threshold1, double threshold2, int apertureSize, bool L2gradient);
#ifdef __cplusplus
}
#endif

#endif // FILTER_H

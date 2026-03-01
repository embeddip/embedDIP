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

    int filter2D_single_channel(Image *inImg, Image *outImg, int ch_idx, void *ctx);

    int filter2D(const Image *inImg, Image *outImg, const float *kernel, int kernelSize);

    int sepfilter2D(Image *inImg, Image *outImg, int sizeX, float *kernelX, int sizeY, float *kernelY, float delta);

    int minFilter(const Image *inImg, Image *outImg, int kernelSize);

    int maxFilter(const Image *inImg, Image *outImg, int kernelSize);

    int medianFilter(const Image *inImg, Image *outImg, int kernelSize);

    int rgbSplit(const Image *inImg, Image *rImg, Image *gImg, Image *bImg);

    int rgbMerge(const Image *rImg, const Image *gImg, const Image *bImg, Image *outImg);

    int dogFilter(const Image *inImg, Image *outImg, float sigma1, float sigma2);

    embeddip_status_t logFilter(const Image *inImg, Image *outImg, float sigma);

    embeddip_status_t gaussianGradients(const Image *inImg, Image *outIx, Image *outIy, float sigma);

    embeddip_status_t gradientMagnitude(const Image *IxImg, const Image *IyImg, Image *outMag);

    embeddip_status_t gradientPhase(const Image *IxImg, const Image *IyImg, Image *outPhase);

    int Canny(const Image *inImg, Image *outImg, double threshold1, double threshold2, int apertureSize, bool L2gradient);
#ifdef __cplusplus
}
#endif

#endif // FILTER_H

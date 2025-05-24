#ifndef FILTER_H
#define FILTER_H

#include "image.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        int size;
        float (*kernel)[];
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
#ifdef __cplusplus
}
#endif

#endif // FILTER_H

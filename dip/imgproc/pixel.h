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

    void negative(const Image *in, Image *out);

    void powerTransform(const Image *inImg, Image *outImg, float gamma, float c);

    void convertScaleAbs(const Image *inImg, Image *outImg, float alpha, float beta);

    void piecewiseTransform(const Image *inImg, Image *outImg,
                            const uint8_t *breakpoints, const uint8_t *values,
                            int numPoints);

#ifdef __cplusplus
}
#endif

#endif // PIXEL_H

#ifndef COMMON_H
#define COMMON_H

#include "core/image.h"

#ifdef __cplusplus
extern "C"
{
#endif

    Image *createImage(ImageResolution resolution, ImageFormat format);
    Image *createImageWH(int width, int height, ImageFormat format);

    void deleteImage(Image *image);

    bool isChalsEmpty(const Image *inImg);

    void createChals(Image *inImg, uint8_t numChals);

    void Tick();

    uint32_t Tok();

#ifdef __cplusplus
}
#endif

#endif // COMMON_H
#ifndef COMMON_H
#define COMMON_H

#include "image.h"

#ifdef __cplusplus
extern "C"
{
#endif

    Image *createImage(image_resolution_t resolution, ImageFormat format);

#ifdef __cplusplus
}
#endif

#endif // COMMON_H
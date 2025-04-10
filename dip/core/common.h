#ifndef DIP_H
#define DIP_H

#include "image.h"

#ifdef __cplusplus
extern "C"
{
#endif

    Image *createImage(uint8_t size, uint8_t format);

#ifdef __cplusplus
}
#endif

#endif // DIP_H
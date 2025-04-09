#include "image.h"

void negative(const Image *inImg, Image *outImg)
{

    uint8_t *imgData = inImg->pixels_u8;
    uint8_t *outData = outImg->pixels_u8;

    for (int i = 0; i < inImg->size; ++i)
    {
        outData[i] = MAX_INTENSITY - imgData[i];
    }
}

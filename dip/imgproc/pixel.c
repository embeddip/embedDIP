#include "image.h"
#include "assert.h"

void negative(const Image *inImg, Image *outImg)
{

    uint8_t *imgData = inImg->pixels;
    uint8_t *outData = outImg->pixels;

    for (int i = 0; i < inImg->size; ++i)
    {
        outData[i] = MAX_INTENSITY - imgData[i];
    }
}
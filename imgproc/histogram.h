#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <core/image.h>
#include <board/common.h>
#include "assert.h"
#include "math.h"
#ifdef __cplusplus
extern "C"
{
#endif

    int histForm(const Image *inImg, int *histogram);

    int histEq(const Image *inImg, Image *outImg);

    int histSpec(const Image *inImg, Image *outImg, const int *targetHistogram);
#ifdef __cplusplus
}
#endif

#endif
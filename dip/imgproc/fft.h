#ifndef FFT_H
#define FFT_H

#include "arm_math.h"
#include "arm_const_structs.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <stdbool.h>
#include <limits.h>
#include <image.h>

    typedef enum
    {
        FREQ_FILTER_IDEAL_LOWPASS,
        FREQ_FILTER_GAUSSIAN_LOWPASS,
        FREQ_FILTER_IDEAL_HIGHPASS,
        FREQ_FILTER_GAUSSIAN_HIGHPASS,
        FREQ_FILTER_IDEAL_BANDPASS,
        FREQ_FILTER_GAUSSIAN_BANDPASS
    } FrequencyFilterType;

    void fourier(const Image *inImg, Image *outImg);

    void mag(const Image *inImg, Image *outImg);

    void phase(const Image *inImg, Image *outImg);

    void logImage(Image *img);

    void addScalar(Image *img, float value);

    void fftShift(float *data, int width, int height);

    void fourierInv(const Image *inImg, Image *outImg);

    void polarToCart(const Image *magnitude, const Image *phase, Image *outImg);

    void multiply(const Image *img1, const Image *img2, Image *outImg);

    void getFilter(Image *maskImg, FrequencyFilterType filterType, float cutoff1, float cutoff2);

        /// test

        static bool isValidFFTSize(int w, int h);
    int fft(const Image *inImg, Image *outImg);
    int ifft(const Image *inImg, Image *outImg);
    void _abs(const Image *fftImg, Image *magImg);
    void _phase(const Image *fftImg, Image *phaseImg);
    void fftshift(float *data, int width, int height);
    void getFilter(Image *maskImg, FrequencyFilterType filterType, float cutoff1, float cutoff2);
#ifdef __cplusplus
}
#endif

#endif // FFT_H

#ifndef FFT_H
#define FFT_H

#ifndef __FPU_PRESENT
#define __FPU_PRESENT 1
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <stdbool.h>
#include <limits.h>
#include <core/image.h>
#include <core/error.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        FREQ_FILTER_IDEAL_LOWPASS,
        FREQ_FILTER_GAUSSIAN_LOWPASS,
        FREQ_FILTER_IDEAL_HIGHPASS,
        FREQ_FILTER_GAUSSIAN_HIGHPASS,
        FREQ_FILTER_IDEAL_BANDPASS,
        FREQ_FILTER_GAUSSIAN_BANDPASS
    } FrequencyFilterType;

    embeddip_status_t fourier(const Image *inImg, Image *outImg);

    embeddip_status_t mag(const Image *inImg, Image *outImg);

    embeddip_status_t phase(const Image *inImg, Image *outImg);

    embeddip_status_t logImage(Image *img);

    embeddip_status_t addScalar(Image *img, float value);

    embeddip_status_t fftShift(float *data, int width, int height);

    embeddip_status_t fourierInv(const Image *inImg, Image *outImg);

    embeddip_status_t polarToCart(const Image *magnitude, const Image *phase, Image *outImg);

    embeddip_status_t multiply(const Image *img1, const Image *img2, Image *outImg);

    embeddip_status_t getMask(Image *maskImg, FrequencyFilterType filterType, float cutoff1, float cutoff2);

    /// test

    // static bool isValidFFTSize(int w, int h);
    embeddip_status_t fft(const Image *inImg, Image *outImg);
    embeddip_status_t ifft(const Image *inImg, Image *outImg);
    embeddip_status_t _abs_(const Image *fftImg, Image *magImg);
    embeddip_status_t _phase_(const Image *fftImg, Image *phaseImg);
    embeddip_status_t fftshift(float *data, int width, int height);
    embeddip_status_t getFilter(Image *maskImg, FrequencyFilterType filterType, float cutoff1, float cutoff2);

    embeddip_status_t ffilter2D(const Image *fftImg, const Image *filterMask, Image *outImg);

    embeddip_status_t difference(const Image *img1, const Image *img2, Image *outImg);

#ifdef __cplusplus
}
#endif

#endif // FFT_H

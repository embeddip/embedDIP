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

    embeddip_status_t _log_(Image *img);

    embeddip_status_t _add_(Image *img, float value);

    embeddip_status_t fftShift(Image *img);

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
    embeddip_status_t fftshift(Image *img);

    /**
     * @brief Creates a frequency-domain filter mask.
     * @param maskImg     Image to fill with filter values.
     * @param filterType  Type of filter (lowpass, highpass, bandpass).
     * @param cutoff1     Primary cutoff radius in PIXELS from center.
     * @param cutoff2     Secondary cutoff radius in PIXELS (bandpass only).
     * @return EMBEDDIP_OK on success.
     * @note Cutoff is in PIXELS: distance from image center.
     *       For 256×256 image: center at (128,128), max corner distance ≈181 pixels.
     *       Example: cutoff1=30 passes frequencies within 30-pixel radius (~16.6%).
     */
    embeddip_status_t getFilter(Image *maskImg, FrequencyFilterType filterType, float cutoff1, float cutoff2);

    embeddip_status_t ffilter2D(const Image *fftImg, const Image *filterMask, Image *outImg);

    embeddip_status_t difference(const Image *img1, const Image *img2, Image *outImg);

#ifdef __cplusplus
}
#endif

#endif // FFT_H

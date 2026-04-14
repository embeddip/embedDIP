// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <embedDIP_configs.h>

#ifdef TARGET_BOARD_ESP32

    #include "board/common.h"

    #include "Arduino.h"
    #include "esp_dsp.h"
    #include <esp32/rom/rtc.h>
    #include <imgproc/fft.h>

static bool isValidFFTSize(int w, int h)
{
    return (w == h) && ((w & (w - 1)) == 0);  // square and power-of-2
}

    #include <Arduino.h>  // Required for Serial on Arduino platforms

embeddip_status_t fft(const Image *inImg, Image *outImg)
{
    if (!inImg || !outImg || !inImg->pixels) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    int N = inImg->width;
    if (!isValidFFTSize(N, N)) {
        // Serial.println("[ERROR] Invalid FFT size. Only powers of 2 are supported.");
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    if (isChalsEmpty(outImg)) {
        createChalsComplex(outImg, 2);  // 2 complex channels for interleaved (Re, Im)
        outImg->is_chals = 1;
    }

    // Serial.println("[ERROR] 1pixels are null.");
    float *buf0 = outImg->chals->ch[0];
    float *buf1 = outImg->chals->ch[1];
    uint8_t *input = static_cast<uint8_t *>(inImg->pixels);
    for (int i = 0; i < N * N; i++) {
        buf0[2 * i] = (float)input[i];  // real part
        buf0[2 * i + 1] = 0.0f;         // imaginary part
    }
    // Initialize the FFT library
    dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);

    // Serial.println("[ERROR] 4or pixels are null.");
    //  FFT on rows
    for (int i = 0; i < N; i++) {
        int offset = i * N * 2;
        dsps_fft2r_fc32(buf0 + offset, N);
        dsps_bit_rev_fc32(buf0 + offset, N);
    }
    // Serial.println("[ERROR] 5or pixels are null.");
    //  Transpose to buf1
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            buf1[dst] = buf0[src];
            buf1[dst + 1] = buf0[src + 1];
        }
    }

    // FFT on columns
    for (int i = 0; i < N; i++) {
        int offset = i * N * 2;
        dsps_fft2r_fc32(buf1 + offset, N);
        dsps_bit_rev_fc32(buf1 + offset, N);
    }

    // Transpose back: buf1 → buf0 to undo the earlier transpose
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            buf0[dst] = buf1[src];
            buf0[dst + 1] = buf1[src + 1];
        }
    }

    // Copy back to buf1 for output
    for (int i = 0; i < N * N * 2; i++) {
        buf1[i] = buf0[i];
    }

    outImg->log = IMAGE_DATA_COMPLEX;
    // Serial.println("[INFO] 2D FFT completed successfully.");
    return EMBEDDIP_OK;
}

embeddip_status_t ifft(const Image *inImg, Image *outImg)
{
    int N = inImg->width;

    // Accept both IMAGE_DATA_COMPLEX and IMAGE_DATA_CH0 (match STM32 behavior)
    if (inImg->log != IMAGE_DATA_COMPLEX && inImg->log != IMAGE_DATA_CH0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    float *buf0 = (float *)ps_malloc(N * N * 2 * sizeof(float));
    // Use ch[0] if log is IMAGE_DATA_CH0, otherwise use ch[1]
    float *buf1 = (inImg->log == IMAGE_DATA_CH0) ? inImg->chals->ch[0] : inImg->chals->ch[1];

    // Conjugate input for IFFT (negate imaginary parts)
    for (int i = 0; i < N * N; i++) {
        buf1[2 * i + 1] = -buf1[2 * i + 1];
    }

    // iFFT on rows (using forward FFT on conjugated data)
    for (int row = 0; row < N; row++) {
        dsps_fft2r_fc32(buf1 + row * N * 2, N);
        dsps_bit_rev_fc32(buf1 + row * N * 2, N);
    }

    // Conjugate intermediate result
    for (int i = 0; i < N * N; i++) {
        buf1[2 * i + 1] = -buf1[2 * i + 1];
    }

    // Transpose back to buf0 (INVERSE transpose - swap src/dst compared to FFT)
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int dst = 2 * (y * N + x);
            int src = 2 * (x * N + y);
            buf0[dst] = buf1[src];
            buf0[dst + 1] = buf1[src + 1];
        }
    }

    // Conjugate before second FFT
    for (int i = 0; i < N * N; i++) {
        buf0[2 * i + 1] = -buf0[2 * i + 1];
    }

    // iFFT on columns (using forward FFT on conjugated data)
    for (int row = 0; row < N; row++) {
        dsps_fft2r_fc32(buf0 + row * N * 2, N);
        dsps_bit_rev_fc32(buf0 + row * N * 2, N);
    }

    // Conjugate output
    for (int i = 0; i < N * N; i++) {
        buf0[2 * i + 1] = -buf0[2 * i + 1];
    }

    // Transpose back: buf0 → buf1 to undo the earlier transpose
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            buf1[dst] = buf0[src];
            buf1[dst + 1] = buf0[src + 1];
        }
    }

    if (isChalsEmpty(outImg)) {
        createChals(outImg, 1);
        outImg->is_chals = 1;
    }

    float *result = outImg->chals->ch[0];
    float scale = 1.0f / (N * N);
    for (int i = 0; i < N * N; i++) {
        result[i] = buf1[2 * i] * scale;
    }

    outImg->log = IMAGE_DATA_CH0;
    free(buf0);
    return EMBEDDIP_OK;
}

embeddip_status_t _log_(Image *img)
{
    if (!img || isChalsEmpty(img))
        return EMBEDDIP_ERROR_NULL_PTR;

    float *data = img->chals->ch[0];
    for (int i = 0; i < img->size; ++i) {
        data[i] = logf(data[i] + 1e-3f);  // Avoid log(0)
    }
    return EMBEDDIP_OK;
}

embeddip_status_t _add_(Image *img, float value)
{
    if (!img || isChalsEmpty(img))
        return EMBEDDIP_ERROR_NULL_PTR;

    float *data = img->chals->ch[0];
    for (int i = 0; i < img->size; ++i) {
        data[i] += value;
    }
    return EMBEDDIP_OK;
}

embeddip_status_t fftshift(Image *img)
{
    if (!img || isChalsEmpty(img))
        return EMBEDDIP_ERROR_NULL_PTR;

    float *data = (img->log == IMAGE_DATA_COMPLEX) ? img->chals->ch[1] : img->chals->ch[0];
    int width = img->width;
    int height = img->height;

    int cx = width / 2;
    int cy = height / 2;

    for (int y = 0; y < cy; ++y) {
        for (int x = 0; x < cx; ++x) {
            int q0 = 2 * ((y * width) + x);
            int q1 = 2 * ((y * width) + x + cx);
            int q2 = 2 * (((y + cy) * width) + x);
            int q3 = 2 * (((y + cy) * width) + x + cx);

            for (int i = 0; i < 2; ++i) {
                float tmp = data[q0 + i];
                data[q0 + i] = data[q3 + i];
                data[q3 + i] = tmp;

                tmp = data[q1 + i];
                data[q1 + i] = data[q2 + i];
                data[q2 + i] = tmp;
            }
        }
    }
    return EMBEDDIP_OK;
}

embeddip_status_t _abs_(const Image *fftImg, Image *magImg)
{
    if (!fftImg || !fftImg->chals || !magImg) {
        // Serial.println("[ERROR] Input FFT image or its channels are null.");
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    int size = fftImg->width * fftImg->height;

    float *fft = (fftImg->log == IMAGE_DATA_COMPLEX) ? fftImg->chals->ch[1] : fftImg->chals->ch[0];

    if (!fft) {
        // Serial.println("[ERROR] FFT buffer is null.");
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (isChalsEmpty(magImg)) {
        createChals(magImg, 1);
        magImg->is_chals = 1;
        // Serial.println("[INFO] Output magnitude channel created.");
    }

    float *mag = magImg->chals->ch[0];
    if (!mag) {
        // Serial.println("[ERROR] Magnitude channel buffer is null.");
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    for (int i = 0; i < size; ++i) {
        float re = fft[2 * i];
        float im = fft[2 * i + 1];
        mag[i] = sqrtf(re * re + im * im);
        // Uncomment the line below for verbose per-pixel debugging
        // Serial.printf("[DEBUG] Index %d: re=%.3f, im=%.3f, mag=%.3f\n", i, re, im, mag[i]);
    }

    magImg->log = IMAGE_DATA_MAGNITUDE;
    return EMBEDDIP_OK;
}

embeddip_status_t _phase_(const Image *fftImg, Image *phaseImg)
{
    int size = fftImg->width * fftImg->height;

    float *fft = (fftImg->log == IMAGE_DATA_COMPLEX) ? fftImg->chals->ch[1] : fftImg->chals->ch[0];

    if (isChalsEmpty(phaseImg)) {
        createChals(phaseImg, 1);
        phaseImg->is_chals = 1;
    }

    float *out = phaseImg->chals->ch[0];

    for (int i = 0; i < size; ++i) {
        out[i] = atan2f(fft[2 * i + 1], fft[2 * i]);
    }

    phaseImg->log = IMAGE_DATA_PHASE;
    return EMBEDDIP_OK;
}

embeddip_status_t polarToCart(const Image *magnitude, const Image *phase, Image *outImg)
{
    int size = magnitude->width * magnitude->height;

    if (isChalsEmpty(outImg)) {
        createChalsComplex(outImg, 2);  // Need 2 channels like STM32
        outImg->is_chals = 1;
    }

    float *mag = magnitude->chals->ch[0];
    float *phs = phase->chals->ch[0];
    float *fft = outImg->chals->ch[0];

    for (int i = 0; i < size; ++i) {
        fft[2 * i] = mag[i] * cosf(phs[i]);
        fft[2 * i + 1] = mag[i] * sinf(phs[i]);
    }

    outImg->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/**
 * @brief Performs element-wise complex multiplication in frequency domain.
 *
 *
 * @param img1    First complex image
 * @param img2    Second complex image
 * @param outImg  Output complex image
 */
embeddip_status_t multiply(const Image *img1, const Image *img2, Image *outImg)
{
    // Input validation
    if (!img1 || !img2 || !outImg) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (img1->width != img2->width || img1->height != img2->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    // Allocate output if needed
    if (isChalsEmpty(outImg)) {
        createChals(outImg, 1);
        outImg->is_chals = 1;
    }

    float *in1 = NULL, *in2 = NULL;

    // Select input channel based on log state
    if (img1->log == IMAGE_DATA_CH0) {
        in1 = (float *)img1->chals->ch[0];
    } else if (img1->log == IMAGE_DATA_COMPLEX) {
        in1 = (float *)img1->chals->ch[1];
    } else if (img1->log == IMAGE_DATA_PIXELS) {
        in1 = (float *)img1->pixels;
    } else {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    if (img2->log == IMAGE_DATA_CH0) {
        in2 = (float *)img2->chals->ch[0];
    } else if (img2->log == IMAGE_DATA_COMPLEX) {
        in2 = (float *)img2->chals->ch[1];
    } else if (img2->log == IMAGE_DATA_PIXELS) {
        in2 = (float *)img2->pixels;
    } else {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    float *out = (float *)outImg->chals->ch[0];

    int size = img1->width * img1->height;
    for (int i = 0; i < size; ++i) {
        out[i] = in1[i] * in2[i];
    }

    outImg->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/**
 * @brief Computes pixel-wise difference between two images: out = img1 - img2.
 *
 * Both images must have the same dimensions and a single channel.
 *
 * @param[in]  img1    First input image.
 * @param[in]  img2    Second input image.
 * @param[out] outImg  Output image to store the difference.
 */
/**
 * @brief Computes pixel-wise difference: out = img1 - img2 (clamped to >= 0).
 *
 * Optimized for performance: checks image types once, then uses fast loops.
 *
 * @param[in]  img1    First image (original).
 * @param[in]  img2    Second image (to subtract).
 * @param[out] outImg  Output image (difference).
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t difference(const Image *img1, const Image *img2, Image *outImg)
{
    if (!img1 || !img2 || !outImg)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (img1->width != img2->width || img1->height != img2->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;

    int size = img1->width * img1->height;

    // Allocate output channel if needed
    if (isChalsEmpty(outImg)) {
        embeddip_status_t status = createChals(outImg, 1);
        if (status != EMBEDDIP_OK)
            return status;
        outImg->is_chals = 1;
    }

    float *out = outImg->chals->ch[0];
    if (!out)
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    // Check types ONCE, then use optimized loops (no conditionals inside loop)
    // Most common case: img1=PIXELS, img2=CH0 (your use case)
    if (img1->log == IMAGE_DATA_PIXELS &&
        (img2->log == IMAGE_DATA_CH0 || img2->log == IMAGE_DATA_MAGNITUDE)) {
        if (!img1->pixels || !img2->chals || !img2->chals->ch[0])
            return EMBEDDIP_ERROR_NULL_PTR;

        uint8_t *pix1 = (uint8_t *)img1->pixels;
        float *ch2 = img2->chals->ch[0];
        for (int i = 0; i < size; ++i)
            out[i] = fmaxf((float)pix1[i] - ch2[i], 0.0f);
    }
    // Both pixels
    else if (img1->log == IMAGE_DATA_PIXELS && img2->log == IMAGE_DATA_PIXELS) {
        if (!img1->pixels || !img2->pixels)
            return EMBEDDIP_ERROR_NULL_PTR;

        uint8_t *pix1 = (uint8_t *)img1->pixels;
        uint8_t *pix2 = (uint8_t *)img2->pixels;
        for (int i = 0; i < size; ++i)
            out[i] = fmaxf((float)(pix1[i] - pix2[i]), 0.0f);
    }
    // Both channels
    else if ((img1->log == IMAGE_DATA_CH0 || img1->log == IMAGE_DATA_MAGNITUDE) &&
             (img2->log == IMAGE_DATA_CH0 || img2->log == IMAGE_DATA_MAGNITUDE)) {
        if (!img1->chals || !img1->chals->ch[0] || !img2->chals || !img2->chals->ch[0])
            return EMBEDDIP_ERROR_NULL_PTR;

        float *ch1 = img1->chals->ch[0];
        float *ch2 = img2->chals->ch[0];
        for (int i = 0; i < size; ++i)
            out[i] = fmaxf(ch1[i] - ch2[i], 0.0f);
    }
    // img1=CH0, img2=PIXELS
    else if ((img1->log == IMAGE_DATA_CH0 || img1->log == IMAGE_DATA_MAGNITUDE) &&
             img2->log == IMAGE_DATA_PIXELS) {
        if (!img1->chals || !img1->chals->ch[0] || !img2->pixels)
            return EMBEDDIP_ERROR_NULL_PTR;

        float *ch1 = img1->chals->ch[0];
        uint8_t *pix2 = (uint8_t *)img2->pixels;
        for (int i = 0; i < size; ++i)
            out[i] = fmaxf(ch1[i] - (float)pix2[i], 0.0f);
    } else {
        // Unsupported combination
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    outImg->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

/**
 * @brief Creates a frequency-domain filter mask.
 *
 * Generates circular/radial filters centered at the image center for frequency domain filtering.
 * The filter is created in spatial frequency coordinates where the center represents DC (zero
 * frequency).
 *
 * @param[in,out] maskImg    Image to fill with filter values (must have width/height already set).
 *                           Creates ch[0] channel if needed and sets log = IMAGE_DATA_CH0.
 * @param[in]     filterType Type of filter (lowpass, highpass, bandpass, etc.).
 * @param[in]     cutoff1    Primary cutoff radius in PIXELS from center.
 *                           - For lowpass: frequencies within this radius pass (1.0), outside block
 * (0.0)
 *                           - For highpass: frequencies outside this radius pass (1.0), inside
 * block (0.0)
 *                           - For bandpass: inner radius (with cutoff2 as outer radius)
 *                           - For Gaussian filters: standard deviation of the Gaussian
 * @param[in]     cutoff2    Secondary cutoff radius in PIXELS (only used for bandpass filters).
 *                           Must satisfy: cutoff1 < cutoff2
 *
 * @return EMBEDDIP_OK on success, error code otherwise.
 *
 * @note Cutoff units are PIXELS measured as Euclidean distance from image center.
 *       For a 256×256 image:
 *       - Center is at (128, 128)
 *       - Max distance to corner ≈ 181 pixels
 *       - cutoff1=30 means frequencies within 30-pixel radius from center
 *       - This corresponds to ~16.6% of max frequency (30/181)
 *
 * @note Filter values range from 0.0 (block) to 1.0 (pass).
 *       Ideal filters produce hard edges (0 or 1).
 *       Gaussian filters produce smooth transitions.
 *
 * @example
 *   // Low-pass: pass low frequencies (smooth, blur effect)
 *   getFilter(mask, FREQ_FILTER_IDEAL_LOWPASS, 30, 0);
 *
 *   // High-pass: pass high frequencies (edges, details)
 *   getFilter(mask, FREQ_FILTER_IDEAL_HIGHPASS, 50, 0);
 *
 *   // Band-pass: pass frequencies between 20-60 pixels from center
 *   getFilter(mask, FREQ_FILTER_IDEAL_BANDPASS, 20, 60);
 */
embeddip_status_t
getFilter(Image *maskImg, FrequencyFilterType filterType, float cutoff1, float cutoff2)
{
    if (!maskImg)
        return EMBEDDIP_ERROR_NULL_PTR;

    // Validate cutoff values
    if (cutoff1 < 0.0f)
        return EMBEDDIP_ERROR_INVALID_ARG;

    if (filterType == FREQ_FILTER_IDEAL_BANDPASS || filterType == FREQ_FILTER_GAUSSIAN_BANDPASS) {
        if (cutoff2 < 0.0f || cutoff1 >= cutoff2)
            return EMBEDDIP_ERROR_INVALID_ARG;
    }

    int w = maskImg->width;
    int h = maskImg->height;
    int cx = w / 2;
    int cy = h / 2;

    maskImg->format = IMAGE_FORMAT_GRAYSCALE;

    if (isChalsEmpty(maskImg)) {
        createChals(maskImg, 1);
        maskImg->is_chals = 1;
    }

    float *mask = maskImg->chals->ch[0];

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int dx = x - cx;
            int dy = y - cy;
            float d = sqrtf((float)(dx * dx + dy * dy));

            float value = 0.0f;

            switch (filterType) {
            case FREQ_FILTER_IDEAL_LOWPASS:
                value = (d <= cutoff1) ? 1.0f : 0.0f;
                break;

            case FREQ_FILTER_GAUSSIAN_LOWPASS:
                value = expf(-(d * d) / (2.0f * cutoff1 * cutoff1));
                break;

            case FREQ_FILTER_IDEAL_HIGHPASS:
                value = (d >= cutoff1) ? 1.0f : 0.0f;
                break;

            case FREQ_FILTER_GAUSSIAN_HIGHPASS:
                value = 1.0f - expf(-(d * d) / (2.0f * cutoff1 * cutoff1));
                break;

            case FREQ_FILTER_IDEAL_BANDPASS:
                value = (d >= cutoff1 && d <= cutoff2) ? 1.0f : 0.0f;
                break;

            case FREQ_FILTER_GAUSSIAN_BANDPASS: {
                float gLow = expf(-(d * d) / (2.0f * cutoff2 * cutoff2));
                float gHigh = expf(-(d * d) / (2.0f * cutoff1 * cutoff1));
                value = gLow - gHigh;
                break;
            }

            default:
                value = 0.0f;
                break;
            }

            mask[y * w + x] = value;
        }
    }

    maskImg->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

embeddip_status_t ffilter2D(const Image *fftImg, const Image *filterMask, Image *outImg)
{
    if (!fftImg || !filterMask || !outImg || isChalsEmpty(fftImg) || isChalsEmpty(filterMask))
        return EMBEDDIP_ERROR_NULL_PTR;

    int width = fftImg->width;
    int height = fftImg->height;
    int size = width * height;

    // Create magnitude and phase containers
    Image *magImg = NULL;
    Image *phaseImg = NULL;
    createImageWH(width, height, IMAGE_FORMAT_GRAYSCALE, &magImg);
    createImageWH(width, height, IMAGE_FORMAT_GRAYSCALE, &phaseImg);

    _abs_(fftImg, magImg);
    _phase_(fftImg, phaseImg);

    float *mag = magImg->chals->ch[0];
    float *mask = filterMask->chals->ch[0];

    for (int i = 0; i < size; ++i)
        mag[i] *= mask[i];

    polarToCart(magImg, phaseImg, outImg);
    return EMBEDDIP_OK;
}
#endif

#include <embedDIP_configs.h>

#ifdef TARGET_BOARD_ESP32

#include <imgproc/fft.h>

#include "esp_dsp.h"
#include "Arduino.h"
#include <esp32/rom/rtc.h>
#include "board/common.h"

static bool isValidFFTSize(int w, int h)
{
    return (w == h) && ((w & (w - 1)) == 0); // square and power-of-2
}

#include <Arduino.h> // Required for Serial on Arduino platforms

int fft(const Image *inImg, Image *outImg)
{
    int N = inImg->width;
    if (!isValidFFTSize(N, N))
    {
        // Serial.println("[ERROR] Invalid FFT size. Only powers of 2 are supported.");
        return -1;
    }

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 2);
        outImg->is_chals = 1;
    }

    // Serial.println("[ERROR] 1pixels are null.");
    float *buf0 = outImg->chals->ch[0];
    float *buf1 = outImg->chals->ch[1];
    // Serial.println("[ERROR] 2or pixels are null.");
    if (!inImg || !inImg->pixels)
    {
        // Serial.println("[ERROR]3 or pixels are null.");
        return -3;
    }

    uint8_t *input = static_cast<uint8_t *>(inImg->pixels);
    for (int i = 0; i < N * N; i++)
    {
        buf0[2 * i] = (float)input[i]; // real part
        buf0[2 * i + 1] = 0.0f;        // imaginary part
    }
    // Initialize the FFT library
    dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);

    // Serial.println("[ERROR] 4or pixels are null.");
    //  FFT on rows
    for (int i = 0; i < N; i++)
    {
        int offset = i * N * 2;
        dsps_fft2r_fc32(buf0 + offset, N);
        dsps_bit_rev_fc32(buf0 + offset, N);
    }
    // Serial.println("[ERROR] 5or pixels are null.");
    //  Transpose to buf1
    for (int y = 0; y < N; y++)
    {
        for (int x = 0; x < N; x++)
        {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            buf1[dst] = buf0[src];
            buf1[dst + 1] = buf0[src + 1];
        }
    }

    // FFT on columns
    for (int i = 0; i < N; i++)
    {
        int offset = i * N * 2;
        dsps_fft2r_fc32(buf1 + offset, N);
        dsps_bit_rev_fc32(buf1 + offset, N);
    }

    outImg->log = IMAGE_DATA_COMPLEX;
    // Serial.println("[INFO] 2D FFT completed successfully.");
    return 0;
}

int ifft(const Image *inImg, Image *outImg)
{
    int N = inImg->width;

    if (inImg->log != IMAGE_DATA_COMPLEX)
        return -1;

    float *buf0 = (float *)ps_malloc(N * N * 2 * sizeof(float));
    float *buf1 = inImg->chals->ch[1];

    // iFFT on rows
    for (int row = 0; row < N; row++)
    {
        dsps_fft2r_fc32(buf1 + row * N * 2, N);
        dsps_bit_rev_fc32(buf1 + row * N * 2, N);
    }

    // Transpose back to buf0
    for (int y = 0; y < N; y++)
    {
        for (int x = 0; x < N; x++)
        {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            buf0[dst] = buf1[src];
            buf0[dst + 1] = buf1[src + 1];
        }
    }

    // iFFT on columns
    for (int row = 0; row < N; row++)
    {
        dsps_fft2r_fc32(buf0 + row * N * 2, N);
        dsps_bit_rev_fc32(buf0 + row * N * 2, N);
    }

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 1);
        outImg->is_chals = 1;
    }

    float *result = outImg->chals->ch[0];
    float scale = 1.0f / (N * N);
    for (int i = 0; i < N * N; i++)
    {
        result[i] = buf0[2 * i] * scale;
    }

    outImg->log = IMAGE_DATA_CH0;
    free(buf0);
    return 0;
}

void logImage(Image *img)
{
    if (!img || isChalsEmpty(img))
        return;

    float *data = img->chals->ch[0];
    for (int i = 0; i < img->size; ++i)
    {
        data[i] = logf(data[i] + 1e-3f); // Avoid log(0)
    }
}

void addScalar(Image *img, float value)
{
    if (!img || isChalsEmpty(img))
        return;

    float *data = img->chals->ch[0];
    for (int i = 0; i < img->size; ++i)
    {
        data[i] += value;
    }
}

void fftShift(float *data, int width, int height)
{
    int cx = width / 2;
    int cy = height / 2;

    for (int y = 0; y < cy; ++y)
    {
        for (int x = 0; x < cx; ++x)
        {
            int q0 = 2 * ((y * width) + x);
            int q1 = 2 * ((y * width) + x + cx);
            int q2 = 2 * (((y + cy) * width) + x);
            int q3 = 2 * (((y + cy) * width) + x + cx);

            for (int i = 0; i < 2; ++i)
            {
                float tmp = data[q0 + i];
                data[q0 + i] = data[q3 + i];
                data[q3 + i] = tmp;

                tmp = data[q1 + i];
                data[q1 + i] = data[q2 + i];
                data[q2 + i] = tmp;
            }
        }
    }
}

void _abs_(const Image *fftImg, Image *magImg)
{
    int size = fftImg->width * fftImg->height;

    if (!fftImg || !fftImg->chals)
    {
        // Serial.println("[ERROR] Input FFT image or its channels are null.");
        return;
    }

    float *fft = (fftImg->log == IMAGE_DATA_COMPLEX)
                     ? fftImg->chals->ch[1]
                     : fftImg->chals->ch[0];

    if (!fft)
    {
        // Serial.println("[ERROR] FFT buffer is null.");
        return;
    }

    if (isChalsEmpty(magImg))
    {
        createChals(magImg, 1);
        magImg->is_chals = 1;
        // Serial.println("[INFO] Output magnitude channel created.");
    }

    float *mag = magImg->chals->ch[0];
    if (!mag)
    {
        // Serial.println("[ERROR] Magnitude channel buffer is null.");
        return;
    }

    for (int i = 0; i < size; ++i)
    {
        float re = fft[2 * i];
        float im = fft[2 * i + 1];
        mag[i] = sqrtf(re * re + im * im);
        // Uncomment the line below for verbose per-pixel debugging
        // Serial.printf("[DEBUG] Index %d: re=%.3f, im=%.3f, mag=%.3f\n", i, re, im, mag[i]);
    }

    magImg->log = IMAGE_DATA_MAGNITUDE;
}

void _phase_(const Image *fftImg, Image *phaseImg)
{
    int size = fftImg->width * fftImg->height;

    float *fft = (fftImg->log == IMAGE_DATA_COMPLEX)
                     ? fftImg->chals->ch[1]
                     : fftImg->chals->ch[0];

    if (isChalsEmpty(phaseImg))
    {
        createChals(phaseImg, 1);
        phaseImg->is_chals = 1;
    }

    float *out = phaseImg->chals->ch[0];

    for (int i = 0; i < size; ++i)
    {
        out[i] = atan2f(fft[2 * i + 1], fft[2 * i]);
    }

    phaseImg->log = IMAGE_DATA_PHASE;
}

void polarToCart(const Image *magnitude, const Image *phase, Image *outImg)
{
    int size = magnitude->width * magnitude->height;

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 2);
        outImg->is_chals = 1;
    }

    float *mag = magnitude->chals->ch[0];
    float *phs = phase->chals->ch[0];
    float *fft = outImg->chals->ch[0];

    for (int i = 0; i < size; ++i)
    {
        fft[2 * i] = mag[i] * cosf(phs[i]);
        fft[2 * i + 1] = mag[i] * sinf(phs[i]);
    }

    outImg->log = IMAGE_DATA_CH0;
}

/**
 * @brief Performs element-wise complex multiplication in frequency domain.
 *
 *
 * @param img1    First complex image
 * @param img2    Second complex image
 * @param outImg  Output complex image
 */
void multiply(const Image *img1, const Image *img2, Image *outImg)
{
    if (img1->width != img2->width || img1->height != img2->height)
        return;

    if (!img1 || !img2 || !outImg)
        return;

    // Allocate output if needed
    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 1);
        outImg->is_chals = 1;
    }

    uint8_t *in1, *in2;
    float *out;
    // this should be adjusted according to the last states of the images.
    if (img1->log == IMAGE_DATA_CH0)
    {
        in1 = img1->chals->ch[0];
    }
    else if (img1->log == IMAGE_DATA_COMPLEX)
    {
        in1 = img1->chals->ch[1];
    }
    else if (img1->log == IMAGE_DATA_PIXELS)
    {
        in1 = img1->pixels;
    }

    if (img2->log == IMAGE_DATA_CH0)
    {
        in2 = img2->chals->ch[0];
    }
    else if (img2->log == IMAGE_DATA_COMPLEX)
    {
        in2 = img2->chals->ch[1];
    }
    else if (img2->log == IMAGE_DATA_PIXELS)
    {
        in2 = img2->pixels;
    }

    out = outImg->chals->ch[0];

    int size = img1->width * img1->height;
    for (int i = 0; i < size; ++i)
    {
        out[i] = in1[i] * in2[i];
    }

    outImg->log = IMAGE_DATA_CH0;
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
void difference(const Image *img1, const Image *img2, Image *outImg)
{
    if (!img1 || !img2 || !outImg ||
        img1->width != img2->width || img1->height != img2->height)
        return;

    int size = img1->width * img1->height;

    // Allocate output channel if needed
    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 1);
        outImg->is_chals = 1;
    }

    uint8_t *in1, *in2;
    float *out;
    // this should be adjusted according to the last states of the images.
    if (img1->log == IMAGE_DATA_CH0)
    {
        in1 = img1->chals->ch[0];
    }
    else if (img1->log == IMAGE_DATA_COMPLEX)
    {
        in1 = img1->chals->ch[1];
    }
    else if (img1->log == IMAGE_DATA_PIXELS)
    {
        in1 = img1->pixels;
    }

    if (img2->log == IMAGE_DATA_CH0)
    {
        in2 = img2->chals->ch[0];
    }
    else if (img2->log == IMAGE_DATA_COMPLEX)
    {
        in2 = img2->chals->ch[1];
    }
    else if (img2->log == IMAGE_DATA_PIXELS)
    {
        in2 = img2->pixels;
    }

    out = outImg->chals->ch[0];

    for (int i = 0; i < size; ++i)
    {
        out[i] = fmaxf((float)in1[i] - (float)in2[i], 0.0f);
    }
}

/**
 * @brief Fills the given image with a frequency domain filter mask.
 *
 * This modifies the image in-place. It must already have width and height set.
 *
 * @param maskImg    Target image to be filled with mask values.
 * @param filterType Type of filter to create (low-pass, high-pass, band-pass, etc.).
 * @param cutoff1    Cutoff radius (or inner radius for band-pass).
 * @param cutoff2    Outer radius for band-pass (ignored for other types).
 */
void getFilter(Image *maskImg, FrequencyFilterType filterType, float cutoff1, float cutoff2)
{
    if (!maskImg)
        return;

    int w = maskImg->width;
    int h = maskImg->height;
    int cx = w / 2;
    int cy = h / 2;

    maskImg->format = IMAGE_FORMAT_GRAYSCALE;

    if (isChalsEmpty(maskImg))
    {
        createChals(maskImg, 1);
        maskImg->is_chals = 1;
    }

    float *mask = maskImg->chals->ch[0];

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            int dx = x - cx;
            int dy = y - cy;
            float d = sqrtf((float)(dx * dx + dy * dy));

            float value = 0.0f;

            switch (filterType)
            {
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

            case FREQ_FILTER_GAUSSIAN_BANDPASS:
            {
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
}

void ffilter2D(const Image *fftImg, const Image *filterMask, Image *outImg)
{
    if (!fftImg || !filterMask || !outImg || isChalsEmpty(fftImg) || isChalsEmpty(filterMask))
        return;

    int width = fftImg->width;
    int height = fftImg->height;
    int size = width * height;

    // Create magnitude and phase containers
    Image *magImg = createImageWH(width, height, IMAGE_FORMAT_GRAYSCALE);
    Image *phaseImg = createImageWH(width, height, IMAGE_FORMAT_GRAYSCALE);

    _abs_(fftImg, magImg);
    _phase_(fftImg, phaseImg);

    float *mag = magImg->chals->ch[0];
    float *mask = filterMask->chals->ch[0];

    for (int i = 0; i < size; ++i)
        mag[i] *= mask[i];

    polarToCart(magImg, phaseImg, outImg);
}
#endif
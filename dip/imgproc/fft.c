#include <fft.h>

/**
 * @brief Computes the Fourier transform of the input image.
 *
 * @param inImg Input image.
 * @param outImg Output image (Fourier transformed).
 */
void fourier(const Image *inImg, Image *outImg)
{
    int imageN = 256;

    if (isChalsEmpty(outImg))
    {
        outImg->chals = (channels_t *)memory_alloc(sizeof(channels_t));
        outImg->is_chals = 1;
    }
    else
    {
        memory_free(outImg->chals->ch[0]);
        memory_free(outImg->chals->ch[1]);
    }

    outImg->chals->ch[0] = (float *)memory_alloc(inImg->height * inImg->width * 8);
    outImg->chals->ch[1] = (float *)memory_alloc(inImg->height * inImg->width * 8);

    float *fourier = outImg->chals->ch[0];
    float *fourier2 = outImg->chals->ch[1];

    if (isChalsEmpty(inImg))
    {
        for (int row = 0; row < imageN * imageN; row++)
        {
            fourier[2 * row] = (uint32_t)((uint8_t *)inImg->pixels)[row];
            fourier[2 * row + 1] = 0x00000000;
        }

        for (int i = 0; i < imageN; i++)
        {
            arm_cfft_f32(&arm_cfft_sR_f32_len256, fourier + imageN * i * 2, 0, 1);
        }

        for (int k = 0; k < imageN; k++)
        {
            for (int j = 0; j < imageN; j++)
            {
                fourier2[2 * j + k * imageN * 2] = (float)fourier[j * imageN * 2 + k * 2];
                fourier2[2 * j + 1 + k * imageN * 2] = (float)fourier[j * imageN * 2 + k * 2 + 1];
            }
        }

        for (int i = 0; i < imageN; i++)
        {
            arm_cfft_f32(&arm_cfft_sR_f32_len256, fourier2 + imageN * i * 2, 0, 1);
        }
    }
    else
    {

        for (int row = 0; row < imageN * imageN; row++)
        {
            fourier[2 * row] = (float)inImg->chals->ch[0][row];
            fourier[2 * row + 1] = 0x00000000;
        }

        for (int i = 0; i < imageN; i++)
        {
            arm_cfft_f32(&arm_cfft_sR_f32_len256, fourier + imageN * i * 2, 0, 1);
        }

        for (int k = 0; k < imageN; k++)
        {
            for (int j = 0; j < imageN; j++)
            {
                fourier2[2 * j + k * imageN * 2] = (float)fourier[j * imageN * 2 + k * 2];
                fourier2[2 * j + 1 + k * imageN * 2] = (float)fourier[j * imageN * 2 + k * 2 + 1];
            }
        }

        for (int i = 0; i < imageN; i++)
        {
            arm_cfft_f32(&arm_cfft_sR_f32_len256, fourier2 + imageN * i * 2, 0, 1);
        }
    }
}

/**
 * @brief Computes the magnitude of the Fourier transform.
 *
 * @param[in]  inImg   Pointer to the input image with complex Fourier data (2 channels: real + imaginary).
 * @param[out] outImg  Pointer to the output grayscale image representing the magnitude.
 */
void mag(const Image *inImg, Image *outImg)
{
    int imageN = 256;

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 1);
        outImg->is_chals = 1;
    }

    float *fft = inImg->chals->ch[1];
    float *magnitude = outImg->chals->ch[0];

    for (int i = 0; i < imageN * imageN; ++i)
    {
        float re = fft[i * 2];
        float im = fft[i * 2 + 1];
        magnitude[i] = log(sqrtf(re * re + im * im) + 1);
    }
}

/**
 * @brief Computes the phase angle of the Fourier transform.
 *
 * @param[in]  inImg   Pointer to the input image with complex Fourier data (2 channels: real + imaginary).
 * @param[out] outImg  Pointer to the output image where the phase angles will be stored (1 channel).
 */
void phase(const Image *inImg, Image *outImg)
{
    int imageN = inImg->width;

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 1);
        outImg->is_chals = 1;
    }

    float *fft = inImg->chals->ch[1];
    float *angle = outImg->chals->ch[0];

    for (int i = 0; i < imageN * imageN; ++i)
    {
        angle[i] = atan2f(fft[i * 2 + 1], fft[i * 2]);
    }
}

/**
 * @brief Shifts the zero-frequency component to the center of the spectrum.
 *
 * Rearranges the FFT result stored in an interleaved (Re, Im) format
 * to move the low-frequency components to the center of the image.
 *
 * @param[in,out] data Interleaved complex buffer of size [width × height × 2]
 *                     representing FFT output (2 floats per pixel).
 * @param width        Width of the image (e.g., 256).
 * @param height       Height of the image (e.g., 256).
 */
void fftShift(float *data, int width, int height)
{
    int cx = width / 2;
    int cy = height / 2;

    for (int y = 0; y < cy; y++)
    {
        for (int x = 0; x < cx; x++)
        {
            // Calculate index for top-left (q0), top-right (q1), bottom-left (q2), bottom-right (q3)
            int q0 = 2 * ((y * width) + x);
            int q1 = 2 * ((y * width) + x + cx);
            int q2 = 2 * (((y + cy) * width) + x);
            int q3 = 2 * (((y + cy) * width) + x + cx);

            // Swap q0 and q3
            float tmp_re = data[q0];
            float tmp_im = data[q0 + 1];
            data[q0] = data[q3];
            data[q0 + 1] = data[q3 + 1];
            data[q3] = tmp_re;
            data[q3 + 1] = tmp_im;

            // Swap q1 and q2
            tmp_re = data[q1];
            tmp_im = data[q1 + 1];
            data[q1] = data[q2];
            data[q1 + 1] = data[q2 + 1];
            data[q2] = tmp_re;
            data[q2 + 1] = tmp_im;
        }
    }
}

/**
 * @brief Computes the inverse Fourier transform of the input image.
 *
 * @param inImg Input image (Fourier domain).
 * @param outImg Output image (spatial domain).
 */
void fourierInv(const Image *inImg, Image *outImg)
{

    int imageN = 256;
    float *fourier = inImg->chals->ch[0];
    float *fourier2 = inImg->chals->ch[1];

    for (int i = 0; i < imageN; i++)
    {
        arm_cfft_f32(&arm_cfft_sR_f32_len256, fourier2 + imageN * i * 2, 1, 1);
    }

    for (int k = 0; k < imageN; k++)
    {
        for (int j = 0; j < imageN; j++)
        {
            fourier[2 * j + k * imageN * 2] = (float)fourier2[j * imageN * 2 + k * 2];
            fourier[2 * j + 1 + k * imageN * 2] = (float)fourier2[j * imageN * 2 + k * 2 + 1];
        }
    }

    for (int i = 0; i < imageN; i++)
    {
        arm_cfft_f32(&arm_cfft_sR_f32_len256, fourier + imageN * i * 2, 1, 1);
    }

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 1);
        outImg->is_chals = 1;
    }
    for (int i = 0; i < imageN * imageN; i++)
    {
        outImg->chals->ch[0][i] = (float)sqrt(
            fourier[2 * i] * fourier[2 * i] + fourier[2 * i + 1] * fourier[2 * i + 1]);
    }
}

/**
 * @brief Converts polar coordinates (magnitude and phase) to complex cartesian (real and imaginary).
 *
 * @param magnitude Pointer to magnitude image (1 channel).
 * @param phase     Pointer to phase image (1 channel), in radians.
 * @param outImg    Output image with 2 channels: real and imaginary.
 */
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
        fft[i * 2] = mag[i] * cosf(phs[i]);
        fft[i * 2 + 1] = mag[i] * sinf(phs[i]);
    }
}

/**
 * @brief Performs element-wise complex multiplication in frequency domain.
 *
 * @param img1    First complex image (2 channels).
 * @param img2    Second complex image (2 channels).
 * @param outImg  Output complex image (2 channels).
 */
void multiply(const Image *img1, const Image *img2, Image *outImg)
{
    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 2);
        outImg->is_chals = 1;
    }

    float *img1_data = img1->chals->ch[0];

    float *img2_data = img2->chals->ch[0];

    float *outImg_data = outImg->chals->ch[0];

    for (int i = 0; i < img1->size; ++i)
    {
        // (a + ib)(c + id) = (ac - bd) + i(ad + bc)
        outImg_data[i] = img1_data[i] * img2_data[i];
    }
}

/**
 * @brief Creates a frequency domain filter mask (ideal or Gaussian).
 *
 * @param maskImg    Output single-channel mask image.
 * @param filterType Type of filter to create.
 * @param cutoff1    Cutoff radius (for low/high-pass), inner radius for band-pass.
 * @param cutoff2    Outer radius (for band-pass). Ignored for low/high-pass.
 */
void getMask(Image *maskImg, FrequencyFilterType filterType, float cutoff1, float cutoff2)
{
    int w = maskImg->width;
    int h = maskImg->height;
    int cx = w / 2;
    int cy = h / 2;

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
            float d = sqrtf(dx * dx + dy * dy);

            float value = 0.0f;

            switch (filterType)
            {
            case FREQ_FILTER_IDEAL_LOWPASS:
                value = (d <= cutoff1) ? 1.0f : 0.0f;
                break;

            case FREQ_FILTER_GAUSSIAN_LOWPASS:
                value = expf(-(d * d) / (2 * cutoff1 * cutoff1));
                break;

            case FREQ_FILTER_IDEAL_HIGHPASS:
                value = (d >= cutoff1) ? 1.0f : 0.0f;
                break;

            case FREQ_FILTER_GAUSSIAN_HIGHPASS:
                value = 1.0f - expf(-(d * d) / (2 * cutoff1 * cutoff1));
                break;

            case FREQ_FILTER_IDEAL_BANDPASS:
                value = (d >= cutoff1 && d <= cutoff2) ? 1.0f : 0.0f;
                break;

            case FREQ_FILTER_GAUSSIAN_BANDPASS:
            {
                float gLow = expf(-(d * d) / (2 * cutoff2 * cutoff2));
                float gHigh = expf(-(d * d) / (2 * cutoff1 * cutoff1));
                value = gLow - gHigh; // Gaussian band-pass = LP - HP
                break;
            }
            }

            mask[y * w + x] = value;
        }
    }
}
#include <fft.h>

/**
 * @brief Applies natural logarithm to each pixel value in a single-channel image.
 *
 * @param[in,out] img  Image to be transformed (in-place).
 */
void logImage(Image *img)
{
    if (!img || isChalsEmpty(img))
        return;

    float *data = img->chals->ch[0];
    for (int i = 0; i < img->size; ++i)
    {
        data[i] = logf(data[i]); // Add epsilon to avoid log(0)
    }
}

/**
 * @brief Adds a scalar value to all pixels in a single-channel image.
 *
 * @param[in,out] img   Image to modify (in-place).
 * @param[in]     value Scalar value to add.
 */
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
        fft[i * 2] = mag[i] * cosf(phs[i]);     // REEL
        fft[i * 2 + 1] = mag[i] * sinf(phs[i]); // IMJ
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

/**
 * @brief Checks if input dimensions are valid (powers of 2 and matching).
 */
static bool isValidFFTSize(int w, int h)
{
    return (w == h) && ((w & (w - 1)) == 0); // square and power-of-2
}

/**
 * @brief Performs forward 2D FFT on a single-channel image.
 *        ch[0] holds interleaved (Re, Im), ch[1] holds transposed for vertical pass.
 */
int fft(const Image *inImg, Image *outImg)
{
    int N = inImg->width;
    if (!isValidFFTSize(inImg->width, inImg->height))
        return -1;

    float *buf0;
    float *buf1;

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 2);
        outImg->is_chals = 1;
        buf0 = outImg->chals->ch[0];
        buf1 = outImg->chals->ch[1];
    }
    else
    {
        buf0 = outImg->chals->ch[0];
        buf1 = outImg->chals->ch[1];
    }

    const uint8_t *pixels = inImg->pixels;
    for (int i = 0; i < N * N; i++)
    {
        buf0[2 * i] = (float)pixels[i];
        buf0[2 * i + 1] = 0.0f;
    }

    for (int row = 0; row < N; row++)
    {
        arm_cfft_f32(&arm_cfft_sR_f32_len256, buf0 + row * N * 2, 0, 1);
    }

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

    for (int row = 0; row < N; row++)
    {
        arm_cfft_f32(&arm_cfft_sR_f32_len256, buf1 + row * N * 2, 0, 1);
    }

    // testing
    outImg->log = IMAGE_DATA_COMPLEX;
    // memory_free(buf0);
    return 0;
}

/**
 * @brief Performs inverse 2D FFT on complex image. ch[0] is output.
 */
int ifft(const Image *inImg, Image *outImg)
{
    int N = inImg->width;

    // if input image does not hold the correct data.
    if (inImg->log != IMAGE_DATA_COMPLEX && inImg->log != IMAGE_DATA_CH0)
    {
        return -1;
    }

    float *buf0;
    float *buf1;

    if (inImg->log == IMAGE_DATA_COMPLEX)
    {
        // current fft to ifft application.
        buf0 = (float *)memory_alloc(N * N * 2 * sizeof(float));
        buf1 = inImg->chals->ch[1];
    }
    else // if IMAGE_DATA_CH0
    {
        // In this case only 0 is allocated i guess.
        buf0 = (float *)memory_alloc(N * N * 2 * sizeof(float));
        buf1 = inImg->chals->ch[0];
    }

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 1);
        outImg->is_chals = 1;
    }

    for (int row = 0; row < N; row++)
    {
        arm_cfft_f32(&arm_cfft_sR_f32_len256, buf1 + row * N * 2, 1, 1);
    }

    for (int y = 0; y < N; y++)
    {
        for (int x = 0; x < N; x++)
        {
            int dst = 2 * (y * N + x);
            int src = 2 * (x * N + y);
            buf0[dst] = buf1[src];
            buf0[dst + 1] = buf1[src + 1];
        }
    }

    for (int row = 0; row < N; row++)
    {
        arm_cfft_f32(&arm_cfft_sR_f32_len256, buf0 + row * N * 2, 1, 1);
    }

    for (int i = 0; i < N * N; i++)
    {
        outImg->chals->ch[0][i] = buf0[2 * i];
    }

    outImg->log = IMAGE_DATA_CH0;
    memory_free(buf0);
    return 0;
}

/**
 * @brief Performs inverse 2D FFT on a frequency-domain image.
 *        Uses ch[1] as input (transposed buffer), writes to ch[0] as interleaved (Re, Im).
 */
int ifft__(const Image *inImg, Image *outImg)
{
    int N = inImg->width;
    if (!isValidFFTSize(inImg->width, inImg->height))
        return -1;

    float *buf0;

    if (isChalsEmpty(outImg))
    {
        createChals(outImg, 2);
        outImg->is_chals = 1;
        buf0 = outImg->chals->ch[0];
    }
    else
    {
        // memory_free(outImg->chals->ch[0]);
        // buf0 = (float *)memory_alloc(N * N * 2 * sizeof(float));
        outImg->chals->ch[0] = buf0;
    }

    float *input = inImg->chals->ch[1];

    // Inverse FFT on rows (from transposed data)
    for (int row = 0; row < N; row++)
    {
        arm_cfft_f32(&arm_cfft_sR_f32_len256, input + row * N * 2, 1, 1); // Inverse FFT
    }

    // Transpose back
    for (int y = 0; y < N; y++)
    {
        for (int x = 0; x < N; x++)
        {
            int src = 2 * (y * N + x);
            int dst = 2 * (x * N + y);
            buf0[dst] = input[src];
            buf0[dst + 1] = input[src + 1];
        }
    }

    // Inverse FFT on columns (rows of transposed image)
    for (int row = 0; row < N; row++)
    {
        arm_cfft_f32(&arm_cfft_sR_f32_len256, buf0 + row * N * 2, 1, 1); // Inverse FFT
    }

    // Normalize the output (divide all by N*N)
    float scale = 1.0f / (N * N);
    for (int i = 0; i < N * N * 2; ++i)
    {
        buf0[i] *= scale;
    }

    return 0;
}

/**
 * @brief Computes log-magnitude spectrum.
 */
void _abs(const Image *fftImg, Image *magImg)
{
    int size = fftImg->width * fftImg->height;

    float *fft;
    if (fftImg->log == IMAGE_DATA_COMPLEX)
    {
        // current fft to ifft application.
        fft = fftImg->chals->ch[1];
    }
    else if (fftImg->log == IMAGE_DATA_CH0)
    {
        // In this case only 0 is allocated i guess.
        fft = fftImg->chals->ch[0];
    }
    else
    {
        return -1;
    }

    if (isChalsEmpty(magImg))
    {
        createChals(magImg, 1);
        magImg->is_chals = 1;
    }

    float *mag = magImg->chals->ch[0];
    for (int i = 0; i < size; i++)
    {
        float re = fft[2 * i];
        float im = fft[2 * i + 1];
        mag[i] = sqrtf(re * re + im * im);
    }

    magImg->log = IMAGE_DATA_MAGNITUDE;
}

/**
 * @brief Computes phase angle from FFT image.
 */
void _phase(const Image *fftImg, Image *phaseImg)
{
    int size = fftImg->width * fftImg->height;

    float *fft;
    if (fftImg->log == IMAGE_DATA_COMPLEX)
    {
        // current fft to ifft application.
        fft = fftImg->chals->ch[1];
    }
    else if (fftImg->log == IMAGE_DATA_CH0)
    {
        // In this case only 0 is allocated i guess.
        fft = fftImg->chals->ch[0];
    }
    else
    {
        return -1;
    }

    if (isChalsEmpty(phaseImg))
    {
        createChals(phaseImg, 1);
        phaseImg->is_chals = 1;
    }

    float *out = phaseImg->chals->ch[0];
    for (int i = 0; i < size; i++)
    {
        out[i] = atan2f(fft[2 * i + 1], fft[2 * i]);
    }

    phaseImg->log = IMAGE_DATA_PHASE;
}

/**
 * @brief Rearranges FFT result so that low-frequency component is centered.
 */
void fftshift(float *data, int width, int height)
{
    int cx = width / 2, cy = height / 2;
    for (int y = 0; y < cy; ++y)
    {
        for (int x = 0; x < cx; ++x)
        {
            int q0 = 2 * ((y * width) + x);
            int q1 = 2 * ((y * width) + x + cx);
            int q2 = 2 * (((y + cy) * width) + x);
            int q3 = 2 * (((y + cy) * width) + x + cx);

            for (int k = 0; k < 2; ++k)
            {
                float tmp = data[q0 + k];
                data[q0 + k] = data[q3 + k];
                data[q3 + k] = tmp;

                tmp = data[q1 + k];
                data[q1 + k] = data[q2 + k];
                data[q2 + k] = tmp;
            }
        }
    }
}

/**
 * @brief Applies a frequency-domain filter to a complex image.
 *
 * This function performs element-wise complex multiplication between a Fourier-domain image
 * and a filter mask. The mask can be either a grayscale magnitude mask or a complex-valued mask.
 *
 * @param[in]  fftImg     Complex frequency-domain image (Re, Im interleaved in ch[0]).
 * @param[in]  filterMask Grayscale or complex mask to apply in frequency domain.
 * @param[out] outImg     Output image after filtering in the frequency domain.
 */
void ffilter2D(const Image *fftImg, const Image *filterMask, Image *outImg)
{

    if (!fftImg || !filterMask || !outImg ||
        isChalsEmpty(fftImg) || isChalsEmpty(filterMask))
        return;

    int width = fftImg->width;
    int height = fftImg->height;
    int size = width * height;

    // Step 1: Compute magnitude and phase
    Image *magImg = createImageWH(fftImg->width, fftImg->height, fftImg->format);
    Image *phaseImg = createImageWH(fftImg->width, fftImg->height, fftImg->format);

    _abs(fftImg, magImg);
    _phase(fftImg, phaseImg);

    // Step 2: Multiply magnitude by filter mask (element-wise)
    float *mag = magImg->chals->ch[0];
    float *mask = filterMask->chals->ch[0];
    for (int i = 0; i < size; ++i)
        mag[i] *= mask[i];

    // Step 3: Reconstruct complex data from filtered mag + original phase
    polarToCart(magImg, phaseImg, outImg);
}

/*

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
        // memory_free(outImg->chals->ch[1]);
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

    float test = 0;
    for (int i = 0; i < imageN * imageN; ++i)
    {
        float re = fft[i * 2];
        float im = fft[i * 2 + 1];
        magnitude[i] = sqrtf(re * re + im * im);
        if (magnitude[i] > test)
            test = magnitude[i];
    }

    test = test + 1;
}

void phase(const Image *inImg, Image *outImg)
{
    int imageN = 256;

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

*/

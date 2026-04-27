// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include <arch/fft_backend.h>
#include <board/common.h>
#include <core/memory_manager.h>
#include <embedDIP_configs.h>
#include <imgproc/fft.h>

static bool isValidFFTSize(int w, int h)
{
    return (w == h) && ((w & (w - 1)) == 0);
}

static void transposeComplex(const float *src, float *dst, int n)
{
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            int src_idx = 2 * (y * n + x);
            int dst_idx = 2 * (x * n + y);
            dst[dst_idx] = src[src_idx];
            dst[dst_idx + 1] = src[src_idx + 1];
        }
    }
}

embeddip_status_t fft(const Image *src, Image *dst)
{
    if (!src || !dst || !src->pixels)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (!isValidFFTSize(src->width, src->height))
        return EMBEDDIP_ERROR_INVALID_SIZE;

    int n = src->width;
    embeddip_status_t status = embeddip_fft_backend_init(n);
    if (status != EMBEDDIP_OK)
        return status;

    if (isChalsEmpty(dst)) {
        status = createChalsComplex(dst, 2);
        if (status != EMBEDDIP_OK)
            return status;
        dst->is_chals = 1;
    }

    if (!dst->chals || !dst->chals->ch[0] || !dst->chals->ch[1])
        return EMBEDDIP_ERROR_NULL_PTR;

    float *buf0 = dst->chals->ch[0];
    float *buf1 = dst->chals->ch[1];

    const uint8_t *pixels = (const uint8_t *)src->pixels;
    for (int i = 0; i < n * n; ++i) {
        buf0[2 * i] = (float)pixels[i];
        buf0[2 * i + 1] = 0.0f;
    }

    for (int row = 0; row < n; ++row) {
        status = embeddip_fft_backend_forward_1d(buf0 + row * n * 2, n);
        if (status != EMBEDDIP_OK)
            return status;
    }

    transposeComplex(buf0, buf1, n);

    for (int row = 0; row < n; ++row) {
        status = embeddip_fft_backend_forward_1d(buf1 + row * n * 2, n);
        if (status != EMBEDDIP_OK)
            return status;
    }

    transposeComplex(buf1, buf0, n);

    for (int i = 0; i < n * n * 2; ++i) {
        buf1[i] = buf0[i];
    }

    dst->log = IMAGE_DATA_COMPLEX;
    return EMBEDDIP_OK;
}

embeddip_status_t ifft(const Image *src, Image *dst)
{
    if (!src || !dst || !src->chals)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (!isValidFFTSize(src->width, src->height))
        return EMBEDDIP_ERROR_INVALID_SIZE;

    if (src->log != IMAGE_DATA_COMPLEX && src->log != IMAGE_DATA_CH0)
        return EMBEDDIP_ERROR_INVALID_ARG;

    int n = src->width;
    embeddip_status_t status = embeddip_fft_backend_init(n);
    if (status != EMBEDDIP_OK)
        return status;

    float *input = (src->log == IMAGE_DATA_COMPLEX) ? src->chals->ch[1] : src->chals->ch[0];
    if (!input)
        return EMBEDDIP_ERROR_NULL_PTR;

    float *tmp = (float *)memory_alloc((size_t)n * (size_t)n * 2U * sizeof(float));
    if (!tmp)
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    if (isChalsEmpty(dst)) {
        status = createChals(dst, 1);
        if (status != EMBEDDIP_OK) {
            memory_free(tmp);
            return status;
        }
        dst->is_chals = 1;
    }

    if (!dst->chals || !dst->chals->ch[0]) {
        memory_free(tmp);
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    for (int row = 0; row < n; ++row) {
        status = embeddip_fft_backend_inverse_1d(input + row * n * 2, n);
        if (status != EMBEDDIP_OK) {
            memory_free(tmp);
            return status;
        }
    }

    transposeComplex(input, tmp, n);

    for (int row = 0; row < n; ++row) {
        status = embeddip_fft_backend_inverse_1d(tmp + row * n * 2, n);
        if (status != EMBEDDIP_OK) {
            memory_free(tmp);
            return status;
        }
    }

    transposeComplex(tmp, input, n);

    for (int i = 0; i < n * n; ++i) {
        dst->chals->ch[0][i] = input[2 * i];
    }

    dst->log = IMAGE_DATA_CH0;
    memory_free(tmp);
    return EMBEDDIP_OK;
}

static embeddip_status_t getComplexInput(const Image *src, float **out)
{
    if (!src || !out || !src->chals) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    if (src->log == IMAGE_DATA_COMPLEX) {
        *out = src->chals->ch[1];
    } else if (src->log == IMAGE_DATA_CH0) {
        *out = src->chals->ch[0];
    } else {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    return (*out != NULL) ? EMBEDDIP_OK : EMBEDDIP_ERROR_NULL_PTR;
}

embeddip_status_t _log_(Image *img)
{
    if (!img)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (isChalsEmpty(img))
        return EMBEDDIP_ERROR_INVALID_ARG;

    float *data = img->chals->ch[0];
    if (!data)
        return EMBEDDIP_ERROR_NULL_PTR;

    for (int i = 0; i < img->size; ++i) {
        float v = data[i];
#if defined(EMBED_DIP_ARCH_XTENSA)
        v += 1e-3f;  // Preserve previous Xtensa behavior and avoid log(0).
#endif
        data[i] = logf(v);
    }

    return EMBEDDIP_OK;
}

embeddip_status_t _add_(Image *img, float value)
{
    if (!img)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (isChalsEmpty(img))
        return EMBEDDIP_ERROR_INVALID_ARG;

    float *data = img->chals->ch[0];
    if (!data)
        return EMBEDDIP_ERROR_NULL_PTR;

    for (int i = 0; i < img->size; ++i) {
        data[i] += value;
    }

    return EMBEDDIP_OK;
}

embeddip_status_t _abs_(const Image *src, Image *dst)
{
    if (!src || !dst)
        return EMBEDDIP_ERROR_NULL_PTR;

    int size = src->width * src->height;

    float *fft = NULL;
    embeddip_status_t status = getComplexInput(src, &fft);
    if (status != EMBEDDIP_OK)
        return status;

    if (isChalsEmpty(dst)) {
        status = createChals(dst, 1);
        if (status != EMBEDDIP_OK)
            return status;
        dst->is_chals = 1;
    }

    float *mag = dst->chals->ch[0];
    if (!mag)
        return EMBEDDIP_ERROR_NULL_PTR;

    for (int i = 0; i < size; ++i) {
        float re = fft[2 * i];
        float im = fft[2 * i + 1];
        mag[i] = sqrtf(re * re + im * im);
    }

    dst->log = IMAGE_DATA_MAGNITUDE;
    return EMBEDDIP_OK;
}

embeddip_status_t _phase_(const Image *src, Image *dst)
{
    if (!src || !dst)
        return EMBEDDIP_ERROR_NULL_PTR;

    int size = src->width * src->height;

    float *fft = NULL;
    embeddip_status_t status = getComplexInput(src, &fft);
    if (status != EMBEDDIP_OK)
        return status;

    if (isChalsEmpty(dst)) {
        status = createChals(dst, 1);
        if (status != EMBEDDIP_OK)
            return status;
        dst->is_chals = 1;
    }

    float *out = dst->chals->ch[0];
    if (!out)
        return EMBEDDIP_ERROR_NULL_PTR;

    for (int i = 0; i < size; ++i) {
        out[i] = atan2f(fft[2 * i + 1], fft[2 * i]);
    }

    dst->log = IMAGE_DATA_PHASE;
    return EMBEDDIP_OK;
}

embeddip_status_t fftshift(Image *img)
{
    if (!img || isChalsEmpty(img))
        return EMBEDDIP_ERROR_NULL_PTR;

    float *data = (img->log == IMAGE_DATA_COMPLEX) ? img->chals->ch[1] : img->chals->ch[0];
    if (!data)
        return EMBEDDIP_ERROR_NULL_PTR;

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

embeddip_status_t polarToCart(const Image *mag_img, const Image *phase_img, Image *dst)
{
    if (!mag_img || !phase_img || !dst)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (mag_img->width != phase_img->width || mag_img->height != phase_img->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;

    if (!mag_img->chals || !phase_img->chals || !mag_img->chals->ch[0] || !phase_img->chals->ch[0])
        return EMBEDDIP_ERROR_NULL_PTR;

    int size = mag_img->width * mag_img->height;

    if (!isChalsEmpty(dst) && dst->chals && dst->chals->ch[0]) {
        memory_free(dst->chals->ch[0]);
        dst->chals->ch[0] = NULL;
    }

    embeddip_status_t status = createChalsComplex(dst, 1);
    if (status != EMBEDDIP_OK)
        return status;

    float *mag = mag_img->chals->ch[0];
    float *phs = phase_img->chals->ch[0];
    float *fft = dst->chals->ch[0];

    for (int i = 0; i < size; ++i) {
        fft[2 * i] = mag[i] * cosf(phs[i]);
        fft[2 * i + 1] = mag[i] * sinf(phs[i]);
    }

    dst->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

embeddip_status_t multiply(const Image *img1, const Image *img2, Image *outImg)
{
    if (!img1 || !img2 || !outImg)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (img1->width != img2->width || img1->height != img2->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;

    if (isChalsEmpty(outImg)) {
        embeddip_status_t status = createChals(outImg, 1);
        if (status != EMBEDDIP_OK)
            return status;
        outImg->is_chals = 1;
    }

    float *in1 = NULL;
    float *in2 = NULL;
    const uint8_t *pix1 = NULL;
    const uint8_t *pix2 = NULL;

    if (img1->log == IMAGE_DATA_CH0) {
        in1 = img1->chals ? img1->chals->ch[0] : NULL;
    } else if (img1->log == IMAGE_DATA_COMPLEX) {
        in1 = img1->chals ? img1->chals->ch[1] : NULL;
    } else if (img1->log == IMAGE_DATA_PIXELS) {
        pix1 = (const uint8_t *)img1->pixels;
    } else {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    if (img2->log == IMAGE_DATA_CH0) {
        in2 = img2->chals ? img2->chals->ch[0] : NULL;
    } else if (img2->log == IMAGE_DATA_COMPLEX) {
        in2 = img2->chals ? img2->chals->ch[1] : NULL;
    } else if (img2->log == IMAGE_DATA_PIXELS) {
        pix2 = (const uint8_t *)img2->pixels;
    } else {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    if ((!in1 && !pix1) || (!in2 && !pix2) || !outImg->chals || !outImg->chals->ch[0])
        return EMBEDDIP_ERROR_NULL_PTR;

    float *out = outImg->chals->ch[0];
    int size = img1->width * img1->height;
    for (int i = 0; i < size; ++i) {
        float v1 = in1 ? in1[i] : (float)pix1[i];
        float v2 = in2 ? in2[i] : (float)pix2[i];
        out[i] = v1 * v2;
    }

    outImg->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

embeddip_status_t difference(const Image *src1, const Image *src2, Image *dst)
{
    if (!src1 || !src2 || !dst)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (src1->width != src2->width || src1->height != src2->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;

    int size = src1->width * src1->height;

    if (isChalsEmpty(dst)) {
        embeddip_status_t status = createChals(dst, 1);
        if (status != EMBEDDIP_OK)
            return status;
        dst->is_chals = 1;
    }

    float *out = dst->chals->ch[0];
    if (!out)
        return EMBEDDIP_ERROR_OUT_OF_MEMORY;

    if (src1->log == IMAGE_DATA_PIXELS &&
        (src2->log == IMAGE_DATA_CH0 || src2->log == IMAGE_DATA_MAGNITUDE)) {
        if (!src1->pixels || !src2->chals || !src2->chals->ch[0])
            return EMBEDDIP_ERROR_NULL_PTR;

        uint8_t *pix1 = src1->pixels;
        float *ch2 = src2->chals->ch[0];
        for (int i = 0; i < size; ++i)
            out[i] = fmaxf((float)pix1[i] - ch2[i], 0.0f);
    } else if (src1->log == IMAGE_DATA_PIXELS && src2->log == IMAGE_DATA_PIXELS) {
        if (!src1->pixels || !src2->pixels)
            return EMBEDDIP_ERROR_NULL_PTR;

        uint8_t *pix1 = src1->pixels;
        uint8_t *pix2 = src2->pixels;
        for (int i = 0; i < size; ++i)
            out[i] = fmaxf((float)(pix1[i] - pix2[i]), 0.0f);
    } else if ((src1->log == IMAGE_DATA_CH0 || src1->log == IMAGE_DATA_MAGNITUDE) &&
               (src2->log == IMAGE_DATA_CH0 || src2->log == IMAGE_DATA_MAGNITUDE)) {
        if (!src1->chals || !src1->chals->ch[0] || !src2->chals || !src2->chals->ch[0])
            return EMBEDDIP_ERROR_NULL_PTR;

        float *ch1 = src1->chals->ch[0];
        float *ch2 = src2->chals->ch[0];
        for (int i = 0; i < size; ++i)
            out[i] = fmaxf(ch1[i] - ch2[i], 0.0f);
    } else if ((src1->log == IMAGE_DATA_CH0 || src1->log == IMAGE_DATA_MAGNITUDE) &&
               src2->log == IMAGE_DATA_PIXELS) {
        if (!src1->chals || !src1->chals->ch[0] || !src2->pixels)
            return EMBEDDIP_ERROR_NULL_PTR;

        float *ch1 = src1->chals->ch[0];
        uint8_t *pix2 = src2->pixels;
        for (int i = 0; i < size; ++i)
            out[i] = fmaxf(ch1[i] - (float)pix2[i], 0.0f);
    } else {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    dst->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

embeddip_status_t
getFilter(Image *filter_img, FrequencyFilterType filter_type, float cutoff1, float cutoff2)
{
    if (!filter_img)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (cutoff1 < 0.0f)
        return EMBEDDIP_ERROR_INVALID_ARG;

    if (filter_type == FREQ_FILTER_IDEAL_BANDPASS || filter_type == FREQ_FILTER_GAUSSIAN_BANDPASS) {
        if (cutoff2 < 0.0f || cutoff1 >= cutoff2)
            return EMBEDDIP_ERROR_INVALID_ARG;
    }

    int w = filter_img->width;
    int h = filter_img->height;
    int cx = w / 2;
    int cy = h / 2;

    filter_img->format = IMAGE_FORMAT_GRAYSCALE;

    if (isChalsEmpty(filter_img)) {
        embeddip_status_t status = createChals(filter_img, 1);
        if (status != EMBEDDIP_OK)
            return status;
        filter_img->is_chals = 1;
    }

    float *mask = filter_img->chals->ch[0];
    if (!mask)
        return EMBEDDIP_ERROR_NULL_PTR;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int dx = x - cx;
            int dy = y - cy;
            float d = sqrtf((float)(dx * dx + dy * dy));

            float value = 0.0f;
            switch (filter_type) {
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

    filter_img->log = IMAGE_DATA_CH0;
    return EMBEDDIP_OK;
}

embeddip_status_t ffilter2D(const Image *src_fft, const Image *filter, Image *dst)
{
    if (!src_fft || !filter || !dst)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (isChalsEmpty(src_fft) || isChalsEmpty(filter))
        return EMBEDDIP_ERROR_INVALID_ARG;

    int size = src_fft->width * src_fft->height;

    Image *mag_img = NULL;
    Image *phase_img = NULL;

    embeddip_status_t status =
        createImageWH(src_fft->width, src_fft->height, src_fft->format, &mag_img);
    if (status != EMBEDDIP_OK)
        return status;

    status = createImageWH(src_fft->width, src_fft->height, src_fft->format, &phase_img);
    if (status != EMBEDDIP_OK) {
        deleteImage(mag_img);
        return status;
    }

    status = _abs_(src_fft, mag_img);
    if (status != EMBEDDIP_OK) {
        deleteImage(mag_img);
        deleteImage(phase_img);
        return status;
    }

    status = _phase_(src_fft, phase_img);
    if (status != EMBEDDIP_OK) {
        deleteImage(mag_img);
        deleteImage(phase_img);
        return status;
    }

    if (!mag_img->chals || !mag_img->chals->ch[0] || !filter->chals || !filter->chals->ch[0]) {
        deleteImage(mag_img);
        deleteImage(phase_img);
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    float *mag = mag_img->chals->ch[0];
    float *mask = filter->chals->ch[0];

    for (int i = 0; i < size; ++i)
        mag[i] *= mask[i];

    status = polarToCart(mag_img, phase_img, dst);

    deleteImage(mag_img);
    deleteImage(phase_img);
    return status;
}

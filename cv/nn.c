// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/nn.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "cv/image_gray.h"

embeddip_status_t cv_nn_image_to_tensor(const ImageView *src, cv_tensor_t *dst)
{
    embeddip_status_t status;
    size_t i;
    uint32_t y;

    if (dst == NULL || dst->data == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    status = cv_gray_view_validate(src);
    if (status != EMBEDDIP_OK) {
        return status;
    }
    if (dst->channels != 1u) {
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }
    if ((uint32_t)dst->width != src->width || (uint32_t)dst->height != src->height) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    if ((dst->type == CV_TENSOR_I8 || dst->type == CV_TENSOR_U8) && !(dst->scale > 0.0f)) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    i = 0u;
    for (y = 0u; y < src->height; ++y) {
        const uint8_t *row = &src->pixels[(size_t)y * (size_t)src->row_stride_bytes];
        uint32_t x;
        for (x = 0u; x < src->width; ++x, ++i) {
            float normalized = (float)row[x] / 255.0f;

            switch (dst->type) {
            case CV_TENSOR_F32: {
                if ((i + 1u) * sizeof(float) > dst->bytes) {
                    return EMBEDDIP_ERROR_INVALID_SIZE;
                }
                ((float *)dst->data)[i] = normalized;
                break;
            }
            case CV_TENSOR_I8: {
                float q = roundf(normalized / dst->scale) + (float)dst->zero_point;
                if (i + 1u > dst->bytes) {
                    return EMBEDDIP_ERROR_INVALID_SIZE;
                }
                if (q > 127.0f) {
                    q = 127.0f;
                } else if (q < -128.0f) {
                    q = -128.0f;
                }
                ((int8_t *)dst->data)[i] = (int8_t)q;
                break;
            }
            case CV_TENSOR_U8: {
                float q = roundf(normalized / dst->scale) + (float)dst->zero_point;
                if (i + 1u > dst->bytes) {
                    return EMBEDDIP_ERROR_INVALID_SIZE;
                }
                if (q > 255.0f) {
                    q = 255.0f;
                } else if (q < 0.0f) {
                    q = 0.0f;
                }
                ((uint8_t *)dst->data)[i] = (uint8_t)q;
                break;
            }
            default:
                return EMBEDDIP_ERROR_NOT_SUPPORTED;
            }
        }
    }

    return EMBEDDIP_OK;
}

embeddip_status_t
cv_nn_argmax(const float *scores, size_t count, size_t *out_index, float *out_value)
{
    size_t best = 0u;
    size_t i;

    if (scores == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (count == 0u) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    for (i = 1u; i < count; ++i) {
        if (scores[i] > scores[best]) {
            best = i;
        }
    }
    if (out_index != NULL) {
        *out_index = best;
    }
    if (out_value != NULL) {
        *out_value = scores[best];
    }
    return EMBEDDIP_OK;
}

embeddip_status_t cv_nn_softmax(float *logits, size_t count)
{
    float max_logit;
    double sum = 0.0;
    size_t i;

    if (logits == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (count == 0u) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    max_logit = logits[0];
    for (i = 1u; i < count; ++i) {
        if (logits[i] > max_logit) {
            max_logit = logits[i];
        }
    }
    for (i = 0u; i < count; ++i) {
        float e = expf(logits[i] - max_logit);
        logits[i] = e;
        sum += (double)e;
    }
    if (sum <= 0.0) {
        return EMBEDDIP_ERROR_UNDERFLOW;
    }
    for (i = 0u; i < count; ++i) {
        logits[i] = (float)((double)logits[i] / sum);
    }
    return EMBEDDIP_OK;
}

embeddip_status_t
cv_nn_segmentation_argmax(const cv_tensor_t *output, uint8_t *class_map, size_t capacity)
{
    size_t pixels;
    size_t channels;
    size_t p;
    const float *data;

    if (output == NULL || output->data == NULL || class_map == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (output->type != CV_TENSOR_F32) {
        return EMBEDDIP_ERROR_NOT_SUPPORTED;
    }
    if (output->width == 0u || output->height == 0u || output->channels == 0u) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    if (output->channels > 256u) {
        return EMBEDDIP_ERROR_NOT_SUPPORTED; /* class index must fit in uint8_t */
    }
    pixels = (size_t)output->width * (size_t)output->height;
    channels = (size_t)output->channels;
    if (capacity < pixels) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    data = (const float *)output->data;
    for (p = 0u; p < pixels; ++p) {
        size_t best = 0u;
        size_t c;
        float best_val;

        /* HWC: channel stride 1, pixel stride channels.
         * CHW: channel stride pixels, pixel stride 1. */
        if (output->layout == CV_TENSOR_HWC) {
            best_val = data[p * channels];
            for (c = 1u; c < channels; ++c) {
                float v = data[p * channels + c];
                if (v > best_val) {
                    best_val = v;
                    best = c;
                }
            }
        } else {
            best_val = data[p];
            for (c = 1u; c < channels; ++c) {
                float v = data[c * pixels + p];
                if (v > best_val) {
                    best_val = v;
                    best = c;
                }
            }
        }
        class_map[p] = (uint8_t)best;
    }
    return EMBEDDIP_OK;
}

embeddip_status_t cv_nn_colorize(const uint8_t *class_map,
                                 uint32_t width,
                                 uint32_t height,
                                 const uint8_t *palette,
                                 size_t palette_count,
                                 uint8_t *rgb,
                                 size_t rgb_capacity)
{
    size_t pixels;
    size_t p;

    if (class_map == NULL || palette == NULL || rgb == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (width == 0u || height == 0u || palette_count == 0u) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }
    pixels = (size_t)width * (size_t)height;
    if (rgb_capacity < pixels * 3u) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    for (p = 0u; p < pixels; ++p) {
        uint8_t cls = class_map[p];
        if ((size_t)cls >= palette_count) {
            return EMBEDDIP_ERROR_OUT_OF_RANGE;
        }
        rgb[p * 3u + 0u] = palette[(size_t)cls * 3u + 0u];
        rgb[p * 3u + 1u] = palette[(size_t)cls * 3u + 1u];
        rgb[p * 3u + 2u] = palette[(size_t)cls * 3u + 2u];
    }
    return EMBEDDIP_OK;
}

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_NN_H
#define EMBEDDIP_CV_NN_H

#include <stddef.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"
#include "runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fill a model input tensor from an 8-bit grayscale image view.
 *
 * Each pixel is normalized to [0, 1] (pixel / 255) and then encoded to the
 * tensor's element type:
 *  - CV_TENSOR_F32: stored as the normalized float.
 *  - CV_TENSOR_I8 / CV_TENSOR_U8: quantized as
 *    round(normalized / scale) + zero_point, clamped to the type range.
 *
 * The tensor must be single-channel and match the image dimensions. Only the
 * normalized-then-quantized single-channel contract is supported.
 *
 * @param[in] src Valid 8-bit grayscale or mask image view.
 * @param[in,out] dst Tensor with allocated @p data and matching dimensions.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_nn_image_to_tensor(const ImageView *src, cv_tensor_t *dst);

/**
 * @brief Index and value of the maximum element of a float score vector.
 *
 * Ties resolve to the lowest index.
 *
 * @param[in] scores Score/logit array.
 * @param[in] count Number of scores (> 0).
 * @param[out] out_index Argmax index (may be NULL).
 * @param[out] out_value Maximum value (may be NULL).
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_nn_argmax(const float *scores, size_t count, size_t *out_index,
                               float *out_value);

/**
 * @brief Numerically stable softmax over a float logit vector, in place.
 *
 * @param[in,out] logits Logits on input, probabilities on output.
 * @param[in] count Number of elements (> 0).
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_nn_softmax(float *logits, size_t count);

/**
 * @brief Per-pixel argmax over a float segmentation output tensor.
 *
 * Writes one class index per pixel (row-major, width*height entries). The
 * tensor must be CV_TENSOR_F32 with channels == number of classes; both
 * CV_TENSOR_HWC and CV_TENSOR_CHW layouts are supported.
 *
 * @param[in] output Segmentation logits/probabilities tensor.
 * @param[out] class_map Caller-owned class-index buffer.
 * @param[in] capacity Capacity of @p class_map in bytes/entries.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_nn_segmentation_argmax(const cv_tensor_t *output,
                                            uint8_t *class_map, size_t capacity);

/**
 * @brief Colorize a class-index map into a packed RGB888 buffer.
 *
 * @param[in] class_map Class indices, width*height entries.
 * @param[in] width Map width.
 * @param[in] height Map height.
 * @param[in] palette Row-major RGB palette; class c uses palette[c*3..c*3+2].
 * @param[in] palette_count Number of palette entries.
 * @param[out] rgb Caller-owned RGB888 output (width*height*3 bytes).
 * @param[in] rgb_capacity Capacity of @p rgb in bytes.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_nn_colorize(const uint8_t *class_map, uint32_t width,
                                 uint32_t height, const uint8_t *palette,
                                 size_t palette_count, uint8_t *rgb,
                                 size_t rgb_capacity);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_NN_H */

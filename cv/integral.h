// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_INTEGRAL_H
#define EMBEDDIP_CV_INTEGRAL_H

#include <stdint.h>

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Non-owning view of an unsigned 32-bit integral image.
 */
typedef struct {
    uint32_t *values;           /**< Integral values in row-major order. */
    uint32_t width;             /**< Logical width in values. */
    uint32_t height;            /**< Logical height in values. */
    uint32_t row_stride_values; /**< Distance between rows, in uint32_t values. */
} CvIntegralU32;

/**
 * @brief Build an unsigned 32-bit integral image from an 8-bit gray view.
 *
 * @param[in] src Valid 8-bit grayscale or mask image view.
 * @param[in,out] dst Caller-owned integral table matching the source dimensions.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_integral_u8_u32(const ImageView *src, CvIntegralU32 *dst);

/**
 * @brief Sum a rectangular region using an unsigned 32-bit integral image.
 *
 * @param[in] table Integral table to query.
 * @param[in] roi Positive, in-bounds rectangle.
 * @param[out] out_sum Sum of the values in the rectangle.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_integral_sum_u32(const CvIntegralU32 *table, Rectangle roi,
                                      uint64_t *out_sum);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_INTEGRAL_H */

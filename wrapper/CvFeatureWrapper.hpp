// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#pragma once

#include <cstddef>

extern "C" {
#include "core/error.h"
#include "core/image.h"

#include "cv/haar.h"
#include "cv/hog.h"
#include "cv/image_gray.h"
#include "cv/integral.h"
#include "cv/linear_classifier.h"
}

#include "wrapper/ImageWrapper.hpp"

namespace embedDIP
{

/**
 * @brief Non-owning, non-throwing facade over the classical-feature C API.
 *
 * Every method returns the underlying embeddip_status_t and operates on
 * caller-owned buffers and models; no hidden allocation occurs.
 */
class CvFeatures
{
  public:
    /** @see cv_gray_view_validate */
    static embeddip_status_t validateGray(const Image &image, ImageView &view) noexcept
    {
        embeddip_status_t status = image_view_from_image(image.raw(), &view);
        if (status != EMBEDDIP_OK) {
            return status;
        }
        return cv_gray_view_validate(&view);
    }

    /** @see cv_integral_u8_u32 */
    static embeddip_status_t integral(const ImageView &src, CvIntegralU32 &dst) noexcept
    {
        return cv_integral_u8_u32(&src, &dst);
    }

    /** @see cv_haar_feature_response */
    static embeddip_status_t haarFeature(const CvIntegralU32 &table,
                                         int32_t originX,
                                         int32_t originY,
                                         const CvHaarRect *rectangles,
                                         uint8_t rectangleCount,
                                         int32_t &responseQ8) noexcept
    {
        return cv_haar_feature_response(
            &table, originX, originY, rectangles, rectangleCount, &responseQ8);
    }

    /** @see cv_haar_cascade_eval */
    static embeddip_status_t haarCascade(const CvIntegralU32 &table,
                                         int32_t originX,
                                         int32_t originY,
                                         const CvHaarCascade &cascade,
                                         bool &detected) noexcept
    {
        return cv_haar_cascade_eval(&table, originX, originY, &cascade, &detected);
    }

    /** @see cv_hog_descriptor_size */
    static embeddip_status_t
    hogSize(Rectangle roi, const CvHogConfig &config, size_t &length) noexcept
    {
        return cv_hog_descriptor_size(roi, &config, &length);
    }

    /** @see cv_hog_extract */
    static embeddip_status_t hog(const ImageView &src,
                                 Rectangle roi,
                                 const CvHogConfig &config,
                                 float *descriptor,
                                 size_t capacity,
                                 size_t &length) noexcept
    {
        return cv_hog_extract(&src, roi, &config, descriptor, capacity, &length);
    }

    /** @see cv_linear_classifier_topk */
    static embeddip_status_t linearTopK(const CvLinearClassifier &model,
                                        const float *descriptor,
                                        size_t descriptorLength,
                                        size_t topK,
                                        CvClassScore *scores,
                                        size_t scoreCapacity,
                                        size_t &count) noexcept
    {
        return cv_linear_classifier_topk(
            &model, descriptor, descriptorLength, topK, scores, scoreCapacity, &count);
    }
};

}  // namespace embedDIP

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/haar.h"

#include <stdint.h>

embeddip_status_t cv_haar_feature_response(const CvIntegralU32 *table, int32_t origin_x,
                                           int32_t origin_y,
                                           const CvHaarRect *rectangles,
                                           uint8_t rectangle_count,
                                           int32_t *out_response_q8)
{
    int64_t accumulator = 0;
    uint8_t i;

    if (out_response_q8 == NULL || rectangles == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (rectangle_count == 0u || rectangle_count > CV_HAAR_MAX_RECTS) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }

    for (i = 0u; i < rectangle_count; ++i) {
        const CvHaarRect *rect = &rectangles[i];
        Rectangle roi = {
            .x = origin_x + (int32_t)rect->x,
            .y = origin_y + (int32_t)rect->y,
            .width = (int32_t)rect->width,
            .height = (int32_t)rect->height,
        };
        uint64_t region_sum = 0u;
        embeddip_status_t status;

        /* cv_integral_sum_u32 validates the table and rejects out-of-bounds ROIs. */
        status = cv_integral_sum_u32(table, roi, &region_sum);
        if (status != EMBEDDIP_OK) {
            return status;
        }
        /* region_sum <= UINT32_MAX and |weight_q8| <= 32768 keep this in int64. */
        accumulator += (int64_t)region_sum * (int64_t)rect->weight_q8;
    }

    if (accumulator > (int64_t)INT32_MAX || accumulator < (int64_t)INT32_MIN) {
        return EMBEDDIP_ERROR_OVERFLOW;
    }
    *out_response_q8 = (int32_t)accumulator;
    return EMBEDDIP_OK;
}

embeddip_status_t cv_haar_cascade_score(const CvIntegralU32 *table, int32_t origin_x,
                                        int32_t origin_y, const CvHaarCascade *cascade,
                                        bool *out_detected, int32_t *out_score)
{
    size_t s;
    int64_t score = 0;
    bool detected = true;

    if (cascade == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (cascade->stage_count > 0u && cascade->stages == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }

    for (s = 0u; s < cascade->stage_count; ++s) {
        const CvHaarStage *stage = &cascade->stages[s];
        int64_t stage_sum = 0;
        size_t w;

        if (stage->weak_count > 0u && stage->weak == NULL) {
            return EMBEDDIP_ERROR_NULL_PTR;
        }
        for (w = 0u; w < stage->weak_count; ++w) {
            const CvHaarWeakClassifier *weak = &stage->weak[w];
            int32_t response = 0;
            embeddip_status_t status;

            status = cv_haar_feature_response(table, origin_x, origin_y,
                                              weak->rectangles, weak->rectangle_count,
                                              &response);
            if (status != EMBEDDIP_OK) {
                return status;
            }
            stage_sum += (response >= weak->threshold_q8) ? (int64_t)weak->right_value
                                                          : (int64_t)weak->left_value;
        }
        score += stage_sum - (int64_t)stage->threshold;
        if (stage_sum < (int64_t)stage->threshold) {
            detected = false;
            break;
        }
    }

    if (score > (int64_t)INT32_MAX) {
        score = (int64_t)INT32_MAX;
    } else if (score < (int64_t)INT32_MIN) {
        score = (int64_t)INT32_MIN;
    }
    if (out_detected != NULL) {
        *out_detected = detected;
    }
    if (out_score != NULL) {
        *out_score = (int32_t)score;
    }
    return EMBEDDIP_OK;
}

embeddip_status_t cv_haar_cascade_eval(const CvIntegralU32 *table, int32_t origin_x,
                                       int32_t origin_y, const CvHaarCascade *cascade,
                                       bool *out_detected)
{
    if (out_detected == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    return cv_haar_cascade_score(table, origin_x, origin_y, cascade, out_detected, NULL);
}

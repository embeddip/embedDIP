// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_LINEAR_CLASSIFIER_H
#define EMBEDDIP_CV_LINEAR_CLASSIFIER_H

#include <stddef.h>
#include <stdint.h>

#include "core/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Non-owning linear (one-vs-rest) classifier model.
 *
 * Weights are row-major: class c uses weights[c * descriptor_length ..].
 */
typedef struct {
    const float *weights;      /**< class_count * descriptor_length values. */
    const float *bias;         /**< class_count bias terms (may be NULL). */
    uint16_t class_count;      /**< Number of classes (> 0). */
    uint16_t descriptor_length;/**< Feature length per class (> 0). */
} CvLinearClassifier;

/**
 * @brief Scored class result.
 */
typedef struct {
    uint16_t class_index; /**< Class index. */
    float score;          /**< Linear score for the class. */
} CvClassScore;

/**
 * @brief Score a descriptor and return the top-k classes by descending score.
 *
 * Ties break toward the lower class index. The model is never mutated.
 *
 * @param[in] model Classifier model.
 * @param[in] descriptor Feature vector.
 * @param[in] descriptor_length Length of @p descriptor; must match the model.
 * @param[in] top_k Number of results requested (> 0).
 * @param[out] scores Caller-owned output array.
 * @param[in] score_capacity Capacity of @p scores.
 * @param[out] out_count Number of results written (min of top_k, class_count).
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_linear_classifier_topk(const CvLinearClassifier *model,
                                            const float *descriptor,
                                            size_t descriptor_length, size_t top_k,
                                            CvClassScore *scores, size_t score_capacity,
                                            size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_LINEAR_CLASSIFIER_H */

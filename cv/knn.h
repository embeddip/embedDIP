// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_KNN_H
#define EMBEDDIP_CV_KNN_H

#include <stddef.h>
#include <stdint.h>

#include "core/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Distance metric for k-NN over float feature vectors. */
typedef enum {
    CV_KNN_L2 = 0, /**< Squared Euclidean distance. */
    CV_KNN_L1 = 1  /**< Manhattan distance. */
} CvKnnMetric;

/**
 * @brief k-nearest-neighbour classification over float feature vectors.
 *
 * Brute-force: computes the distance from @p query to every training vector,
 * keeps the @p k nearest, and returns the majority label (ties broken by
 * smaller summed distance, then smaller label). Pairs naturally with feature
 * extractors such as HOG (`cv/hog.h`) for whole-image recognition.
 *
 * @param[in] train Row-major training matrix, @p n_train x @p dim.
 * @param[in] labels One label per training row.
 * @param[in] n_train Number of training vectors (> 0).
 * @param[in] dim Feature dimension (> 0).
 * @param[in] query Query vector of length @p dim.
 * @param[in] k Neighbours to poll (1 <= k <= n_train).
 * @param[in] metric Distance metric.
 * @param[out] out_label Predicted label.
 * @param[out] out_votes Optional: how many of the k neighbours had that label
 *             (may be NULL).
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_knn_classify_f32(const float *train, const int32_t *labels,
                                      size_t n_train, size_t dim, const float *query, int k,
                                      CvKnnMetric metric, int32_t *out_label, int *out_votes);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_KNN_H */

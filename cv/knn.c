// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/knn.h"

#include <float.h>

#define CV_KNN_MAX_K 64

embeddip_status_t cv_knn_classify_f32(const float *train, const int32_t *labels,
                                      size_t n_train, size_t dim, const float *query, int k,
                                      CvKnnMetric metric, int32_t *out_label, int *out_votes)
{
    if (!train || !labels || !query || !out_label)
        return EMBEDDIP_ERROR_NULL_PTR;
    if (n_train == 0 || dim == 0)
        return EMBEDDIP_ERROR_INVALID_SIZE;
    if (k < 1 || (size_t)k > n_train || k > CV_KNN_MAX_K)
        return EMBEDDIP_ERROR_INVALID_ARG;

    /* k nearest, kept sorted ascending by distance (insertion). */
    float bd[CV_KNN_MAX_K];
    int32_t bl[CV_KNN_MAX_K];
    int have = 0;
    for (int i = 0; i < k; ++i)
        bd[i] = FLT_MAX;

    for (size_t t = 0; t < n_train; ++t) {
        const float *row = train + t * dim;
        float d = 0.0f;
        for (size_t j = 0; j < dim; ++j) {
            float diff = row[j] - query[j];
            d += (metric == CV_KNN_L1) ? (diff < 0 ? -diff : diff) : diff * diff;
        }
        if (d >= bd[k - 1] && have >= k)
            continue;
        /* insert (d, label) into the sorted top-k */
        int pos = (have < k) ? have : k - 1;
        while (pos > 0 && bd[pos - 1] > d) {
            bd[pos] = bd[pos - 1];
            bl[pos] = bl[pos - 1];
            --pos;
        }
        bd[pos] = d;
        bl[pos] = labels[t];
        if (have < k)
            ++have;
    }

    /* Majority vote among the k neighbours; tie -> smaller summed distance,
     * then smaller label. */
    int32_t best_label = bl[0];
    int best_votes = 0;
    float best_sum = FLT_MAX;
    for (int i = 0; i < k; ++i) {
        int32_t lab = bl[i];
        int seen = 0;
        for (int j = 0; j < i; ++j)
            if (bl[j] == lab) {
                seen = 1;
                break;
            }
        if (seen)
            continue;
        int votes = 0;
        float sum = 0.0f;
        for (int j = 0; j < k; ++j)
            if (bl[j] == lab) {
                ++votes;
                sum += bd[j];
            }
        int better = (votes > best_votes) ||
                     (votes == best_votes && sum < best_sum) ||
                     (votes == best_votes && sum == best_sum && lab < best_label);
        if (better) {
            best_votes = votes;
            best_sum = sum;
            best_label = lab;
        }
    }

    *out_label = best_label;
    if (out_votes)
        *out_votes = best_votes;
    return EMBEDDIP_OK;
}

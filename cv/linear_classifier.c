// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "cv/linear_classifier.h"

#include <stddef.h>

embeddip_status_t cv_linear_classifier_topk(const CvLinearClassifier *model,
                                            const float *descriptor,
                                            size_t descriptor_length, size_t top_k,
                                            CvClassScore *scores, size_t score_capacity,
                                            size_t *out_count)
{
    size_t result_count;
    uint16_t c;
    size_t filled = 0u;

    if (model == NULL || descriptor == NULL || scores == NULL || out_count == NULL ||
        model->weights == NULL) {
        return EMBEDDIP_ERROR_NULL_PTR;
    }
    if (model->class_count == 0u || model->descriptor_length == 0u || top_k == 0u) {
        return EMBEDDIP_ERROR_INVALID_ARG;
    }
    if (descriptor_length != (size_t)model->descriptor_length) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    result_count = ((size_t)model->class_count < top_k) ? (size_t)model->class_count
                                                        : top_k;
    if (score_capacity < result_count) {
        return EMBEDDIP_ERROR_INVALID_SIZE;
    }

    /* Insertion into a bounded descending list; O(class_count * result_count). */
    for (c = 0u; c < model->class_count; ++c) {
        const float *row = &model->weights[(size_t)c * descriptor_length];
        float score = (model->bias != NULL) ? model->bias[c] : 0.0f;
        size_t i;
        size_t pos;

        for (i = 0u; i < descriptor_length; ++i) {
            score += row[i] * descriptor[i];
        }

        /* Find insertion point: strictly-greater score wins the higher slot,
         * so equal scores keep the earlier (lower) class index ahead. */
        pos = filled;
        while (pos > 0u && scores[pos - 1u].score < score) {
            --pos;
        }
        if (pos < result_count) {
            size_t last = (filled < result_count) ? filled : result_count - 1u;
            size_t j;
            for (j = last; j > pos; --j) {
                scores[j] = scores[j - 1u];
            }
            scores[pos].class_index = c;
            scores[pos].score = score;
            if (filled < result_count) {
                ++filled;
            }
        }
    }

    *out_count = result_count;
    return EMBEDDIP_OK;
}

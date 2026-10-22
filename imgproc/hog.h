// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_HOG_H
#define EMBEDDIP_HOG_H

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HOG-lite descriptor computation.
 *
 * Splits the image into a @p cells x @p cells grid and accumulates
 * unsigned-gradient magnitude into @p bins orientation bins per cell from
 * simple central-difference gradients (no Gaussian smoothing or block
 * normalization, unlike full HOG).
 *
 * @param[in]  src         Source grayscale image.
 * @param[in]  cells       Grid cells per side.
 * @param[in]  bins        Orientation bins per cell.
 * @param[out] descriptor  Caller-allocated output, length cells*cells*bins.
 */
embeddip_status_t computeHogDescriptor(const Image *src, int cells, int bins, float *descriptor);

/**
 * @brief Brute-force k-NN classifier over squared Euclidean distance.
 *
 * @param[in]  query        Descriptor to classify, length @p desc_len.
 * @param[in]  train_set    Training descriptors, row-major [num_samples][desc_len].
 * @param[in]  labels       Class label (0..num_classes-1) for each training sample.
 * @param[in]  num_samples  Number of training samples.
 * @param[in]  desc_len     Descriptor length.
 * @param[in]  k            Number of nearest neighbors to vote.
 * @param[in]  num_classes  Number of distinct class labels.
 * @param[out] out_label    Majority-vote label.
 */
embeddip_status_t knnClassify(const float *query,
                              const float *train_set,
                              const int *labels,
                              int num_samples,
                              int desc_len,
                              int k,
                              int num_classes,
                              int *out_label);

#ifdef __cplusplus
}
#endif

#endif  // EMBEDDIP_HOG_H

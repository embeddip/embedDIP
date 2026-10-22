// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_FEATURES_H
#define EMBEDDIP_FEATURES_H

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Bit width of the descriptor produced by computeBriefDescriptors(). */
#define EMBEDDIP_BRIEF_DESC_BITS 32

/**
 * @brief Integer pixel coordinate of a detected keypoint.
 */
typedef struct {
    int16_t x; /**< Column */
    int16_t y; /**< Row */
} Keypoint;

/**
 * @brief FAST-lite corner detector.
 *
 * 8-point ring at radius 3; a pixel is a corner if at least 6 of the 8 ring
 * samples are all brighter or all darker than the center by more than
 * @p threshold. (Full FAST uses a 16-point ring and a 12-point contiguous
 * arc requirement; this is the simplified variant.)
 *
 * @param[in]  src            Source grayscale image.
 * @param[in]  threshold      Brightness difference threshold.
 * @param[out] keypoints      Caller-allocated output array.
 * @param[in]  max_keypoints  Capacity of @p keypoints.
 * @param[out] num_keypoints  Number of keypoints written.
 */
embeddip_status_t detectFastKeypoints(const Image *src,
                                      uint8_t threshold,
                                      Keypoint *keypoints,
                                      int max_keypoints,
                                      int *num_keypoints);

/**
 * @brief BRIEF-lite descriptor computation.
 *
 * Builds a 32-bit descriptor per keypoint from deterministic pixel-pair
 * comparisons in a +/-4 patch around the keypoint. (Full BRIEF is typically
 * 256 bits and draws pair offsets from a stored random pattern table.)
 *
 * @param[in]  src        Source grayscale image.
 * @param[in]  keypoints  Keypoints to describe.
 * @param[in]  num_keypoints  Number of entries in @p keypoints.
 * @param[out] descriptors    Caller-allocated output array, one per keypoint.
 */
embeddip_status_t computeBriefDescriptors(const Image *src,
                                          const Keypoint *keypoints,
                                          int num_keypoints,
                                          uint32_t *descriptors);

/**
 * @brief Hamming distance between two 32-bit descriptors.
 */
int hammingDistance32(uint32_t a, uint32_t b);

/**
 * @brief Nearest-neighbor descriptor matching by Hamming distance.
 *
 * For each descriptor in @p descA, finds its nearest neighbor in @p descB
 * and counts it as a match if the Hamming distance is <= @p max_hamming.
 *
 * @param[in]  descA        First descriptor set.
 * @param[in]  numA         Number of entries in @p descA.
 * @param[in]  descB        Second descriptor set.
 * @param[in]  numB         Number of entries in @p descB.
 * @param[in]  max_hamming  Maximum Hamming distance accepted as a match.
 * @param[out] num_matches  Number of matches found.
 */
embeddip_status_t matchDescriptors(const uint32_t *descA,
                                   int numA,
                                   const uint32_t *descB,
                                   int numB,
                                   int max_hamming,
                                   int *num_matches);

#ifdef __cplusplus
}
#endif

#endif  // EMBEDDIP_FEATURES_H

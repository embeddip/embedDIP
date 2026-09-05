// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_KEYPOINT_H
#define EMBEDDIP_CV_KEYPOINT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief BRIEF descriptor length in bytes (256 bits). */
#define CV_BRIEF_BYTES 32

/**
 * @brief A detected keypoint.
 */
typedef struct {
    int32_t x;   /**< Column. */
    int32_t y;   /**< Row. */
    float score; /**< FAST corner strength (sum of |arc - center|). */
    float angle; /**< Orientation in radians (0 until cv_keypoint_orient runs). */
} CvKeypoint;

/**
 * @brief One descriptor match (query index -> train index) with distance.
 */
typedef struct {
    uint32_t query_idx; /**< Index into the query descriptor set. */
    uint32_t train_idx; /**< Index of the best train descriptor. */
    uint32_t distance;  /**< Hamming distance (0..256). */
} CvMatch;

/**
 * @brief FAST-9 corner detector on an 8-bit grayscale image.
 *
 * A pixel is a corner if at least 9 contiguous pixels on the radius-3 Bresenham
 * circle are all brighter than center+threshold or all darker than
 * center-threshold. With @p nonmax set, only strict local maxima of the corner
 * score (within a 3x3 window) are kept. Highest-scoring corners survive when
 * more are found than @p out_capacity.
 *
 * @param[in] src 8-bit grayscale image view.
 * @param[in] threshold Intensity margin (typical 10..40).
 * @param[in] nonmax Apply 3x3 non-maximum suppression.
 * @param[out] out Caller-owned keypoint buffer, filled by descending score.
 * @param[in] out_capacity Capacity of @p out.
 * @param[out] out_count Number of keypoints written (<= out_capacity).
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_fast_detect(const ImageView *src, uint8_t threshold, bool nonmax,
                                 CvKeypoint *out, size_t out_capacity, size_t *out_count);

/**
 * @brief Assign each keypoint an orientation (ORB intensity-centroid method).
 *
 * Sets kp.angle = atan2(m01, m10) over a radius-15 circular patch, where
 * m10 = sum x*I, m01 = sum y*I. Keypoints closer than 15 px to a border keep
 * angle 0. Feeding oriented keypoints to ::cv_brief_describe steers the
 * sampling pattern, giving rotation-invariant (ORB-style) descriptors.
 *
 * @param[in] src 8-bit grayscale image view.
 * @param[in,out] kps Keypoints whose .angle is filled.
 * @param[in] count Number of keypoints.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_keypoint_orient(const ImageView *src, CvKeypoint *kps, size_t count);

/**
 * @brief Compute BRIEF-256 descriptors for keypoints.
 *
 * If a keypoint's .angle is non-zero the sampling pattern is rotated by that
 * angle (steered BRIEF / rBRIEF), making the descriptor rotation-invariant;
 * with .angle == 0 this is plain BRIEF.
 *
 * Writes @p count descriptors of ::CV_BRIEF_BYTES each into @p desc (row-major,
 * descriptor i at desc + i*CV_BRIEF_BYTES). Keypoints closer than the patch
 * half-width (15 px) to a border get an all-zero descriptor. The sampling
 * pattern is fixed, so descriptors are comparable across calls and images.
 *
 * @param[in] src 8-bit grayscale image view.
 * @param[in] kps Keypoints to describe.
 * @param[in] count Number of keypoints.
 * @param[out] desc Output buffer of at least count*CV_BRIEF_BYTES bytes.
 * @param[in] desc_capacity_bytes Capacity of @p desc in bytes.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_brief_describe(const ImageView *src, const CvKeypoint *kps, size_t count,
                                    uint8_t *desc, size_t desc_capacity_bytes);

/**
 * @brief Multi-scale ORB configuration (image pyramid).
 */
typedef struct {
    int nlevels;             /**< Pyramid levels (>= 1). Each level halves size. */
    uint8_t fast_threshold;  /**< FAST threshold applied at every level. */
} CvOrbConfig;

/**
 * @brief Scale-invariant ORB: pyramid FAST + orientation + steered BRIEF.
 *
 * Builds a halving image pyramid (level L is the image downsampled by 2^L),
 * runs FAST + intensity-centroid orientation + rBRIEF at each level, and merges
 * the keypoints. Coordinates and .score are reported in level-0 pixels; .angle
 * is the level's orientation; each descriptor is computed at its native level so
 * its BRIEF patch covers a scale-appropriate area — giving scale invariance
 * that single-scale FAST/BRIEF lacks. Highest-scoring keypoints are kept when
 * more are found than @p out_capacity (descriptors stay aligned to @p out).
 *
 * @param[in] src 8-bit grayscale image view.
 * @param[in] cfg Pyramid configuration.
 * @param[out] out Keypoint buffer (level-0 coords), descending score.
 * @param[out] desc Descriptor buffer, >= out_capacity * CV_BRIEF_BYTES bytes,
 *             row i aligned to out[i].
 * @param[in] out_capacity Capacity of @p out (and descriptor rows).
 * @param[out] out_count Number of keypoints written.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_orb_detect_and_describe(const ImageView *src, const CvOrbConfig *cfg,
                                             CvKeypoint *out, uint8_t *desc,
                                             size_t out_capacity, size_t *out_count);

/**
 * @brief Brute-force nearest-neighbour Hamming match of BRIEF descriptors.
 *
 * For each of @p query_count query descriptors, finds the train descriptor with
 * the smallest Hamming distance and writes one ::CvMatch (kept only if the
 * distance is at most @p max_distance).
 *
 * @param[in] query Query descriptors (query_count * CV_BRIEF_BYTES).
 * @param[in] query_count Number of query descriptors.
 * @param[in] train Train descriptors (train_count * CV_BRIEF_BYTES).
 * @param[in] train_count Number of train descriptors.
 * @param[in] max_distance Reject matches above this Hamming distance (0..256).
 * @param[out] out Caller-owned match buffer.
 * @param[in] out_capacity Capacity of @p out.
 * @param[out] out_count Number of matches written.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_hamming_match(const uint8_t *query, size_t query_count,
                                   const uint8_t *train, size_t train_count,
                                   uint32_t max_distance, CvMatch *out, size_t out_capacity,
                                   size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_KEYPOINT_H */

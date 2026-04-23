// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_SEGMENTATION_H
#define EMBEDDIP_SEGMENTATION_H

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Segments a grayscale image using K-means clustering.
 *
 * @param[in]  inImg   Pointer to input grayscale image.
 * @param[out] outImg  Pointer to output segmented image.
 * @param[in]  k       Number of clusters (e.g., 2 for foreground/background).
 */
embeddip_status_t grayscaleKMeans(const Image *src, Image *dst, int k);

/**
 * @brief Segments an HSI image using K-means clustering.
 *
 * @param[in]  inImg   Pointer to input HSI image.
 * @param[out] outImg  Pointer to output segmented HSI image.
 * @param[in]  k       Number of clusters.
 */
embeddip_status_t colorKMeans_old(const Image *inImg, Image *outImg, int k);

/**
 * @brief Segments an image using K-means in its native color space.
 *
 * @param[in]  inImg   Input image
 * @param[out] outImg  Output segmented image (same format as input)
 * @param[in]  k       Number of clusters (>=1, <= number of pixels)
 */
embeddip_status_t colorKMeans(const Image *inImg, Image *outImg, int k);

/**
 * @brief Segments a grayscale image using region growing with multiple seed points.
 *
 * @param[in]  inImg      Pointer to input grayscale image.
 * @param[out] outImg     Pointer to output binary image (0 for background, 255 for region).
 * @param[in]  seeds      Array of seed points.
 * @param[in]  numSeeds   Number of seed points.
 * @param[in]  tolerance  Intensity difference threshold for region inclusion.
 */
embeddip_status_t grayscaleRegionGrowing(const Image *inImg,
                                         Image *outImg,
                                         const Point *seeds,
                                         int numSeeds,
                                         uint8_t tolerance);

/**
 * @brief Single-seed color region growing.
 *
 * @param[in]  inImg      Pointer to input color image.
 * @param[out] outImg     Pointer to output binary image.
 * @param[in]  seedX      Seed point X coordinate.
 * @param[in]  seedY      Seed point Y coordinate.
 * @param[in]  tolerance  Color distance tolerance.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t
colorRegionGrowing_single(const Image *inImg, Image *outImg, int seedX, int seedY, float tolerance);

/**
 * @brief Multi-seed color region growing.
 *
 * @param[in]  inImg      Pointer to input color image.
 * @param[out] outImg     Pointer to output binary image.
 * @param[in]  seeds      Array of seed points.
 * @param[in]  numSeeds   Number of seeds.
 * @param[in]  tolerance  Color distance tolerance.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t colorRegionGrowing(const Image *inImg,
                                     Image *outImg,
                                     const Point *seeds,
                                     int numSeeds,
                                     float tolerance);

/**
 * @brief GrabCut segmentation (grayscale realistic).
 *
 * @param[in]  src        Pointer to input grayscale image.
 * @param[out] mask       Pointer to output mask image.
 * @param[in]  roi        Region of interest.
 * @param[in]  iterations Number of iterations.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t grabCut(const Image *src, Image *mask, Rectangle roi, int iterations);

/**
 * @brief GrabCut segmentation (lightweight version).
 *
 * @param[in]  src        Pointer to input image.
 * @param[out] mask       Pointer to output mask image.
 * @param[in]  roi        Region of interest.
 * @param[in]  iterations Number of iterations.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t grabCutLite(const Image *src, Image *mask, Rectangle roi, int iterations);

#ifdef __cplusplus
}
#endif

#endif  // EMBEDDIP_SEGMENTATION_H

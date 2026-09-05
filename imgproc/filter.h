// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef FILTER_H
#define FILTER_H

#include "board/common.h"
#include "core/image.h"
#include "core/memory_manager.h"

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Context for generic 2D convolution.
 *
 * Stores kernel metadata and a flattened square kernel buffer.
 */
typedef struct {
    int size;       ///< Kernel dimension (size x size)
    float *kernel;  ///< Flat kernel array in row-major order
    int chal;       ///< Reserved channel field (internal use)
} Filter2DContext;

/**
 * @brief Applies 2D filter to a single channel (internal helper function).
 * @param src Pointer to the input image
 * @param dst Pointer to the output image
 * @param ch_idx Channel index to process
 * @param ctx Filter context (kernel parameters)
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t filter2D_single_channel(Image *src, Image *dst, int ch_idx, void *ctx);

/**
 * @brief Applies 2D convolution filter to an image.
 * @param src Pointer to the input image
 * @param dst Pointer to the output image
 * @param kernel Pointer to the convolution kernel (flattened 2D array)
 * @param kernel_size Size of the square kernel (e.g., 3 for 3x3)
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t filter2D(const Image *src, Image *dst, const float *kernel, int kernel_size);

/**
 * @brief Applies separable 2D filter (row kernel × column kernel).
 * @param src Pointer to the input image
 * @param dst Pointer to the output image
 * @param size_x Size of the horizontal kernel
 * @param kernel_x Pointer to the horizontal kernel
 * @param size_y Size of the vertical kernel
 * @param kernel_y Pointer to the vertical kernel
 * @param delta Value added to each filtered pixel
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t sepfilter2D(Image *src,
                              Image *dst,
                              int size_x,
                              float *kernel_x,
                              int size_y,
                              float *kernel_y,
                              float delta);

/**
 * @brief Applies minimum filter (erosion) to an image.
 * @param src Pointer to the input image
 * @param dst Pointer to the output image
 * @param kernel_size Size of the filter kernel
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t minFilter(const Image *src, Image *dst, int kernel_size);

/**
 * @brief Applies maximum filter (dilation) to an image.
 * @param src Pointer to the input image
 * @param dst Pointer to the output image
 * @param kernel_size Size of the filter kernel
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t maxFilter(const Image *src, Image *dst, int kernel_size);

/**
 * @brief Applies median filter to an image for noise reduction.
 * @param src Pointer to the input image
 * @param dst Pointer to the output image
 * @param kernel_size Size of the filter kernel
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t medianFilter(const Image *src, Image *dst, int kernel_size);

/**
 * @brief Splits an RGB image into separate R, G, B channel images.
 * @param src Pointer to the input RGB image
 * @param r_img Pointer to the output red channel image
 * @param g_img Pointer to the output green channel image
 * @param b_img Pointer to the output blue channel image
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t rgbSplit(const Image *src, Image *r_img, Image *g_img, Image *b_img);

/**
 * @brief Merges separate R, G, B channel images into an RGB image.
 * @param r_img Pointer to the red channel image
 * @param g_img Pointer to the green channel image
 * @param b_img Pointer to the blue channel image
 * @param dst Pointer to the output RGB image
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t rgbMerge(const Image *r_img, const Image *g_img, const Image *b_img, Image *dst);

/**
 * @brief Applies Difference of Gaussians (DoG) filter for edge detection.
 * @param src Pointer to the input image
 * @param dst Pointer to the output image
 * @param sigma1 Standard deviation of the first Gaussian
 * @param sigma2 Standard deviation of the second Gaussian
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t dogFilter(const Image *src, Image *dst, float sigma1, float sigma2);

/**
 * @brief Applies Laplacian of Gaussian (LoG) filter for edge detection.
 * @param src Pointer to the input image
 * @param dst Pointer to the output image
 * @param sigma Standard deviation of the Gaussian
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t logFilter(const Image *src, Image *dst, float sigma);

/**
 * @brief Computes Gaussian-smoothed image gradients in X and Y directions.
 * @param src Pointer to the input image
 * @param dst_ix Pointer to the output X-gradient image
 * @param dst_iy Pointer to the output Y-gradient image
 * @param sigma Standard deviation of the Gaussian smoothing
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t gaussianGradients(const Image *src, Image *dst_ix, Image *dst_iy, float sigma);

/**
 * @brief Computes gradient magnitude from X and Y gradient images.
 * @param ix_img Pointer to the X-gradient image
 * @param iy_img Pointer to the Y-gradient image
 * @param dst Pointer to the output magnitude image
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t gradientMagnitude(const Image *ix_img, const Image *iy_img, Image *dst);

/**
 * @brief Computes gradient phase (angle) from X and Y gradient images.
 * @param ix_img Pointer to the X-gradient image
 * @param iy_img Pointer to the Y-gradient image
 * @param dst Pointer to the output phase image
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t gradientPhase(const Image *ix_img, const Image *iy_img, Image *dst);

/**
 * @brief Applies Canny edge detection algorithm.
 * @param src Pointer to the input image
 * @param dst Pointer to the output edge image
 * @param threshold1 Lower threshold for edge detection
 * @param threshold2 Upper threshold for edge detection
 * @param aperture_size Size of Sobel kernel (3, 5, or 7)
 * @param l2_gradient Use L2 norm for gradient magnitude if true
 * @return EMBEDDIP_OK on success, error code on failure
 */
embeddip_status_t Canny(const Image *src,
                        Image *dst,
                        double threshold1,
                        double threshold2,
                        int aperture_size,
                        bool l2_gradient);
#ifdef __cplusplus
}
#endif

#endif  // FILTER_H

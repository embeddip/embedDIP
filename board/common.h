// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef COMMON_H
#define COMMON_H

/**
 * @file common.h
 * @brief Common helper functions for the EmbedDIP library.
 *
 * This header provides utility functions for:
 * - Creating and destroying images
 * - Managing image channels
 * - Measuring execution time for performance profiling
 *
 * All functions are available from both C and C++.
 */

#include "core/error.h"
#include "core/image.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup embedDIP_common Common Utilities
 * @ingroup embedDIP_c_api
 * @brief Image creation, channel management, and timing helpers.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate and initialize an Image structure using predefined resolution.
 *
 * This function allocates memory for an Image structure and its pixel buffer.
 * The resolution is determined from lookup tables, and the format specifies
 * how the pixels are stored.
 *
 * @param[in]  resolution  Desired image resolution (lookup-based).
 * @param[in]  format      Desired image format (e.g., GRAYSCALE, RGB565, RGB888).
 * @param[out] out_image   Pointer to store the newly allocated Image.
 *
 * @return EMBEDDIP_OK on success, error code on failure.
 */
embeddip_status_t createImage(ImageResolution resolution, ImageFormat format, Image **out_image);

/**
 * @brief Allocate and initialize an Image structure with custom width/height.
 *
 * This function allows flexible allocation of images by specifying exact
 * dimensions instead of relying on lookup tables.
 *
 * @param[in]  width      Desired image width in pixels.
 * @param[in]  height     Desired image height in pixels.
 * @param[in]  format     Desired image format (e.g., GRAYSCALE, RGB565, RGB888).
 * @param[out] out_image  Pointer to store the newly allocated Image.
 *
 * @return EMBEDDIP_OK on success, error code on failure.
 */
embeddip_status_t createImageWH(int width, int height, ImageFormat format, Image **out_image);

/**
 * @brief Free all resources associated with an Image structure.
 *
 * This function safely frees the pixel buffer, optional channel buffers,
 * and the Image structure itself. After calling this, the pointer to
 * Image should not be used again.
 *
 * @param[in] image Pointer to the Image structure to delete.
 */
void deleteImage(Image *image);

/**
 * @brief Check whether an image has channel buffers allocated.
 *
 * @param[in] inImg Pointer to the Image structure.
 * @return true if no channels are allocated, false otherwise.
 */
bool isChalsEmpty(const Image *inImg);

/**
 * @brief Allocate channel buffers for an image.
 *
 * This function creates floating-point channel buffers for the image
 * if they are not already allocated. Each buffer is sized as
 * width × height × 2 floats, supporting complex data if needed.
 *
 * @param[in,out] inImg    Pointer to the Image structure.
 * @param[in]     numChals Number of channels to allocate (1..6).
 *
 * @return EMBEDDIP_OK on success, error code on failure.
 */
embeddip_status_t createChals(Image *inImg, uint8_t numChals);

/**
 * @brief Creates complex-valued channels for FFT operations.
 *
 * Allocates channels with 2× size to store interleaved complex data (real + imaginary).
 * Used by FFT operations: width × height × 2 × sizeof(float) per channel.
 *
 * @param[in,out] inImg    Target image to allocate complex channels for.
 * @param[in]     numChals Number of complex channels to allocate (1..6).
 *
 * @return EMBEDDIP_OK on success, error code on failure.
 */
embeddip_status_t createChalsComplex(Image *inImg, uint8_t numChals);

/* ========================================================================
 * Deprecated/Legacy Functions (for backward compatibility)
 * ======================================================================== */

/**
 * @brief Legacy wrapper for createImage (returns NULL on error).
 * @deprecated Use createImage() with status code instead.
 * @note This function is provided for backward compatibility with
 *       existing internal code. New code should use the status-code version.
 */
Image *createImage_legacy(ImageResolution resolution, ImageFormat format);

/**
 * @brief Legacy wrapper for createImageWH (returns NULL on error).
 * @deprecated Use createImageWH() with status code instead.
 * @note This function is provided for backward compatibility with
 *       existing internal code. New code should use the status-code version.
 */
Image *createImageWH_legacy(int width, int height, ImageFormat format);

/**
 * @brief Starts a timer for performance measurement.
 *
 * Records the current time; used with ::toc to measure elapsed time.
 */
void tic(void);

/**
 * @brief Stops the timer and returns elapsed time in milliseconds.
 *
 * @return Elapsed time in milliseconds since the last call to ::tic.
 */
uint32_t toc(void);

#ifdef __cplusplus
}
#endif

/** @} */ /* end of embedDIP_common */

#endif /* COMMON_H */

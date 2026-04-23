// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_IMGPROC_COMPRESS_H
#define EMBEDDIP_IMGPROC_COMPRESS_H

#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Compression format */
#define IMAGE_COMP_JPEG 0

/**
 * @brief Compress image to JPEG format.
 * @param src Source image (RGB565, RGB888, or grayscale).
 * @param dst Destination image (JPEG payload stored in pixels buffer).
 * @param format Compression format (use IMAGE_COMP_JPEG).
 * @param quality JPEG quality (1-100, higher = better quality).
 * @return 0 on success, -1 on error.
 */
int compress(Image *src, Image *dst, int format, int quality);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_IMGPROC_COMPRESS_H */

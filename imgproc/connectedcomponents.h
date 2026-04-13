// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CONNECTEDCOMPONENTS_H
#define EMBEDDIP_CONNECTEDCOMPONENTS_H

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Label connected components in a binary image.
 *
 * @param[in]  src  Pointer to input binary image.
 * @param[out] dst  Pointer to output labeled image.
 * @param[out] label_count Optional output pointer for number of connected
 *                         labels present in output (including background).
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t connectedComponents(const Image *src, Image *dst, uint32_t *label_count);

/**
 * @brief Extracts a specific connected component from a labeled image.
 *
 * @param labeledImg Input image (output of connectedComponents).
 * @param objImg Output binary image (extracted object).
 * @param targetLabel The label of the object to extract.
 */
embeddip_status_t extractComponent(const Image *src, Image *dst, int target_label);

#ifdef __cplusplus
}
#endif

#endif  // EMBEDDIP_CONNECTEDCOMPONENTS_H

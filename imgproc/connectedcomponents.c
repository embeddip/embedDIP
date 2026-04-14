// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#include "connectedcomponents.h"

#include <string.h> /* For memset */

embeddip_status_t connectedComponents(const Image *src, Image *dst, uint32_t *label_count)
{
    int label = 1;
    uint8_t *src_data = src->pixels;
    uint8_t *dst_data = dst->pixels;

    uint8_t equvData[256] = {0};
    memset(dst_data, 0x00, src->size);

    // First pass
    for (uint32_t y = 0; y < src->height; y++) {
        for (uint32_t x = 0; x < src->width; x++) {
            int index = y * src->width + x;
            if (src_data[index] != 0) {  // If pixel is part of an object
                int left = (x > 0) ? dst_data[index - 1] : 0;
                int up = (y > 0) ? dst_data[index - src->width] : 0;

                if (left && up) {                               // Both neighbors are labeled
                    dst_data[index] = (left < up) ? left : up;  // Assign the minimum label
                    if (left != up) {
                        // Store the equivalence between labels
                        int minLabel = (left < up) ? left : up;
                        int maxLabel = (left > up) ? left : up;
                        equvData[maxLabel] = minLabel;
                    }
                } else if (left || up) {                       // One neighbor is labeled
                    dst_data[index] = (left > 0) ? left : up;  // Assign the labeled label
                } else {
                    if (label >= 255) {
                        return EMBEDDIP_ERROR_OVERFLOW;
                    }
                    dst_data[index] = label++;  // Assign a new label
                }
            }
        }
    }

    // Second pass to resolve equivalences
    for (uint32_t i = 0; i < src->width * src->height; i++) {
        if (dst_data[i] > 0) {
            int root = dst_data[i];
            while (equvData[root] > 0) {
                root = equvData[root];
            }
            dst_data[i] = root;
        }
    }

    // Relabel to consecutive values
    int newLabel = 1;
    int labelMap[256] = {0};  // assuming max 255 labels, adjust if needed

    for (uint32_t i = 0; i < src->size; i++) {
        int lbl = dst_data[i];
        if (lbl > 0) {
            if (labelMap[lbl] == 0) {
                labelMap[lbl] = newLabel++;
            }
            dst_data[i] = labelMap[lbl];
        }
    }

    if (label_count) {
        *label_count = (uint32_t)newLabel;
    }

    dst->log = IMAGE_DATA_PIXELS;

    return EMBEDDIP_OK;
}

/**
 * @brief Extracts a specific connected component from a labeled image.
 *
 * @param labeledImg Input image (output of connectedComponents).
 * @param objImg Output binary image (extracted object).
 * @param targetLabel The label of the object to extract.
 */
embeddip_status_t extractComponent(const Image *src, Image *dst, int target_label)
{
    if (!src || !dst)
        return EMBEDDIP_ERROR_NULL_PTR;

    if (src->width != dst->width || src->height != dst->height)
        return EMBEDDIP_ERROR_INVALID_SIZE;

    uint8_t *src_data = src->pixels;
    uint8_t *dst_data = dst->pixels;

    for (uint32_t i = 0; i < src->size; i++) {
        if (src_data[i] == target_label)
            dst_data[i] = 255;  // Object pixels → white (or 1)
        else
            dst_data[i] = 0;  // Background → black (or 0)
    }

    return EMBEDDIP_OK;
}

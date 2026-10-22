// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_DETECT_H
#define EMBEDDIP_DETECT_H

#include "core/error.h"
#include "core/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Normalized ([0,1]) detection box with a confidence score.
 */
typedef struct {
    float x;     /**< Left edge */
    float y;     /**< Top edge */
    float w;     /**< Width */
    float h;     /**< Height */
    float score; /**< Confidence, [0,1] */
} ScoredBox;

/**
 * @brief Intersection-over-union of two ScoredBox regions.
 */
float boxIoU(const ScoredBox *a, const ScoredBox *b);

/**
 * @brief FOMO-style multi-detection decode of a quantized per-cell grid.
 *
 * Reads every grid cell of a [tx, ty, tw, th, obj, class] INT8 output
 * tensor (row-major, 6 values per cell, anchor (1,1) in cell units) and
 * emits one ScoredBox per cell whose combined objectness*class confidence
 * clears @p conf_threshold, instead of only the single highest-confidence
 * cell.
 *
 * @param[in]  output          Quantized model output, row-major [grid*grid][6] int8.
 * @param[in]  grid             Grid size (grid x grid cells).
 * @param[in]  scale            Dequantization scale for @p output.
 * @param[in]  zero_point       Dequantization zero point for @p output.
 * @param[in]  conf_threshold   Minimum objectness*class confidence to keep.
 * @param[out] detections       Caller-allocated output array.
 * @param[in]  max_detections   Capacity of @p detections.
 * @param[out] num_detections   Number of detections written.
 */
embeddip_status_t decodeFomoGrid(const int8_t *output,
                                 int grid,
                                 float scale,
                                 int8_t zero_point,
                                 float conf_threshold,
                                 ScoredBox *detections,
                                 int max_detections,
                                 int *num_detections);

/**
 * @brief Greedy non-max suppression over confidence-scored boxes, in place.
 *
 * Repeatedly keeps the highest-scoring remaining box and discards any
 * remaining box whose IoU with it is >= @p iou_threshold.
 *
 * @param[in,out] boxes          Boxes to filter, in place (score field is
 *                                 destroyed: consumed entries are set to -1).
 * @param[in]     num_boxes      Number of entries in @p boxes.
 * @param[in]     iou_threshold  IoU at/above which a box is suppressed.
 * @param[out]    num_kept       Number of boxes kept (front of array).
 */
embeddip_status_t nonMaxSuppressBoxes(ScoredBox *boxes, int num_boxes, float iou_threshold, int *num_kept);

#ifdef __cplusplus
}
#endif

#endif  // EMBEDDIP_DETECT_H

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef EMBEDDIP_CV_DETECT_H
#define EMBEDDIP_CV_DETECT_H

#include <stddef.h>
#include <stdint.h>

#include "core/error.h"
#include "core/image.h"
#include "cv/haar.h"
#include "cv/integral.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief One detection: a window rectangle and its cascade score.
 */
typedef struct {
    Rectangle box; /**< Detected window in integral-image pixels. */
    int32_t score; /**< Cascade confidence (see cv_haar_cascade_score). */
} CvDetection;

/**
 * @brief Sliding-window scan configuration.
 */
typedef struct {
    uint16_t window_width;  /**< Scan window width in pixels (> 0). */
    uint16_t window_height; /**< Scan window height in pixels (> 0). */
    uint16_t step_x;        /**< Horizontal step in pixels (> 0). */
    uint16_t step_y;        /**< Vertical step in pixels (> 0). */
} CvScanConfig;

/**
 * @brief Slide a Haar cascade across an integral image, collecting passes.
 *
 * Appends detections to @p out (never exceeding @p out_capacity) starting at
 * @p *out_count, so the same buffer can accumulate detections from several
 * scales across multiple calls. The window's own dimensions are used; the
 * cascade's stored window is ignored so a caller can reuse one cascade across
 * scales by scaling the integral image instead.
 *
 * @param[in] table Integral image to scan.
 * @param[in] cascade Cascade model, read-only.
 * @param[in] scan Window and step configuration.
 * @param[in,out] out Caller-owned detection buffer.
 * @param[in] out_capacity Capacity of @p out.
 * @param[in,out] out_count In: existing count; out: count after appending.
 * @return EMBEDDIP_OK on success (including a full buffer that stops early),
 *         error code otherwise.
 */
embeddip_status_t cv_detect_scan(const CvIntegralU32 *table,
                                 const CvHaarCascade *cascade,
                                 const CvScanConfig *scan, CvDetection *out,
                                 size_t out_capacity, size_t *out_count);

/**
 * @brief Greedy non-maximum suppression over detections.
 *
 * Sorts by descending score (stable, lower index wins ties), then keeps a
 * detection only if its intersection-over-union with every already-kept
 * detection is at most @p iou_threshold. Operates in place: survivors are
 * moved to the front of @p detections and the survivor count is returned.
 *
 * @param[in,out] detections Detections to filter, reordered in place.
 * @param[in] count Number of input detections.
 * @param[in] iou_threshold IoU above which a lower-scored box is suppressed
 *            (0.0..1.0).
 * @param[out] out_kept Number of survivors left at the front.
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_detect_nms(CvDetection *detections, size_t count,
                                float iou_threshold, size_t *out_kept);

/**
 * @brief A neural-network detection: box (in input-image pixels), class, score.
 */
typedef struct {
    Rectangle box;  /**< Bounding box in input-image pixels. */
    int32_t cls;    /**< Predicted class index. */
    float score;    /**< Confidence (max activating cell in the blob). */
} CvNnDetection;

/**
 * @brief Decode a FOMO-style class grid into object detections.
 *
 * FOMO ("Faster Objects, More Objects") outputs a coarse @p grid_w x @p grid_h
 * grid of per-cell class scores. This takes each cell's arg-max class; cells
 * whose winning class is not @p bg_class and whose score exceeds @p threshold
 * are "active". 8-connected active cells of the same class are grouped into one
 * detection whose box is the group's cell-bounding-box scaled to the input
 * resolution and whose score is the strongest cell in the group.
 *
 * @param[in] grid Per-cell scores, row-major with class fastest:
 *            grid[(y*grid_w + x)*num_classes + c].
 * @param[in] grid_w Grid width in cells (> 0).
 * @param[in] grid_h Grid height in cells (> 0).
 * @param[in] num_classes Number of classes including background (> 1).
 * @param[in] bg_class Background class index to ignore.
 * @param[in] threshold Minimum winning-class score for a cell to be active.
 * @param[in] in_width Input image width in pixels (for box scaling).
 * @param[in] in_height Input image height in pixels.
 * @param[out] out Caller-owned detection buffer, filled by descending score.
 * @param[in] out_capacity Capacity of @p out.
 * @param[out] out_count Number of detections written (<= out_capacity).
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_detect_fomo_decode(const float *grid, int grid_w, int grid_h,
                                        int num_classes, int bg_class, float threshold,
                                        int in_width, int in_height, CvNnDetection *out,
                                        size_t out_capacity, size_t *out_count);

/**
 * @brief YOLO (v2/v3-tiny style) detection-head configuration.
 */
typedef struct {
    int grid_w;         /**< Output grid width in cells. */
    int grid_h;         /**< Output grid height in cells. */
    int num_anchors;    /**< Anchor boxes per cell. */
    int num_classes;    /**< Object classes. */
    const float *anchors; /**< num_anchors*2 (w,h) in grid-cell units. */
    int in_width;       /**< Network input width (pixels). */
    int in_height;      /**< Network input height (pixels). */
    float conf_threshold; /**< Keep boxes with objectness*class_prob above this. */
    float iou_threshold;  /**< Class-aware NMS IoU threshold. */
} CvYoloConfig;

/**
 * @brief Decode a YOLO detection head into boxes (+ class-aware NMS).
 *
 * @p pred is laid out per cell then per anchor:
 * pred[((r*grid_w + c)*num_anchors + a)*(5 + num_classes) + k], with k =
 * 0..3 = tx,ty,tw,th, 4 = objectness logit, 5.. = per-class logits. Applies the
 * YOLO box transform (sigmoid centre + anchor*exp size), confidence =
 * sigmoid(obj)*sigmoid(best class), thresholds, then greedy per-class NMS.
 * Boxes are in input-image pixels.
 *
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_detect_yolo_decode(const float *pred, const CvYoloConfig *cfg,
                                        CvNnDetection *out, size_t out_capacity,
                                        size_t *out_count);

/**
 * @brief SSD (MobileNet-SSD style) detection-head configuration.
 */
typedef struct {
    int num_priors;   /**< Number of prior/anchor boxes. */
    int num_classes;  /**< Classes including background at index 0. */
    const float *priors; /**< num_priors*4 (cx,cy,w,h), normalized [0,1]. */
    float var_xy;     /**< Centre variance (typ. 0.1). */
    float var_wh;     /**< Size variance (typ. 0.2). */
    int in_width;     /**< Input width (pixels). */
    int in_height;    /**< Input height (pixels). */
    float conf_threshold; /**< Minimum class probability. */
    float iou_threshold;  /**< Class-aware NMS IoU threshold. */
} CvSsdConfig;

/**
 * @brief Decode an SSD detection head into boxes (+ class-aware NMS).
 *
 * @p loc is num_priors*4 box offsets (dcx,dcy,dw,dh); @p conf is
 * num_priors*num_classes class probabilities (softmax already applied).
 * Applies the standard SSD decode (centre += offset*var*prior_size, size *=
 * exp(offset*var)) against @p priors, drops background (class 0), thresholds,
 * then greedy per-class NMS. Boxes are in input-image pixels.
 *
 * @return EMBEDDIP_OK on success, error code otherwise.
 */
embeddip_status_t cv_detect_ssd_decode(const float *loc, const float *conf,
                                       const CvSsdConfig *cfg, CvNnDetection *out,
                                       size_t out_capacity, size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* EMBEDDIP_CV_DETECT_H */

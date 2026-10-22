// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

#ifndef PIXEL_H
#define PIXEL_H

#include "board/common.h"
#include "core/image.h"
#include "core/memory_manager.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Include all imgproc sub-modules (OpenCV-style organization)
// ============================================================================

#include "cascade.h"              // Integral image and Haar-lite cascade detection
#include "color.h"                // Color space conversions
#include "connectedcomponents.h"  // Connected component analysis
#include "detect.h"               // NN detection post-processing (grid decode, NMS)
#include "drawing.h"              // Drawing primitives
#include "features.h"             // Keypoint detection and descriptor matching
#include "fft.h"                  // FFT operations
#include "filter.h"               // Filtering operations
#include "histogram.h"            // Histogram operations
#include "hog.h"                  // HOG-lite descriptor and k-NN classification
#include "hough.h"                // Hough transform
#include "imgwarp.h"              // Geometric transformations
#include "misc.h"                 // Arithmetic and logical operations
#include "morph.h"                // Morphological operations
#include "segmentation.h"         // Segmentation algorithms
#include "thresh.h"               // Thresholding and intensity transforms

#ifdef __cplusplus
}
#endif

#endif  // PIXEL_H

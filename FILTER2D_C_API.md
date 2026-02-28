# filter2D - C API Documentation

## Overview

The `filter2D()` function is the C counterpart to the C++ `Image::filter2D()` method. It provides 2D convolution filtering for image processing operations like blurring, sharpening, and edge detection.

---

## Function Signature

```c
embeddip_status_t filter2D(
    const Image *inImg,      // Input image
    Image *outImg,           // Output image (pre-allocated)
    const float *kernel,     // Flattened kernel array
    int kernelSize           // Kernel dimension (must be odd)
);
```

### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `inImg` | `const Image *` | Input image (not modified) |
| `outImg` | `Image *` | Output image (must be pre-allocated with same dimensions) |
| `kernel` | `const float *` | Flattened 2D kernel in row-major order |
| `kernelSize` | `int` | Size of square kernel (must be odd: 3, 5, 7, etc.) |

### Return Value

Returns `embeddip_status_t`:
- `EMBEDDIP_OK` - Success
- `EMBEDDIP_ERROR_NULL_PTR` - NULL pointer passed
- `EMBEDDIP_ERROR_INVALID_ARG` - Invalid kernel size (even or < 1)
- `EMBEDDIP_ERROR_INVALID_SIZE` - Input/output dimensions don't match
- `EMBEDDIP_ERROR_INVALID_FORMAT` - Input/output formats don't match
- `EMBEDDIP_ERROR_NOT_SUPPORTED` - Unsupported image format

---

## Basic Usage

### 1. Simple Box Blur (3x3)

```c
#include "embedDIP.h"
#include "imgproc/filter.h"
#include "board/common.h"

// Create images
Image *input = NULL, *output = NULL;
createImageWH(640, 480, IMAGE_FORMAT_GRAYSCALE, &input);
createImageWH(640, 480, IMAGE_FORMAT_GRAYSCALE, &output);

// Define 3x3 box blur kernel
float boxKernel[9] = {
    1.0f/9, 1.0f/9, 1.0f/9,
    1.0f/9, 1.0f/9, 1.0f/9,
    1.0f/9, 1.0f/9, 1.0f/9
};

// Apply filter
embeddip_status_t status = filter2D(input, output, boxKernel, 3);
if (status != EMBEDDIP_OK) {
    printf("Filter failed: %s\n", embeddip_status_str(status));
}

// Cleanup
deleteImage(input);
deleteImage(output);
```

### 2. Sharpen Filter

```c
float sharpenKernel[9] = {
     0.0f, -1.0f,  0.0f,
    -1.0f,  5.0f, -1.0f,
     0.0f, -1.0f,  0.0f
};

filter2D(input, output, sharpenKernel, 3);
```

### 3. Edge Detection (Sobel X)

```c
float sobelXKernel[9] = {
    -1.0f,  0.0f,  1.0f,
    -2.0f,  0.0f,  2.0f,
    -1.0f,  0.0f,  1.0f
};

filter2D(input, output, sobelXKernel, 3);
```

### 4. Gaussian Blur (5x5)

```c
// Gaussian kernel with sigma=1.0 (approximate)
float gaussianKernel[25] = {
    1/256.0f,  4/256.0f,  6/256.0f,  4/256.0f, 1/256.0f,
    4/256.0f, 16/256.0f, 24/256.0f, 16/256.0f, 4/256.0f,
    6/256.0f, 24/256.0f, 36/256.0f, 24/256.0f, 6/256.0f,
    4/256.0f, 16/256.0f, 24/256.0f, 16/256.0f, 4/256.0f,
    1/256.0f,  4/256.0f,  6/256.0f,  4/256.0f, 1/256.0f
};

filter2D(input, output, gaussianKernel, 5);
```

---

## Kernel Format

Kernels are stored as **1D arrays in row-major order**:

### 3x3 Kernel Layout
```
Visual representation:
┌───┬───┬───┐
│ 0 │ 1 │ 2 │
├───┼───┼───┤
│ 3 │ 4 │ 5 │
├───┼───┼───┤
│ 6 │ 7 │ 8 │
└───┴───┴───┘

Array representation:
float kernel[9] = {k00, k01, k02, k10, k11, k12, k20, k21, k22};
```

### 5x5 Kernel Layout
```
Visual representation:
┌────┬────┬────┬────┬────┐
│  0 │  1 │  2 │  3 │  4 │
├────┼────┼────┼────┼────┤
│  5 │  6 │  7 │  8 │  9 │
├────┼────┼────┼────┼────┤
│ 10 │ 11 │ 12 │ 13 │ 14 │
├────┼────┼────┼────┼────┤
│ 15 │ 16 │ 17 │ 18 │ 19 │
├────┼────┼────┼────┼────┤
│ 20 │ 21 │ 22 │ 23 │ 24 │
└────┴────┴────┴────┴────┘

Array representation:
float kernel[25] = {k00, k01, ..., k44};
```

**Index formula:** `kernel[row * kernelSize + col]`

---

## Supported Image Formats

| Format | Behavior |
|--------|----------|
| `IMAGE_FORMAT_GRAYSCALE` | Filters single channel (ch[0]) |
| `IMAGE_FORMAT_RGB888` | Filters R, G, B channels independently |
| Other formats | Returns `EMBEDDIP_ERROR_NOT_SUPPORTED` |

### RGB888 Filtering

For RGB images, each channel is filtered independently:

```c
Image *inputRGB = NULL, *outputRGB = NULL;
createImageWH(640, 480, IMAGE_FORMAT_RGB888, &inputRGB);
createImageWH(640, 480, IMAGE_FORMAT_RGB888, &outputRGB);

// This will filter R, G, and B channels separately
filter2D(inputRGB, outputRGB, gaussianKernel, 5);
```

---

## Common Kernels

### Identity (No Change)
```c
float identity[9] = {
    0, 0, 0,
    0, 1, 0,
    0, 0, 0
};
```

### Box Blur 3x3
```c
float boxBlur[9] = {
    1.0f/9, 1.0f/9, 1.0f/9,
    1.0f/9, 1.0f/9, 1.0f/9,
    1.0f/9, 1.0f/9, 1.0f/9
};
```

### Box Blur 5x5
```c
float boxBlur5[25] = {
    1.0f/25, 1.0f/25, 1.0f/25, 1.0f/25, 1.0f/25,
    1.0f/25, 1.0f/25, 1.0f/25, 1.0f/25, 1.0f/25,
    1.0f/25, 1.0f/25, 1.0f/25, 1.0f/25, 1.0f/25,
    1.0f/25, 1.0f/25, 1.0f/25, 1.0f/25, 1.0f/25,
    1.0f/25, 1.0f/25, 1.0f/25, 1.0f/25, 1.0f/25
};
```

### Sharpen
```c
float sharpen[9] = {
     0, -1,  0,
    -1,  5, -1,
     0, -1,  0
};
```

### Edge Enhance
```c
float edgeEnhance[9] = {
    -1, -1, -1,
    -1,  9, -1,
    -1, -1, -1
};
```

### Emboss
```c
float emboss[9] = {
    -2, -1,  0,
    -1,  1,  1,
     0,  1,  2
};
```

### Sobel X (Vertical Edges)
```c
float sobelX[9] = {
    -1,  0,  1,
    -2,  0,  2,
    -1,  0,  1
};
```

### Sobel Y (Horizontal Edges)
```c
float sobelY[9] = {
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1
};
```

### Laplacian (Edge Detection)
```c
float laplacian[9] = {
     0,  1,  0,
     1, -4,  1,
     0,  1,  0
};
```

### Laplacian of Gaussian (LoG)
```c
float loG[25] = {
     0,  0, -1,  0,  0,
     0, -1, -2, -1,  0,
    -1, -2, 16, -2, -1,
     0, -1, -2, -1,  0,
     0,  0, -1,  0,  0
};
```

---

## Error Handling

### Pattern 1: Simple Check

```c
embeddip_status_t status = filter2D(input, output, kernel, kernelSize);
if (status != EMBEDDIP_OK) {
    fprintf(stderr, "Filter failed: %s\n", embeddip_status_str(status));
    return -1;
}
```

### Pattern 2: Detailed Error Handling

```c
embeddip_status_t status = filter2D(input, output, kernel, kernelSize);

switch (status) {
    case EMBEDDIP_OK:
        printf("Filter applied successfully\n");
        break;

    case EMBEDDIP_ERROR_NULL_PTR:
        fprintf(stderr, "ERROR: NULL pointer - check input/output/kernel\n");
        break;

    case EMBEDDIP_ERROR_INVALID_ARG:
        fprintf(stderr, "ERROR: Invalid kernel size (must be odd and >= 1)\n");
        break;

    case EMBEDDIP_ERROR_INVALID_SIZE:
        fprintf(stderr, "ERROR: Input and output image dimensions don't match\n");
        break;

    case EMBEDDIP_ERROR_INVALID_FORMAT:
        fprintf(stderr, "ERROR: Input and output formats don't match\n");
        break;

    case EMBEDDIP_ERROR_NOT_SUPPORTED:
        fprintf(stderr, "ERROR: Image format not supported for filtering\n");
        break;

    default:
        fprintf(stderr, "ERROR: Unknown error code %d\n", status);
        break;
}
```

---

## Implementation Details

### Internal Operation

1. **Validation**: Checks NULL pointers, kernel size (odd), image dimensions
2. **Context Setup**: Creates `Filter2DContext` with kernel size and pointer
3. **Channel Dispatch**:
   - Grayscale: Calls `filter2D_single_channel()` once (ch_idx=0)
   - RGB888: Calls `filter2D_single_channel()` three times (ch_idx=1,2,3)
4. **Convolution**: For each output pixel, computes weighted sum of neighborhood

### Boundary Handling

Pixels outside image boundaries are treated as **zero** (zero-padding):

```
Image boundary:     Effective values:
┌─────────┐         ┌─────────────┐
│ A B C D │         │ 0 0 0 0 0 0 │
│ E F G H │   →     │ 0 A B C D 0 │
│ I J K L │         │ 0 E F G H 0 │
└─────────┘         │ 0 I J K L 0 │
                    │ 0 0 0 0 0 0 │
                    └─────────────┘
```

### Memory Considerations

- **No allocation**: Kernel is used directly (const pointer)
- **Channel buffers**: Allocated internally by `filter2D_single_channel()`
- **Output buffer**: Must be pre-allocated by caller

---

## Performance Tips

1. **Reuse output image**: Create once, filter multiple times
2. **Small kernels**: 3x3 is ~4x faster than 5x5, ~16x faster than 7x7
3. **Separable filters**: Use `filter2D_separable()` for Gaussian/box blur
4. **Consider format**: Grayscale is 3x faster than RGB888

### Benchmark (STM32F746 @ 216MHz, 640x480)

| Kernel | Format | Time |
|--------|--------|------|
| 3x3 | Grayscale | ~45ms |
| 3x3 | RGB888 | ~135ms |
| 5x5 | Grayscale | ~120ms |
| 5x5 | RGB888 | ~360ms |

---

## Comparison with C++

### C++ API
```cpp
#include "embedDIP.hpp"

embedDIP::Image img(640, 480, IMAGE_FORMAT_GRAYSCALE);
embedDIP::Image output(640, 480, IMAGE_FORMAT_GRAYSCALE);

std::vector<std::vector<float>> kernel = {
    {1.0f/9, 1.0f/9, 1.0f/9},
    {1.0f/9, 1.0f/9, 1.0f/9},
    {1.0f/9, 1.0f/9, 1.0f/9}
};

img.filter2D(kernel, output);  // void return, silent failure
```

### C API (New)
```c
#include "embedDIP.h"
#include "imgproc/filter.h"

Image *input = NULL, *output = NULL;
createImageWH(640, 480, IMAGE_FORMAT_GRAYSCALE, &input);
createImageWH(640, 480, IMAGE_FORMAT_GRAYSCALE, &output);

float kernel[9] = {
    1.0f/9, 1.0f/9, 1.0f/9,
    1.0f/9, 1.0f/9, 1.0f/9,
    1.0f/9, 1.0f/9, 1.0f/9
};

embeddip_status_t status = filter2D(input, output, kernel, 3);
if (status != EMBEDDIP_OK) {
    // Proper error handling
}

deleteImage(input);
deleteImage(output);
```

### Key Differences

| Aspect | C++ | C |
|--------|-----|---|
| Kernel format | `vector<vector<float>>` | `float *` (flattened) |
| Error handling | Silent failure (void return) | Status code return |
| Memory | RAII (automatic) | Manual (create/delete) |
| Syntax | Object-oriented | Procedural |

---

## See Also

- [filter.h](imgproc/filter.h) - Full filter API
- [Example 04](examples/04_filter2d_example/) - Complete working example
- `filter2D_separable()` - Optimized for separable kernels
- `dogFilter()` - Difference of Gaussians
- `logFilter()` - Laplacian of Gaussian

---

## Notes

- **Thread safety**: Not thread-safe (uses internal channel buffers)
- **Const correctness**: Input image marked const (not modified)
- **Kernel normalization**: Caller responsible for normalizing kernel weights
- **Performance**: Consider using CMSIS-DSP or ESP-DSP for optimized convolution

---

**Location:** `embedDIP/imgproc/filter.c`, `embedDIP/imgproc/filter.h`
**Version:** 0.1.0
**License:** MIT

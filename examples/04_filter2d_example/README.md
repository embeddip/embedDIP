# Example 04: 2D Filtering with filter2D

This example demonstrates the C `filter2D()` function, which is the counterpart to the C++ `Image::filter2D()` method.

## What You'll Learn

- Using `filter2D()` for 2D convolution filtering
- Creating various filter kernels (box blur, sharpen, edge detection, Gaussian)
- Filtering grayscale and RGB images
- Error handling with embeddip_status_t

## Common Filter Kernels

### Box Blur (3x3)
Averages pixels for simple smoothing:
```c
float boxKernel[9] = {
    1.0f/9, 1.0f/9, 1.0f/9,
    1.0f/9, 1.0f/9, 1.0f/9,
    1.0f/9, 1.0f/9, 1.0f/9
};
filter2D(input, output, boxKernel, 3);
```

### Sharpen (3x3)
Enhances edges and details:
```c
float sharpenKernel[9] = {
     0.0f, -1.0f,  0.0f,
    -1.0f,  5.0f, -1.0f,
     0.0f, -1.0f,  0.0f
};
filter2D(input, output, sharpenKernel, 3);
```

### Sobel X (3x3)
Detects vertical edges:
```c
float sobelXKernel[9] = {
    -1.0f,  0.0f,  1.0f,
    -2.0f,  0.0f,  2.0f,
    -1.0f,  0.0f,  1.0f
};
filter2D(input, output, sobelXKernel, 3);
```

### Sobel Y (3x3)
Detects horizontal edges:
```c
float sobelYKernel[9] = {
    -1.0f, -2.0f, -1.0f,
     0.0f,  0.0f,  0.0f,
     1.0f,  2.0f,  1.0f
};
filter2D(input, output, sobelYKernel, 3);
```

### Gaussian Blur (Generated)
Creates smooth blur with Gaussian distribution:
```c
float gaussianKernel[25];
createGaussianKernel(gaussianKernel, 5, 1.0f);  // 5x5, sigma=1.0
filter2D(input, output, gaussianKernel, 5);
```

## Function Signature

```c
embeddip_status_t filter2D(
    const Image *inImg,      // Input image
    Image *outImg,           // Output image (pre-allocated, same size)
    const float *kernel,     // Flattened kernel array (size x size)
    int kernelSize           // Kernel dimension (must be odd: 3, 5, 7, ...)
);
```

## Kernel Layout

Kernels are passed as 1D arrays in **row-major order**:

For a 3x3 kernel:
```
Visual:        Array:
[ 0  1  2 ]    kernel[0..8] = {k00, k01, k02,
[ 3  4  5 ]                     k10, k11, k12,
[ 6  7  8 ]                     k20, k21, k22}
```

For a 5x5 kernel:
```
Visual:        Array:
[ 0  1  2  3  4 ]    kernel[0..24] = {k00, k01, k02, k03, k04,
[ 5  6  7  8  9 ]                      k10, k11, k12, k13, k14,
[10 11 12 13 14 ]                      k20, k21, k22, k23, k24,
[15 16 17 18 19 ]                      k30, k31, k32, k33, k34,
[20 21 22 23 24 ]                      k40, k41, k42, k43, k44}
```

## Supported Formats

| Format | Behavior |
|--------|----------|
| `IMAGE_FORMAT_GRAYSCALE` | Filters single channel |
| `IMAGE_FORMAT_RGB888` | Filters R, G, B channels independently |
| Others | Returns `EMBEDDIP_ERROR_NOT_SUPPORTED` |

## Error Handling

```c
embeddip_status_t status = filter2D(input, output, kernel, kernelSize);
if (status != EMBEDDIP_OK) {
    fprintf(stderr, "Filter failed: %s\n", embeddip_status_str(status));
    return -1;
}
```

### Possible Errors

| Error | Cause |
|-------|-------|
| `EMBEDDIP_ERROR_NULL_PTR` | NULL input, output, or kernel |
| `EMBEDDIP_ERROR_INVALID_ARG` | Kernel size < 1 or even |
| `EMBEDDIP_ERROR_INVALID_SIZE` | Input/output size mismatch |
| `EMBEDDIP_ERROR_INVALID_FORMAT` | Input/output format mismatch |
| `EMBEDDIP_ERROR_NOT_SUPPORTED` | Unsupported image format |

## Building and Running

```bash
cd embedDIP/build
cmake .. -DEMBEDDIP_TARGET_PLATFORM=HOST -DEMBEDDIP_BUILD_EXAMPLES=ON
make
./examples/04_filter2d_example/example_04_filter2d_example
```

## Expected Output

```
=== embedDIP Example 04: 2D Filtering ===

1. Creating 320x240 grayscale test image...
   ✓ Test pattern created (vertical stripes)

2. Applying 3x3 box blur filter...
   ✓ Box blur applied successfully

3. Applying 3x3 sharpen filter...
   ✓ Sharpen filter applied successfully

4. Applying 3x3 Sobel X edge detection...
   ✓ Sobel X edge detection applied

5. Creating and applying 5x5 Gaussian blur (sigma=1.0)...
   ✓ Gaussian blur applied successfully
   Kernel center value: 0.159155

6. Testing with RGB888 image...
   ✓ RGB888 box blur applied to all 3 channels

7. Testing error handling...
   ✓ NULL input: EMBEDDIP_ERROR_NULL_PTR
   ✓ Even kernel size: EMBEDDIP_ERROR_INVALID_ARG
   ✓ Size mismatch: EMBEDDIP_ERROR_INVALID_SIZE

8. Cleaning up resources...
   ✓ All resources freed

=== Example completed successfully! ===
```

## Comparison: C vs C++

### C++ (Original)
```cpp
#include "embedDIP.hpp"

embedDIP::Image img("input.jpg");
embedDIP::Image output(img.width(), img.height(), IMAGE_FORMAT_GRAYSCALE);

std::vector<std::vector<float>> kernel = {
    {1.0f/9, 1.0f/9, 1.0f/9},
    {1.0f/9, 1.0f/9, 1.0f/9},
    {1.0f/9, 1.0f/9, 1.0f/9}
};

img.filter2D(kernel, output);
```

### C (New)
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
    // Handle error
}

deleteImage(input);
deleteImage(output);
```

## Performance Tips

1. **Pre-allocate output image**: Create once, reuse multiple times
2. **Use separable filters when possible**: `filter2D_separable()` is faster for Gaussian/box filters
3. **Small kernels are faster**: 3x3 is 4x faster than 5x5
4. **Consider in-place**: For memory-constrained systems, allocate temporary buffer

## Next Steps

- Try creating custom edge detection kernels
- Combine multiple filters (e.g., blur + sharpen)
- Implement real-time video filtering on embedded hardware
- Explore `filter2D_separable()` for optimized Gaussian blur

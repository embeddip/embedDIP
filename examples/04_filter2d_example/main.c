/* ========================================================================== */
/*  Example 04: 2D Filtering with filter2D                                   */
/*  Demonstrates convolution filtering with various kernels                  */
/*  SPDX-License-Identifier: MIT                                              */
/* ========================================================================== */

#include "embedDIP.h"
#include "imgproc/filter.h"
#include "board/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/**
 * @brief Create a Gaussian blur kernel
 */
void createGaussianKernel(float *kernel, int size, float sigma)
{
    int half = size / 2;
    float sum = 0.0f;
    float s = 2.0f * sigma * sigma;

    // Generate Gaussian kernel
    for (int y = -half; y <= half; y++) {
        for (int x = -half; x <= half; x++) {
            float r = sqrtf((float)(x * x + y * y));
            int idx = (y + half) * size + (x + half);
            kernel[idx] = expf(-(r * r) / s) / (M_PI * s);
            sum += kernel[idx];
        }
    }

    // Normalize
    for (int i = 0; i < size * size; i++) {
        kernel[i] /= sum;
    }
}

int main(void)
{
    printf("=== embedDIP Example 04: 2D Filtering ===\n\n");

    embeddip_status_t status;

    /* ===== 1. Create Input Image ===== */
    printf("1. Creating 320x240 grayscale test image...\n");
    Image *input = NULL;
    status = createImageWH(320, 240, IMAGE_FORMAT_GRAYSCALE, &input);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Failed to create input image: %s\n", embeddip_status_str(status));
        return EXIT_FAILURE;
    }

    // Fill with test pattern (vertical stripes)
    uint8_t *pixels = (uint8_t *)input->pixels;
    for (int y = 0; y < input->height; y++) {
        for (int x = 0; x < input->width; x++) {
            int idx = y * input->width + x;
            pixels[idx] = (x % 20 < 10) ? 255 : 0;  // Vertical stripes
        }
    }
    printf("   ✓ Test pattern created (vertical stripes)\n");

    /* ===== 2. Create Output Image ===== */
    Image *output = NULL;
    status = createImageWH(320, 240, IMAGE_FORMAT_GRAYSCALE, &output);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Failed to create output image: %s\n", embeddip_status_str(status));
        deleteImage(input);
        return EXIT_FAILURE;
    }

    /* ===== 3. Example 1: Box Blur (3x3) ===== */
    printf("\n2. Applying 3x3 box blur filter...\n");
    float boxKernel[9] = {
        1.0f / 9, 1.0f / 9, 1.0f / 9, 1.0f / 9, 1.0f / 9, 1.0f / 9, 1.0f / 9, 1.0f / 9, 1.0f / 9};

    status = filter2D(input, output, boxKernel, 3);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Box blur failed: %s\n", embeddip_status_str(status));
    } else {
        printf("   ✓ Box blur applied successfully\n");
    }

    /* ===== 4. Example 2: Sharpen Filter (3x3) ===== */
    printf("\n3. Applying 3x3 sharpen filter...\n");
    float sharpenKernel[9] = {0.0f, -1.0f, 0.0f, -1.0f, 5.0f, -1.0f, 0.0f, -1.0f, 0.0f};

    status = filter2D(input, output, sharpenKernel, 3);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Sharpen failed: %s\n", embeddip_status_str(status));
    } else {
        printf("   ✓ Sharpen filter applied successfully\n");
    }

    /* ===== 5. Example 3: Edge Detection (3x3 Sobel X) ===== */
    printf("\n4. Applying 3x3 Sobel X edge detection...\n");
    float sobelXKernel[9] = {-1.0f, 0.0f, 1.0f, -2.0f, 0.0f, 2.0f, -1.0f, 0.0f, 1.0f};

    status = filter2D(input, output, sobelXKernel, 3);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Sobel X failed: %s\n", embeddip_status_str(status));
    } else {
        printf("   ✓ Sobel X edge detection applied\n");
    }

    /* ===== 6. Example 4: Gaussian Blur (5x5) ===== */
    printf("\n5. Creating and applying 5x5 Gaussian blur (sigma=1.0)...\n");
    float gaussianKernel[25];
    createGaussianKernel(gaussianKernel, 5, 1.0f);

    status = filter2D(input, output, gaussianKernel, 5);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Gaussian blur failed: %s\n", embeddip_status_str(status));
    } else {
        printf("   ✓ Gaussian blur applied successfully\n");
        printf("   Kernel center value: %.6f\n", gaussianKernel[12]);  // Center of 5x5
    }

    /* ===== 7. Test RGB888 Format ===== */
    printf("\n6. Testing with RGB888 image...\n");
    Image *inputRGB = NULL, *outputRGB = NULL;
    status = createImageWH(160, 120, IMAGE_FORMAT_RGB888, &inputRGB);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Failed to create RGB input: %s\n", embeddip_status_str(status));
        deleteImage(input);
        deleteImage(output);
        return EXIT_FAILURE;
    }

    status = createImageWH(160, 120, IMAGE_FORMAT_RGB888, &outputRGB);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "Failed to create RGB output: %s\n", embeddip_status_str(status));
        deleteImage(input);
        deleteImage(output);
        deleteImage(inputRGB);
        return EXIT_FAILURE;
    }

    // Fill RGB with color pattern
    uint8_t *rgbPixels = (uint8_t *)inputRGB->pixels;
    for (int i = 0; i < 160 * 120; i++) {
        rgbPixels[i * 3 + 0] = 255;         // R
        rgbPixels[i * 3 + 1] = (i % 256);   // G (gradient)
        rgbPixels[i * 3 + 2] = 128;         // B
    }

    status = filter2D(inputRGB, outputRGB, boxKernel, 3);
    if (status != EMBEDDIP_OK) {
        fprintf(stderr, "RGB filtering failed: %s\n", embeddip_status_str(status));
    } else {
        printf("   ✓ RGB888 box blur applied to all 3 channels\n");
    }

    /* ===== 8. Error Testing ===== */
    printf("\n7. Testing error handling...\n");

    // Test NULL pointer
    status = filter2D(NULL, output, boxKernel, 3);
    printf("   NULL input: %s %s\n",
           status == EMBEDDIP_ERROR_NULL_PTR ? "✓" : "✗",
           embeddip_status_str(status));

    // Test even kernel size
    status = filter2D(input, output, boxKernel, 4);
    printf("   Even kernel size: %s %s\n",
           status == EMBEDDIP_ERROR_INVALID_ARG ? "✓" : "✗",
           embeddip_status_str(status));

    // Test size mismatch
    Image *smallOutput = NULL;
    createImageWH(100, 100, IMAGE_FORMAT_GRAYSCALE, &smallOutput);
    status = filter2D(input, smallOutput, boxKernel, 3);
    printf("   Size mismatch: %s %s\n",
           status == EMBEDDIP_ERROR_INVALID_SIZE ? "✓" : "✗",
           embeddip_status_str(status));
    deleteImage(smallOutput);

    /* ===== 9. Cleanup ===== */
    printf("\n8. Cleaning up resources...\n");
    deleteImage(input);
    deleteImage(output);
    deleteImage(inputRGB);
    deleteImage(outputRGB);
    printf("   ✓ All resources freed\n");

    printf("\n=== Example completed successfully! ===\n");
    return EXIT_SUCCESS;
}
